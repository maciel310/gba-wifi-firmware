# gba-wifi-firmware
Firmware for my WiFi adapter board for the GBA.

## Usage
To use the adapter you will either need to use a GameBoy Color style link cable and attach it to the PCB connector, or solder on a male Link Port connector to the empty header on the board (designed for one taken from a [GBA to GC cable](https://www.ebay.com/sch/i.html?_from=R40&_trksid=m570.l1313&_nkw=gba+to+gc+cable&_sacat=0)). Depending on your cable you may also need to supply power through the MicroUSB port.

Once connected and powered on you should see a GbaWiFi network appear. Connect to that network and enter your WiFi credentials on the WiFi login screen. The GbaWiFi network should then disappear and the board will connect to your WiFi network directly. If you have mDNS configured on your system then you should be able to go to http://gba.local/ and access the UI to push new firmware. If not, you can go into your router's configuration to find the device's IP address.

### Faster iteration using curl
You can skip the Web upload UI by using CURL to push new firmware versions.

```
make && curl -F 'name=@./gba_mb.gba' http://gba.local/upload
```

Just update `gba_mb.gba` with the output of your build step, and possibly replace `gba.local` with the IP address of your module.

### Faster iteration using HardReset
If you look at the gba-wifi-welcome ROM you'll see the `serial_init` and `handle_serial` methods. These implement logic which automatically HardResets the GBA after an upload completes. A HardReset takes you back to the GBA boot screen, which will then load the new ROM version. This is particularly useful when powering the WiFi adapter directly from the GBA, as it avoids power-cycling the adapter and the sometimes lengthy WiFi connection process.

Combined with the above CURL command you have a setup which has your ROM running on hardware within seconds of compiling it.


## Firmware Updating
The firmware can be updated using the Arduino IDE. Configure the [ESP8266 Core](https://github.com/esp8266/Arduino), and under `Tools > Board` select "Node MCU 1.0 (ESP-12E module)". If you don't see a device under `Tools > Port` then you may need to [install the CH340 driver](https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/all).

## Troubleshooting
#### The GbaWiFi network appears even after configuring it successfully.
Sometimes the module won't be able to successfully connect to your WiFi network, and when that happens it falls back to the configuration mode. If this happens frequently you can flash a firmware version with your WiFi credentials hard-coded, which avoids this situation. Just uncomment the `WIFI_SSID` and `WIFI_PASS` #defines, set the values, and flash the firmware.



## Future Enhancements

Below are enhancements which would be nice to have, but aren't necessarily planned. Pull requests implementing these features are welcome!

* Implement multiboot for GBA cables. This will probably need to be a separate firmware implementation, and may require bit-banging the protocol (instead of using the SPI hardware).
