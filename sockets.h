#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#if (defined _WIN32)
	#include <Windows.h>
	#include <winsock.h>
	#include <winsock2.h>
	// # pragma comment(lib, "wsock32.lib")
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
#endif


#if (defined _WIN32)
	typedef struct sockaddr 	sockaddr_t;
	typedef struct sockaddr_in 	sockaddr_in_t;
	typedef struct in_addr 		in_addr_t;
	typedef int socklen_t;
#elif (defined _LINUX)
	typedef int SOCKET;
	typedef struct sockaddr 	sockaddr_t;
	typedef struct sockaddr_in	sockaddr_in_t;
	//typedef struct in_addr 	in_addr_t;	
	#define closesocket(S)		close(S)
	#define INVALID_SOCKET		-1
#endif

#ifndef HOSTNAME_MAX
	#define HOSTNAME_MAX	64
#endif

#ifndef IW_ESSID_MAX_SIZE
	#define IW_ESSID_MAX_SIZE	64
#endif

#define MAC_LEN						6
#define IPADDR_LEN					4


// windows: wifi infos via 
// C:> netsh wlan show interfaces

// linux: wifi infos via
// $ sudo ifconfig

bool parse_ipstr(const char* str, uint8_t* ip);
bool parse_macstr(const char* str, uint8_t* mac);
void print_socketerror(const char* msg);
bool sockets_init(void);
void sockets_deinit(void);
bool sockopt_broadcast(SOCKET sock);
bool sockopt_blocking(SOCKET sock, bool blocking);
SOCKET udpsock(void);

void setup_addr_in(sockaddr_in_t* addr, const char* addrstr, unsigned short port);

#define setup_addr_in_any(addr, port)			setup_addr_in(addr, "0.0.0.0", port)
#define setup_addr_in_broadcast(addr, port)		setup_addr_in(addr, "255.255.255.255", port)

#define STREQU(STR1, STR2)						(strcmp(STR1, STR2) == 0)
#define eprintf(...)							fprintf (stderr, __VA_ARGS__)

int dump_own_addrs(void);

int get_ifc_addr(const char* ifname, char* ipstr);
int wifi_get_ssid(const char* ifname, char* essid, uint8_t* bssid);

#endif // #ifndef SOCKETS_H
