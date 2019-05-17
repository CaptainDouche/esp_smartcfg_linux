#include "esp_smartcfg.h"

#if 1 // helpers

#include <stdint.h>

#if (defined _WIN32)
	#include <windows.h>
	#define timestamp_ms()	GetTickCount()
	#define delay_ms(MS)	Sleep(MS)
#elif (defined _LINUX)
	#define delay_ms(MS)	usleep(1000 * MS)

    #define _POSIX_C_SOURCE 200809L
    #include <time.h>

    unsigned long timestamp_ms(void)
    {
        struct timespec tp;
        return clock_gettime(CLOCK_MONOTONIC, &tp) ? 0 :
        //return clock_gettime(CLOCK_MONOTONIC_RAW, &tp) ? 0 :
        (unsigned long)((tp.tv_sec * 1000) + (tp.tv_nsec / 1000000));
    }

#endif

#define eprintf(...)					fprintf (stderr, __VA_ARGS__)

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

#define LONIB(X)			((X) & 0x0f)
#define HINIB(X)			LONIB((X) >> 4)
#define MKBYTE(H, L)		((H << 4) | ((L) & 0x0f))
#define MKINT16(H, L)		((H << 8) | ((L) & 0xff))

#include <stdio.h>

bool parse_ipstr(const char* str, uint8_t* ip)
{
	int n = sscanf(str, "%hhu.%hhu.%hhu.%hhu", ip+0, ip+1, ip+2, ip+3);
	return (n == 4);
}

bool parse_macstr(const char* str, uint8_t* mac)
{
	int n = sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac+0, mac+1, mac+2, mac+3, mac+4, mac+5);
	return (n == 6);
}

#endif // helpers

#if 1 // sockets

#if (defined _WIN32)
	#include <Windows.h>
	#include <winsock.h>
	#include <winsock2.h>
	// # pragma comment(lib, "wsock32.lib")
	
	typedef struct sockaddr 	sockaddr_t;
	typedef struct sockaddr_in 	sockaddr_in_t;
	typedef struct in_addr 		in_addr_t;
	typedef int socklen_t;
	
#elif (defined _LINUX)
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
    #include <errno.h>
    #include <string.h>
	#include <fcntl.h>

	typedef int SOCKET;
	typedef struct sockaddr 	sockaddr_t;
	typedef struct sockaddr_in	sockaddr_in_t;
	//typedef struct in_addr 	in_addr_t;	
	
	#define closesocket(S)		close(S)
    #define INVALID_SOCKET		-1
#endif

#include <stdio.h>

void print_socketerror(const char* msg)
{
#if (defined _WIN32)
	int e = WSAGetLastError();
	char *s = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		e,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&s,
		0,
		NULL
	);
	eprintf("%s: %s (%d)\n", msg, s, e);
	LocalFree(s);
#elif (defined _LINUX)
	int e = errno;
	eprintf("%s: %s (%d)\n", msg, strerror(e), e);
#endif
}

bool sockets_init(void)
{
    #if (defined _WIN32)
		WSADATA wsaData;
		WORD wVerReq	= MAKEWORD(1, 1);
		WSAStartup(wVerReq, &wsaData);
    #endif
	return true;
}

void sockets_deinit(void)
{
	#if (defined _WIN32)
		WSACleanup();
    #endif
}

bool sockopt_broadcast(SOCKET sock)
{
	int yes=1;
    int res = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes));
	if (res != 0)
	{
		print_socketerror("setsockopt(SO_BROADCAST)");
		closesocket(sock);
		return false;
	}
    return true;
}

bool sockopt_blocking(SOCKET sock, bool blocking)
{
	#if (defined _WIN32)
		u_long flags = blocking ? 0 : 1;
		bool res = (NO_ERROR == ioctlsocket(sock, FIONBIO, &flags));
	#elif (defined _LINUX)
		const int flags = fcntl(sock, F_GETFL, 0);
		bool res = (0 == fcntl(sock, F_SETFL, blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK));
	#endif
	
	if (!res)
	{
		print_socketerror("sockopt_blocking");
	}
	
	return res;
}

