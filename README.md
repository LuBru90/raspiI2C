# Raspberry Pi I2C | SHT21 | ADS1115
### How to use:
Get API key from [thingspeak](https://www.thingspeak.com) and edit **main.c**

```
strcpy(message, "https://api.thingspeak.com/update?api_key=XXXXXXXXXXXXXX");
```
**Compile main.c with Makefile:**
```
make
```
**Execute main every hour with a cronjob:**
```
crontab -e
```
Add the following line:
```
0 0-23 * * * mypath/main
```

### TODO:
- Split programm in to three parts:
    - Read ADS1115
    - Read SHT21
    - Send to Thingspeak 
- Only turn on sensors before measuring

### Sources:
- ADS1115:
    - http://www.ti.com/lit/ds/symlink/ads1114.pdf?&ts=1589711465834
- SHT21:
    - https://olegkutkov.me/2018/02/21/htu21d-raspberry-pi/    
    - http://www.farnell.com/datasheets/1780639.pdf
- Thingspeak:
    - https://thingspeak.com/channels/1056994
- BME280:
    - https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf


