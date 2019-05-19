#include "esp_smartcfg.h"

#define TOTAL_TIMEOUT_MS			45000
#define DEBUG_ENCODING				0

#include <stdint.h>
#include <time.h>
#include <string.h>

#include "sockets.h"

#if (defined _WIN32)
	#include <windows.h>
	#define timestamp_ms()	GetTickCount()
	#define delay_ms(MS)	Sleep(MS)
#elif (defined _LINUX)
	#define delay_ms(MS)	usleep(1000 * MS)
	#define _POSIX_C_SOURCE 200809L

	unsigned long timestamp_ms(void)
	{
		struct timespec tp;
		return clock_gettime(CLOCK_MONOTONIC, &tp) ? 0 :
		//return clock_gettime(CLOCK_MONOTONIC_RAW, &tp) ? 0 :
		(unsigned long)((tp.tv_sec * 1000) + (tp.tv_nsec / 1000000));
	}
#endif

uint8_t _crc8_update(uint8_t crc, uint8_t data)
{
	uint8_t i;

	crc = crc ^ data;
	for (i = 0; i < 8; i++)
	{
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;
	}

	return crc;
}

uint8_t _crc8_update_buf(uint8_t crc, const uint8_t* data, int len)
{
	while (len--)
	{
		crc = _crc8_update(crc, *data++);
	}
	
	return crc;
}

uint8_t _xor_update_buf(uint8_t xor, const uint8_t* data, int len)
{
	while (len--)
	{
		xor ^= *data++;
	}
	
	return xor;
}

// dont change these:
#define PACKET_MAX					1500
#define DATUMCODE_MAX				105

#define GUIDECODE_TOTAL_MS 			2000
#define GUIDECODE_DLY_MS			8
#define DATACODE_TOTAL_MS 			4000
#define DATACODE_DLY_MS				8

#define SMARTCFG_UDP_TX_PORT		7001
#define SMARTCFG_UDP_RX_PORT		18266

#define LONIB(X)					((X) & 0x0f)
#define HINIB(X)					LONIB((X) >> 4)
#define MKBYTE(H, L)				((H << 4) | ((L) & 0x0f))
#define MKINT16(H, L)				((H << 8) | ((L) & 0xff))

int tricode(uint8_t data, uint8_t index, uint16_t* tricode, const char* comment)
{
	uint8_t crc = 0x00;
	crc = _crc8_update(crc, data);
	crc = _crc8_update(crc, index);

	tricode[0] = MKINT16(0x00, MKBYTE(HINIB(crc), HINIB(data)));
	tricode[1] = MKINT16(0x01, index);
	tricode[2] = MKINT16(0x00, MKBYTE(LONIB(crc), LONIB(data)));

	#if (DEBUG_ENCODING)
	printf("TriCode: data=%3d (0x%02x) index=%3d (0x%02x) %-10s -> ", data, data, index, index, comment);
	printf("%04x, %04x, %04x; \n", tricode[0], tricode[1], tricode[2]);
	#endif

	return 3;
}

int make_datumcode(
	uint16_t* datumcode, // must be sized at least DATUMCODE_MAX!
	const char* essid_str,
	const char* bssid_str,
	const char* passwd_str,
	const char* ownaddr_str,
	bool ishidden
)
{
	#if (DEBUG_ENCODING)
	printf("------------------------------\n");
	printf("ESSID    = \"%s\"\n", essid_str);
	printf("BSSID    = %s\n", bssid_str);
	printf("Password = \"%s\"\n", passwd_str);
	printf("Ip-Addr  = %s\n", ownaddr_str);
	printf("Hidden   = %s\n", ishidden ? "yes" : "no");
	#endif

	uint8_t total_xor;

	uint8_t bssid_bytes[MAC_LEN];
	parse_macstr(bssid_str, bssid_bytes);

	uint8_t ownaddr_bytes[IPADDR_LEN];
	parse_ipstr(ownaddr_str, ownaddr_bytes);

	// essid_str and passwd_str could be encrypted here, using "AES/ECB/PKCS5Padding"
	// nice small AES implementation: https://github.com/kokke/tiny-AES-c/

	uint8_t essid_len	= strlen(essid_str);
	uint8_t essid_crc	= _crc8_update_buf(0x00, essid_str, essid_len);

	uint8_t bssid_crc	= _crc8_update_buf(0x00, bssid_bytes, MAC_LEN);
	uint8_t passwd_len	= strlen(passwd_str);

	uint8_t total_len	=
		sizeof(total_len ) +
		sizeof(passwd_len) +
		sizeof(essid_crc) +
		sizeof(bssid_crc ) +
		sizeof(total_xor ) +
		IPADDR_LEN +
		passwd_len +
		essid_len;

	total_xor  = 0x00;
	total_xor ^= total_len;	
	total_xor ^= passwd_len;
	total_xor ^= essid_crc;
	total_xor ^= bssid_crc;
	total_xor = _xor_update_buf(total_xor, ownaddr_bytes, IPADDR_LEN);
	total_xor = _xor_update_buf(total_xor, passwd_str, passwd_len);
	total_xor = _xor_update_buf(total_xor, essid_str, essid_len);	

	#if (DEBUG_ENCODING)
	printf("essid_crc = 0x%02x (%3d)\n", essid_crc, essid_crc);
	printf("bssid_crc  = 0x%02x (%3d)\n", bssid_crc, bssid_crc);
	printf("essid_len = 0x%02x (%3d)\n", essid_len, essid_len);
	printf("passwd_len = 0x%02x (%3d)\n", passwd_len, passwd_len);
	printf("total_len  = 0x%02x (%3d)\n", total_len, total_len);
	printf("total_xor  = 0x%02x (%3d)\n", total_xor, total_xor);
	#endif
	
	memset(datumcode, 0, DATUMCODE_MAX);

	int n = 0; // datacode index
	int t = 0; // tricode index

	n += tricode(total_len,  t++, datumcode + n, "total_len");  // 0
	n += tricode(passwd_len, t++, datumcode + n, "passwd_len"); // 1
	n += tricode(essid_crc, t++, datumcode + n, "essid_crc"); // 2
	n += tricode(bssid_crc,  t++, datumcode + n, "bssid_crc");  // 3
	n += tricode(total_xor,  t++, datumcode + n, "total_xor");  // 4

	for (int i=0; i<IPADDR_LEN; i++)
	{
		n += tricode(ownaddr_bytes[i], t++, datumcode + n, "ipaddr");
	}
	
	for (int i=0; i<passwd_len; i++)
	{
		n += tricode(passwd_str[i], t++, datumcode + n, "ap_pwd");
	}

	if (ishidden)
	{
		for (int i=0; i<essid_len; i++)
		{
			n += tricode(essid_str[i], t++, datumcode + n, "essid");
		}
	}

	for (int i=0; i<MAC_LEN; i++)
	{
		n += tricode(bssid_bytes[i], t++, datumcode + n, "bssid");
	}

	#if (DEBUG_ENCODING)
	printf("dcode_len  = %d\n", n);
	printf("------------------------------\n");
	#endif

	return n;
}

