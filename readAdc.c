#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <curl/curl.h>
#include <string.h>

// ADS1115
#define ADS1115_CHANNEL_0 0xC3
#define ADS1115_CHANNEL_1 0xD3
#define ADS1115_CHANNEL_2 0xE3
#define ADS1115_CHANNEL_3 0xF3

#define MOISTURE_I2C_ADDRESS 0x48
#define BME280_I2C_ADDRESS 0x76


int i2cInit(){
    // Check if on board i2c device is available (bcm2708)
    int masterDev;
    if ((masterDev = open("/dev/i2c-1", O_RDWR)) < 0){
        printf("Error: Couldn't open device! %d\n", masterDev);
        return 1;
    }
    return masterDev;
}

void print(char* string){
    printf("%s\n", string);
}

// TODO: add field selection - for value in values: append fieldx
void post2thingspeak(float value){
    CURL *curl = curl_easy_init();
    if(curl){
        char value2char[10];
        char message[100];
        // convert float to char array and append to message
        sprintf(value2char, "%f", value);
        strcpy(message, "https://api.thingspeak.com/update?api_key=7WM9WPRIQDV1FW0S&field1=");
        strcat(message, value2char);

        printf("http GET: %s\n", message);

        // send http GET with curl
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, message);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}


// TODO: add channel selection - a1 to a4
float getVoltge(int meancount, int meandelay, int channel){
    int masterDev = i2cInit();
    int16_t result;     // 16bit adc result
    uint8_t buffer[10];
    float valuesum = 0;
    float voltage = 0;
    int counter = 0;

    // init connection
    if (ioctl(masterDev, I2C_SLAVE, MOISTURE_I2C_ADDRESS) < 0){
        printf("Error: Couldn't find device on address!\n");
        return (float)1;
    }

    for(int i = 0; i < meancount; i++){
        // set config register and start conversion
        buffer[0] = 0x01;   // set address pointer - 1 == config register
        buffer[1] = channel;   // set 1. config register [6:15]
        buffer[2] = 0x85;   // set 2. config register [0:5]

        // write to i2c bus
        if (write(masterDev, buffer, 3) != 3){
            perror("Write to register 1");
            exit(-1);
        }

        // wait for conversion to complete (msb == 0)
        do {
            if (read(masterDev, buffer, 2) != 2) {
                perror("Read conversion");
                exit(-1);
            }
        } while (!(buffer[0] & 0x80));

        buffer[0] = 0;
        if (write(masterDev, buffer, 1) != 1){
            perror("Write register select");
            exit(-1);
        }

        // read result from register
        if (read(masterDev, buffer, 2) != 2){
            perror("Read conversion");
            exit(-1);
        }

        // convert to int - lower + upper 
        result = (int16_t)buffer[0]*256 + (uint16_t)buffer[1];

        counter ++;
        valuesum += (float)result * 4.096/32768.0;
        usleep(meandelay);
    }
    close(masterDev);

    // calculate mean value of moisture
    voltage = valuesum/meancount;
    //voltage = (1 - (voltage - 1.063)/(2.782 - 1.063)) * 100;

    return voltage;
}

float convertU2Moist(float voltage){
    return voltage = (1 - (voltage - 1.063)/(2.782 - 1.063)) * 100;
}

int main(){
    float result;

    result = getVoltge(10, 1000, ADS1115_CHANNEL_1);
    result = convertU2Moist(result);
    printf("Moisture: %0.3f %\n", result);
    
    result = getVoltge(10, 1000, ADS1115_CHANNEL_0);
    printf("Voltage: %0.3f V\n", result);
    //post2thingspeak(result);

    return 0;
}
