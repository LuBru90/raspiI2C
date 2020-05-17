# Raspberry Pi 3 b+ | C | I2C | SHT21 | ADS1115 | Thingspeak
## About this project
In this project two i2c sensors are used to measure the room temperature, rel. humidity
brightness and the moisture in the soil of a plant.  
The used i2c sensors are the **SHT21 (temperature and humidity)** and the **ADS1115
(brightness and soil moisture).**
A cronjob is used to send the obtained data every hour to thingspeak.
S. also Sources.

### How to use:
Get API key from your [thingspeak](https://www.thingspeak.com)-channel.  
Create a file named **apikey.key** and paste your API key in there.

**Compile main.c with Makefile:**
```
make
```
**Execute main every hour with a cronjob:**  
Open list of cronjobs with the following command:
```
crontab -e
```
Add the following line and modifie **mypath**:
```
0 0-23 * * * cd /mypath && ./main
```

### Test SHT21 with Python:
To test the SHT21 sensor you can use the python script in the pysht folder:
```
python pysht/readsht.py
```

### TODO:
- Documentation: How to connect sensors to the raspberry pi
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

