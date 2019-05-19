#include "sockets.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if (defined _WIN32)
	#include <Windows.h>
	#include <winsock.h>
	#include <winsock2.h>
	// # pragma comment(lib, "wsock32.lib")
#elif (defined _LINUX)
	#define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */	
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <ifaddrs.h>
	#include <linux/if_link.h>
	#include <linux/wireless.h>
#endif

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

void setup_addr_in(sockaddr_in_t* addr, const char* addrstr, unsigned short port)
{
	memset(addr, 0, sizeof(sockaddr_in_t));
	addr->sin_family = AF_INET;
	addr->sin_port	= htons(port);
	addr->sin_addr.s_addr = inet_addr(addrstr); // INADDR_ANY / INADDR_BROADCAST
}

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

int get_ifc_addr(const char* ifname, char* ipstr)
{
#if (defined _WIN32)
	// TODO ...
#elif (defined _LINUX)
	// http://man7.org/linux/man-pages/man3/getifaddrs.3.html
	*ipstr = '\0';
	
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) 
	{
	   perror("getifaddrs");
	   return -1;
	}

	int res = -1;
	
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (ifa->ifa_addr == NULL) continue;
		
		if ((ifa->ifa_addr->sa_family == AF_INET) && STREQU(ifa->ifa_name, ifname))
		{			
			int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ipstr, sizeof("111.222.333.444"), NULL, 0, NI_NUMERICHOST);
			if (s != 0) 
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
			}
			else
			{
				//printf("%s: address: <%s>\n", ifa->ifa_name, ipstr);
				res = 0;				
			}
			break;
		}
	}

	freeifaddrs(ifaddr);
	
	return res;
#endif
}

int dump_own_addrs(void)
{
#if (defined _WIN32)
	char name[HOSTNAME_MAX + 1];

	if (gethostname(name, sizeof(name)) == 0)
	{
		int i = 0;
		struct hostent* hostinfo;

		printf("IP addresses of %s:\n", name);

		if ((hostinfo = gethostbyname(name)) != NULL)
		{
			while (hostinfo->h_addr_list[i])
			{
				char* ip = inet_ntoa(*(struct in_addr *)hostinfo->h_addr_list[i]);
				printf("%s\n", ip);
				i++;
			}
		}
		
		return i;
	}
	else
	{
		print_socketerror("gethostname()");
		return -1;
	}
#elif (defined _LINUX)
	// http://man7.org/linux/man-pages/man3/getifaddrs.3.html
	
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) 
	{
	   perror("getifaddrs");
	   return -1;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
	{
		if (ifa->ifa_addr == NULL) continue;
		
		int family = ifa->ifa_addr->sa_family;
		
		char host[NI_MAXHOST];
		if (family == AF_INET)
		{	
			int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) 
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				break;
			}

			printf("%-8s %s\n", ifa->ifa_name, host);
		}
		else
		if (family == AF_INET6)
		{	
			int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) 
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				break;
			}

			printf("%-8s %s\n", ifa->ifa_name, host);
		}	
	}

	freeifaddrs(ifaddr);
	
	return 0;
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

SOCKET udpsock(void)
{
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (sock == INVALID_SOCKET)
	{
		print_socketerror("socket() failed");
		return INVALID_SOCKET;
	}

	return sock;
}

int wifi_get_ssid(const char* ifname, char* essid, uint8_t* bssid)
{
#if (defined _WIN32)
	// TODO ...
#elif (defined _LINUX)
	int res = -1;
	struct iwreq req;
	strncpy(req.ifr_ifrn.ifrn_name, ifname, IFNAMSIZ);

	int fd, status;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	char* buffer;
	buffer = calloc(32, sizeof(char));
	req.u.essid.pointer = buffer;
	req.u.essid.length = 32;
	if (ioctl(fd, SIOCGIWESSID, &req) < 0) 
	{
		eprintf(stderr, "Failed get ESSID on interface %s: %s\n", ifname, strerror(errno));
	} 
	else 
	{
		//printf("%s ESSID: %s", ifname, (char*)req.u.essid.pointer);
		strncpy(essid, (char*)req.u.essid.pointer, IW_ESSID_MAX_SIZE);
		essid[IW_ESSID_MAX_SIZE] = '\0';
		res = 0;
	}
	
	if (bssid)
	{
		memset(bssid, 0x00, MAC_LEN);
		
		struct iwreq iwr;
		memset(&iwr, 0, sizeof(iwr));
		strncpy(iwr.ifr_name, ifname, IFNAMSIZ);

		if (ioctl(fd, SIOCGIWAP, &iwr) < 0) 
		{
			perror("ioctl[SIOCGIWAP]");
		}
		else
		{
			memcpy(bssid, iwr.u.ap_addr.sa_data, MAC_LEN);
			//printf("%s BSSID: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx \n", ifname, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
		}
	}
	
	free(buffer);
	return res;
#endif	
}

