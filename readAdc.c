#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <curl/curl.h>
#include <string.h>


#define MOISTURE_I2C_ADDRESS 0x48





int post2thingspeak(float value){
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
    return 0;
}


float getMoisture(int deviceFile_i2c, int meancount){
    int16_t result;     // 16bit adc result
    uint8_t buffer[10];
    float valuesum = 0;
    float meanvalue = 0;
    int counter = 0;

    // init connection
    if (ioctl(deviceFile_i2c, I2C_SLAVE, MOISTURE_I2C_ADDRESS) < 0){
        printf("Error: Couldn't find device on address!\n");
        return (float)1;
    }

    for(int i = 0; i < meancount; i++){
        // set config register and start conversion
        buffer[0] = 0x01;   // set address pointer - 1 == config register
        buffer[1] = 0xc3;   // set 1. config register [0:5]
        buffer[2] = 0x85;   // set 2. config register [6:15]
        // 0x85 - 0xc3 = 1000 0101 - 1100 0011

        // write to i2c bus
        if (write(deviceFile_i2c, buffer, 3) != 3){
            perror("Write to register 1");
            exit(-1);
        }

        // wait for conversion to complete (msb == 0)
        do {
            if (read(deviceFile_i2c, buffer, 3) != 3) {
                perror("Read conversion");
                exit(-1);
            }
        } while (!(buffer[0] & 0x80));

        buffer[0] = 0;
        if (write(deviceFile_i2c, buffer, 1) != 1){
            perror("Write register select");
            exit(-1);
        }

        // read result from register
        if (read(deviceFile_i2c, buffer, 2) != 2){
            perror("Read conversion");
            exit(-1);
        }

        // convert to int - lower + upper 
        result = (int16_t)buffer[0]*256 + (uint16_t)buffer[1];

        counter ++;
        valuesum += (float)result * 4.096/32768.0;
        usleep(10 * 1000);
    }
    close(deviceFile_i2c);

    // calculate mean value of moisture
    meanvalue = valuesum/meancount;
    meanvalue = (1 - (meanvalue - 1.063)/(2.782 - 1.063)) * 100;

    return meanvalue;
}


int main() {
    int deviceFile_i2c;
    float moisture = 0;

    // Check if on board i2c device is available (bcm2708)
    if ((deviceFile_i2c = open("/dev/i2c-1", O_RDWR)) < 0){
        printf("Error: Couldn't open device! %d\n", deviceFile_i2c);
        return 1;
    }

    moisture = getMoisture(deviceFile_i2c, 10);
    printf("Moisture: %0.3f %\n", moisture);
    post2thingspeak(moisture);

    return 0;
}

