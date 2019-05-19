#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#include "sockets.h"
#include "esp_smartcfg.h"

#define COMPILE_TIMESTAMP					__DATE__ "/" __TIME__

volatile bool run = true;

void onsignal(int signum)
{
	printf("\n\n***received process signal %d ***\n", signum);
	run = false;
}

void usage(const char* progname)
{
	printf("Usage: %s ...\n", progname);
#if (defined _WIN32)
	printf("  --essid ESSID\n");
	printf("  --bssid BSSID\n");
	printf("  --address LOCALADDRESS)\n");
#elif (defined _LINUX)
	printf("  [ --essid ESSID ]\n");
	printf("  [ --bssid BSSID ]\n");
	printf("  [ (--interface INTERFACENAME) | (--address LOCALADDRESS) ]\n");
#endif
	printf("  --password PASSWORD\n");
	printf("  [ (--timeout TIMEOUT) | --infinite ]\n");
	printf("  [ --hidden | --visible ] \n");	

	//dump_own_addrs();
	printf("Version: " COMPILE_TIMESTAMP " \n");
}

const char* essid   = NULL;
const char* bssid   = NULL;
const char* passwd  = NULL;
const char* myip    = NULL;
const char* ownaddr = NULL;
const char* ifname  = "wlan0";
int   timeout       = 0;

bool hidden = false;

bool scanargs(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		char* arg = argv[i];

		#define ARG_EQU(SHORTARG, LONGARG)  (STREQU(arg, "-"SHORTARG) || STREQU(arg, "--"LONGARG))

		if ARG_EQU("e", "essid")
		{
			i++;
			essid = argv[i];
		}
		else if ARG_EQU("b", "bssid")
		{
			i++;
			bssid = argv[i];
		}			
		else if ARG_EQU("p", "password")
		{
			i++;
			passwd = argv[i];
		}			
		else if ARG_EQU("ifc", "interface")
		{
			i++;
			ifname = argv[i];
		}
		else if ARG_EQU("a", "address")
		{
			i++;
			ownaddr = argv[i];
		}		
		else if ARG_EQU("t", "timeout")
		{
			i++;
			timeout = atoi(argv[i]);
		}
		else if ARG_EQU("hid", "hidden")
		{
			hidden = true;
		}
		else if ARG_EQU("vis", "visible")
		{
			hidden = false;
		}		
		else if ARG_EQU("inf", "infinite")
		{
			timeout = 0;
		}
		else if ARG_EQU("h", "help")
		{
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
		else
		{
			eprintf("unrecognized argument: %s\n", arg);
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char* argv[])
{
	printf("%s, ver: " COMPILE_TIMESTAMP " ...\n", argv[0]);
	
	scanargs(argc, argv);

	signal(SIGINT,  onsignal);
	signal(SIGTERM, onsignal);
	
#if (defined _LINUX)
	if (ownaddr == NULL)
	{
		static char ownaddr_buf[sizeof("111.222.333.444") + 1] = "";
		if (get_ifc_addr(ifname, ownaddr_buf))
		{
			eprintf("cant get address of local interface %s.\n", ifname);
			return EXIT_FAILURE;
		}
		else
		{
			ownaddr = ownaddr_buf;
			printf("using local interface %s address %s\n", ifname, ownaddr);
		}
	}
	
	if (bssid == NULL)
	{
		static char essid_buf[IW_ESSID_MAX_SIZE + 1] = "";
		static char bssid_buf[MAC_LEN * sizeof("xx:") + 1] = "";		

		uint8_t mac[MAC_LEN];
		wifi_get_ssid(ifname, essid_buf, mac);
		snprintf(bssid_buf, sizeof(bssid_buf), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		essid = essid_buf;
		bssid = bssid_buf;
		
		printf("using ssid %s (%s)\n", essid, bssid);
	}
#endif

	esp_smartcfg_run(essid, bssid, passwd, ownaddr, hidden, timeout);
	
	printf("%s: done\n", argv[0]);	

	return EXIT_SUCCESS;
}
