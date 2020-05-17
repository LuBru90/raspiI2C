# raspiI2C
### Cronejob:
'''
crontab -e
Add: 0 0-23 * * * /main
Execute main every hour X:00
'''

### TODO:
- *Split programm in to three parts:*
    1. Read ADS1115
    2. Read SHT21
    3. Send to Thingspeak 
- *Only turn on sensors before measuring*

### Sources:
- *BME280*:
    - https://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BME280-DS002.pdf

- *SHT21*:
    - https://olegkutkov.me/2018/02/21/htu21d-raspberry-pi/    
    - http://www.farnell.com/datasheets/1780639.pdf

- *Thingspeak*:
    - https://thingspeak.com/channels/1056994

