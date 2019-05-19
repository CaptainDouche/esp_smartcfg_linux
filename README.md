__About:__

This is a command line version of the ESPTouch SmartConfig App (https://github.com/EspressifApp/EsptouchForAndroid).
* Works on Raspberry.
* Works on Windows.
* Implemented but not tested for hidden Wifis.
* AES is not implemented, however it is uncertain how that is used at all - i cant figure out from the App's source where the AES-key is handled.

__Build:__

Build via `make`, then a local subdirectory `./bin` appears with the executeable `smartcfg`.

__Usage:__
```
smartcfg ...
  [--essid ESSID]
  [--bssid BSSID]
  --password PASSWORD
  [ (--interface INTERFACENAME) | (--address LOCALADDRESS) ]
  [ (--timeout TIMEOUT) | --infinite ]
  [-hidden | -visible]

```

* If `--interface` is ommitted, then `wlan0` is assumed.  
* If `--address` is ommitted, then the local address of the interface is used.  
* If `--essid` and `--bssid` are ommitted, then the locally connected Wifi is used. You will typically _only_ provide these parameters, if you want to put your ESP into a _different_ wifi of the one, your Raspi is currently booked in. Of course this requires providing the correct password of this wifi.
* The `--password` must always be given.  

__Example:__
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
