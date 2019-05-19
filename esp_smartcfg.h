#ifndef ESP_SMARTCFG_H
#define ESP_SMARTCFG_H

#include <stdint.h>
#include <stdbool.h>

// windows: wifi infos via 
// C:> netsh wlan show interfaces

// linux: wifi infos via
// $ sudo ifconfig

void esp_smartcfg_run(
	const char* essid_str,
	const char* bssid_str,
	const char* passwd_str,
	const char* ownaddr_str,
	bool ishidden,
	int timeout);

#endif // #ifndef SMARTCFG_H
