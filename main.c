/*

pi@raspiz1% ./bin/smartcfg "MyWifName" "9c:c7:a6:9c:f7:84" "MySecretPassword" `hostname -I`
./bin/smartcfg begin...
smartcfg_init ...
socket bound to: 0.0.0.0:18266
Sending Packets: GDGDGDG
recvd 11 bytes from 192.168.0.99.
- IP:  160.32.166.166
- MAC: e5:4:c0:a8:0:63
D
smartcfg_deinit ...

./bin/smartcfg end.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

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
	printf("Usage: %s ESSID BSSID PASSWORD MYIP [-hidden] \n", progname);
	printf("Version: " COMPILE_TIMESTAMP " \n");
}

int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}	
	
    printf("%s begin (built: %s)\n", argv[0], COMPILE_TIMESTAMP);

	signal(SIGINT,  onsignal);
	signal(SIGTERM, onsignal);

    const char* essid  = argv[1];
    const char* bssid  = argv[2];
    const char* passwd = argv[3];
    const char* myip   = argv[4];
    bool hidden = (argc > 5) && !strstr(argv[5], "-hidden");

	esp_smartcfg_run(essid, bssid, passwd, myip, hidden);

	printf("\n%s end.\n", argv[0]);

	return EXIT_SUCCESS;
}
