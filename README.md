A command line version of the ESPTouch SmartConfig App (https://github.com/EspressifApp/EsptouchForAndroid).
* Tested on Raspberry.
* Tested on Windows.
* Implemented but not tested for hidden Wifis.

Build:

Build via `make`, then a subdirectory `bin` appears with the executeable `smartcfg`.

Usage:

```
pi@raspiz1% ./bin/smartcfg "MyWifName" "9c:c7:a6:9c:f7:84" "MySecretPassword" `hostname -I`
./bin/smartcfg begin...
smartcfg_init ...
socket bound to: 0.0.0.0:18266
Sending Packets: GDGDGDG
recvd 11 bytes from 192.168.0.145.
- IP:  192.168.0.145
- MAC: a0:20:a6:14:e5:04
D
smartcfg_deinit ...

./bin/smartcfg end.
```

ToDos:

Use the own SSID, IP-Address instead of expecting it from the command line.
