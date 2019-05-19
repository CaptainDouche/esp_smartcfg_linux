A command line version of the ESPTouch SmartConfig App (https://github.com/EspressifApp/EsptouchForAndroid).
* Tested on Raspberry.
* Tested on Windows.
* Implemented but not tested for hidden Wifis.

Build:

Build via `make`, then a subdirectory `bin` appears with the executeable `smartcfg`.

Usage:

```
smartcfg ...
  [--essid ESSID]
  [--bssid BSSID]
  --password PASSWORD
  [ (--interface INTERFACENAME) | (--address LOCALADDRESS) ]
  [ (--timeout TIMEOUT) | --infinite ]
  [-hidden | -visible]

```

if `--essid`, `--bssid` and `--interface` is ommitted, then the locally connected Wifi is used.
The `--password` must always be given.

Example:

```
pi@raspiz1% ./bin/smartcfg --password "34234673423423"
./bin/smartcfg, ver: May 19 2019/15:20:49 ...
using local interface wlan0 address 192.168.0.141
using ssid MyWifiName (9c:c7:a6:9c:f7:84)
smartcfg_init ...
receiving socket bound to: 0.0.0.0:18266
sending packets: GDGDGDGDGDGD
recvd 11 bytes from 192.168.0.145
- IP:  192.168.0.145
- MAC: a0:20:a6:14:e5:04

smartcfg_deinit ...
./bin/smartcfg: done

```