SOCKET _udpsock(void)
{
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET)
	{
		print_socketerror("socket() failed");
		return INVALID_SOCKET;
	}

	return sock;
}

#endif

#define TOTAL_TIMEOUT_MS			45000
#define DEBUG_ENODING				0

// dont change these:
#define MAC_LEN						6
#define IPADDR_LEN					4
#define PACKET_MAX					1500
#define DATUMCODE_MAX				105

#define GUIDECODE_TOTAL_MS 			2000
#define GUIDECODE_DLY_MS			8
#define DATACODE_TOTAL_MS 			4000
#define DATACODE_DLY_MS				8

#define SMARTCFG_UDP_TX_PORT	   7001
#define SMARTCFG_UDP_RX_PORT	   18266

extern volatile bool run;

SOCKET txsock = INVALID_SOCKET;
sockaddr_in_t txaddr;
char packet[PACKET_MAX];

SOCKET rxsock = INVALID_SOCKET;

int send_sized(int size)
{
    int n = sendto(txsock, packet, size, 0, (sockaddr_t*)(&txaddr), sizeof(sockaddr_in_t));
}

#include <string.h>

int esp_tricode(uint8_t data, uint8_t index, uint16_t* tricode, const char* comment)
{
	uint8_t crc = 0x00;
	crc = _crc8_update(crc, data);
	crc = _crc8_update(crc, index);

	tricode[0] = MKINT16(0x00, MKBYTE(HINIB(crc), HINIB(data)));
	tricode[1] = MKINT16(0x01,                           index);
	tricode[2] = MKINT16(0x00, MKBYTE(LONIB(crc), LONIB(data)));

	#if (DEBUG_ENODING)
	printf("TriCode: data=%3d (0x%02x) index=%3d (0x%02x) %-10s -> ", data, data, index, index, comment);
	printf("%04x, %04x, %04x; \n", tricode[0], tricode[1], tricode[2]);
	#endif

	return 3;
}

int esp_make_datumcode(
	uint16_t* datumcode, // must be sized at least DATUMCODE_MAX!
	const char* apssid_str,
	const char* bssid_str,
	const char* passwd_str,
	const char* ownaddr_str,
	bool ishidden
)
{
	#if (DEBUG_ENODING)
	printf("------------------------------\n");
	printf("ESSID    = \"%s\"\n", apssid_str);
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

    // apssid_str and passwd_str could be encrypted here, using "AES/ECB/PKCS5Padding"
	// nice small AES implementation: https://github.com/kokke/tiny-AES-c/

	uint8_t apssid_len	= strlen(apssid_str);
	uint8_t apssid_crc	= _crc8_update_buf(0x00, apssid_str, apssid_len);

	uint8_t bssid_crc	= _crc8_update_buf(0x00, bssid_bytes, MAC_LEN);
	uint8_t passwd_len	= strlen(passwd_str);

	uint8_t total_len	=
		sizeof(total_len ) +
		sizeof(passwd_len) +
		sizeof(apssid_crc) +
		sizeof(bssid_crc ) +
		sizeof(total_xor ) +
		IPADDR_LEN +
		passwd_len +
		apssid_len;

	total_xor  = 0x00;
	total_xor ^= total_len;	
	total_xor ^= passwd_len;
	total_xor ^= apssid_crc;
	total_xor ^= bssid_crc;
	total_xor = _xor_update_buf(total_xor, ownaddr_bytes, IPADDR_LEN);
	total_xor = _xor_update_buf(total_xor, passwd_str, passwd_len);
	total_xor = _xor_update_buf(total_xor, apssid_str, apssid_len);	

	#if (DEBUG_ENODING)
	printf("apssid_crc = 0x%02x (%3d)\n", apssid_crc, apssid_crc);
	printf("bssid_crc  = 0x%02x (%3d)\n", bssid_crc, bssid_crc);
	printf("apssid_len = 0x%02x (%3d)\n", apssid_len, apssid_len);
	printf("passwd_len = 0x%02x (%3d)\n", passwd_len, passwd_len);
	printf("total_len  = 0x%02x (%3d)\n", total_len, total_len);
	printf("total_xor  = 0x%02x (%3d)\n", total_xor, total_xor);
	#endif
	
	memset(datumcode, 0, DATUMCODE_MAX);

	int n = 0; // datacode index
	int t = 0; // tricode index

	n += esp_tricode(total_len,  t++, datumcode + n, "total_len");  // 0
	n += esp_tricode(passwd_len, t++, datumcode + n, "passwd_len"); // 1
	n += esp_tricode(apssid_crc, t++, datumcode + n, "apssid_crc"); // 2
	n += esp_tricode(bssid_crc,  t++, datumcode + n, "bssid_crc");  // 3
	n += esp_tricode(total_xor,  t++, datumcode + n, "total_xor");  // 4

	for (int i=0; i<IPADDR_LEN; i++)
	{
		n += esp_tricode(ownaddr_bytes[i], t++, datumcode + n, "ipaddr");
	}
	
	for (int i=0; i<passwd_len; i++)
	{
		n += esp_tricode(passwd_str[i], t++, datumcode + n, "ap_pwd");
	}

	if (ishidden)
	{
		for (int i=0; i<apssid_len; i++)
		{
			n += esp_tricode(apssid_str[i], t++, datumcode + n, "apssid");
		}
	}

	for (int i=0; i<MAC_LEN; i++)
	{
		n += esp_tricode(bssid_bytes[i], t++, datumcode + n, "bssid");
	}

	#if (DEBUG_ENODING)
	printf("dcode_len  = %d\n", n);
	printf("------------------------------\n");
	#endif

	return n;
}

