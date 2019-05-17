A command line version of the ESPTouch SmartConfig App (https://github.com/EspressifApp/EsptouchForAndroid).
Tested on Raspberry.
Tested on Windows.
Implemented but not tested for hidden Wifis.

Build:
Build via `make`, then a subdirectory bin appears with the executeable.

Usage:

```
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
```

ToDos:

Use the own SSID, IP-Address instead of expecting it from the command line.