extern volatile bool run;
SOCKET txsock = INVALID_SOCKET;
SOCKET rxsock = INVALID_SOCKET;
sockaddr_in_t txaddr;
char packet[PACKET_MAX];

int send_sized(int size)
{
	int n = sendto(txsock, packet, size, 0, (sockaddr_t*)(&txaddr), sizeof(sockaddr_in_t));
}

bool peek_reply(void)
{
	struct sockaddr_in remoteaddr;
	socklen_t addrlen = sizeof(remoteaddr);
	static uint8_t buf[PACKET_MAX];

	int rxlen = recvfrom(rxsock, buf, sizeof(buf)-1, 0, (struct sockaddr*)(&remoteaddr), &addrlen);

	if (rxlen > 0)
	{
		printf("\nrecvd %d bytes from %s \n", rxlen, inet_ntoa(remoteaddr.sin_addr));

		if (rxlen == (sizeof(char) + MAC_LEN + IPADDR_LEN))
		{
			// expecting here: buf[0] == strlen(essid_str) + strlen(passwd_str)

			const uint8_t* mac = buf + 1;
			const uint8_t* ip  = mac + MAC_LEN;

			printf("- IP:  %hhu.%hhu.%hhu.%hhu \n", ip[0], ip[1], ip[2], ip[3]);
			printf("- MAC: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

			run = false;
			return true;
		}
	}

	return false;
}

void send_guidecode(void)
{
	eprintf("G");
	unsigned long t_end = timestamp_ms() + GUIDECODE_TOTAL_MS;
	
	while ((timestamp_ms() < t_end) && run)
	{
		send_sized(515);
		delay_ms(GUIDECODE_DLY_MS);
		send_sized(514);
		delay_ms(GUIDECODE_DLY_MS);
		send_sized(513);
		delay_ms(GUIDECODE_DLY_MS);
		send_sized(512);
		delay_ms(GUIDECODE_DLY_MS);

		peek_reply();
	}
}

void send_datumcode(const uint16_t* datumcode, int dc_len)
{
	eprintf("D");

	unsigned long t_end = timestamp_ms() + DATACODE_TOTAL_MS;
	
	int i=0;
	while ((timestamp_ms() < t_end) && run)
	{
		send_sized(datumcode[i] + 40);
		delay_ms(DATACODE_DLY_MS);

		i++;
		if (i == dc_len)
		{
			i = 0;
			peek_reply();
		}
	}
}

bool esp_smartcfg_init(void)
{
	printf("smartcfg_init ...\n");

	memset(packet, '1', sizeof(packet));
	sockets_init();
	
	txsock = udpsock();
	sockopt_broadcast(txsock);
	setup_addr_in_broadcast(&txaddr, SMARTCFG_UDP_TX_PORT);

	rxsock = udpsock();
	sockaddr_in_t localaddr;
	setup_addr_in_any(&localaddr, SMARTCFG_UDP_RX_PORT);

	if (bind(rxsock, (struct sockaddr *) &localaddr, sizeof(localaddr)) != 0)
	{
		print_socketerror("bind()");
		return false;
	}
	else
	{
		printf("receiver-socket bound to: %s:%u\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));
	}

	sockopt_blocking(rxsock, false);

	return (txsock != INVALID_SOCKET) && (rxsock != INVALID_SOCKET);
}

void esp_smartcfg_deinit(void)
{
	printf("\nsmartcfg_deinit ...\n");

	closesocket(rxsock);
	rxsock = INVALID_SOCKET;

	closesocket(txsock);
	txsock = INVALID_SOCKET;

	sockets_deinit();	
}

void esp_smartcfg_run(
	const char* essid_str,
	const char* bssid_str,
	const char* passwd_str,
	const char* ownaddr_str,
	bool ishidden,
	int timeout)
{
	esp_smartcfg_init();

	uint16_t datumcode[DATUMCODE_MAX];
	int dc_len = make_datumcode(datumcode, essid_str, bssid_str, passwd_str, ownaddr_str, ishidden);

	unsigned long t_end = timestamp_ms() + (1000 * timeout);

	eprintf("sending packets: ");

	while ((timeout == 0) || (timestamp_ms() < t_end))
	{
		send_guidecode();
		if (!run) break;
		send_datumcode(datumcode, dc_len);
		if (!run) break;
	}

	esp_smartcfg_deinit();
}