bool esp_peek_reply(void)
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
			// expecting here: buf[0] == strlen(apssid_str) + strlen(passwd_str)

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

void esp_send_guidecode(void)
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

		esp_peek_reply();
	}
}

void esp_send_datumcode(const uint16_t* datumcode, int dc_len)
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
            esp_peek_reply();
		}
	}
}

bool esp_smartcfg_init(const char* localaddr_str)
{
	printf("smartcfg_init ...\n");

	memset(packet, '1', sizeof(packet));
	sockets_init();
	
	txsock = _udpsock();
	sockopt_broadcast(txsock);
	memset(&txaddr, 0, sizeof(struct sockaddr_in));
	txaddr.sin_family = AF_INET;
	txaddr.sin_port	= htons(SMARTCFG_UDP_TX_PORT);
	txaddr.sin_addr.s_addr = INADDR_BROADCAST;

	rxsock = _udpsock();
	sockaddr_in_t localaddr;
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family		= AF_INET;
	localaddr.sin_addr.s_addr	= INADDR_ANY; // inet_addr(localaddr_str);
	localaddr.sin_port			= htons(SMARTCFG_UDP_RX_PORT);

	// bind to the local address
	if (bind(rxsock, (struct sockaddr *) &localaddr, sizeof(localaddr)) != 0)
	{
		print_socketerror("bind()");
		return false;
	}
	else
	{
		printf("socket bound to: %s:%u\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));
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
	const char* apssid_str,
	const char* bssid_str,
	const char* passwd_str,
	const char* ownaddr_str, // todo: eliminate this parameter: get own ip addr on interface wlan0
	bool ishidden)
{
	esp_smartcfg_init(ownaddr_str);

	uint16_t datumcode[DATUMCODE_MAX];
	int dc_len = esp_make_datumcode(datumcode, apssid_str, bssid_str, passwd_str, ownaddr_str, ishidden);

	unsigned long t_end = timestamp_ms() + TOTAL_TIMEOUT_MS;

    eprintf("Sending Packets: ");

	while (run && (timestamp_ms() < t_end))
	{
		esp_send_guidecode();
		esp_send_datumcode(datumcode, dc_len);
	}

	esp_smartcfg_deinit();
}
