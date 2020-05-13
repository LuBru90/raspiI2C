#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <curl/curl.h>
#include <string.h>


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
float getMoisture(int meancount, int meandelay){
    int masterDev = i2cInit();
    int16_t result;     // 16bit adc result
    uint8_t buffer[10];
    float valuesum = 0;
    float meanvalue = 0;
    int counter = 0;

    // init connection
    if (ioctl(masterDev, I2C_SLAVE, MOISTURE_I2C_ADDRESS) < 0){
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
    meanvalue = valuesum/meancount;
    meanvalue = (1 - (meanvalue - 1.063)/(2.782 - 1.063)) * 100;

    return meanvalue;
}

uint8_t* sendAndRead(uint8_t* transmit, uint8_t* receive, int len){
    int masterDev = i2cInit();

    // init connection
    if (ioctl(masterDev, I2C_SLAVE, BME280_I2C_ADDRESS) < 0){
        printf("Error: Couldn't find device on address!\n");
        //return (float)1;
    }

    // write to i2c bus
    if (write(masterDev, transmit, len) != len){
        perror("Write to register 1");
        exit(-1);
    }
    // response
    //usleep(100);
    //uint8_t receive[10] = {0};
    if (read(masterDev, receive, 8) != 8){
        perror("Read conversion");
        exit(-1);
    }
    printf("TX: %i %i %i\n", transmit[0], transmit[1], transmit[2]);
    printf("RX: ");
    for(int i = 0; i < 8; i++){
       printf("%i ", receive[i]);
       receive[i] = 0;
    }
    printf("\n");
    printf("--------\n");
    
    return receive; 
    //result = (int16_t)buffer[0]*256 + (uint16_t)buffer[1];
    //convResult += (float)result * 4.096/32768.0;

}

// TODO: does not work :(
float getBME280(){
    int masterDev = i2cInit();
    int16_t result;     // 16bit adc result
    uint8_t transmit[10] = {0};
    uint8_t receive[10] = {0};
    float convResult;

    // init connection
    if (ioctl(masterDev, I2C_SLAVE, BME280_I2C_ADDRESS) < 0){
        printf("Error: Couldn't find device on address!\n");
        return (float)1;
    }

    // transmit[0] = 0xd0; // get device id (should be 0x60)
    // set mode
    transmit[0] = 0xf4; // config register
    transmit[1] = 0x3;  // set force mode
    // write to i2c bus
    if (write(masterDev, transmit, 2) != 2){
        perror("Write to register 1");
        exit(-1);
    }
    // response
    if (read(masterDev, receive, 2) != 2){
        perror("Read conversion");
        exit(-1);
    }
    printf("TX: %i %i %i\n", transmit[0], transmit[1], transmit[2]);
    printf("RX: %i %i %i\n", receive[0], receive[1], receive[2]);
 
    //result = (int16_t)buffer[0]*256 + (uint16_t)buffer[1];
    //convResult += (float)result * 4.096/32768.0;
    return 1;
}

void print(char* string){
    printf("%s\n", string);
}

int main(){
    float result;

    result = getMoisture(10, 1000);
    printf("Moisture: %0.3f %\n", result);
    //post2thingspeak(result);

    return 0;
}

