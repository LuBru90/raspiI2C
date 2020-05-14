#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <curl/curl.h>
#include <string.h>

// ADS1115 konstants
#define ADS1115_CHANNEL_0 0xC3
#define ADS1115_CHANNEL_1 0xD3
#define ADS1115_CHANNEL_2 0xE3
#define ADS1115_CHANNEL_3 0xF3
#define ADS1115_ADDR 0x48

// BME280 konstants
#define BME280_I2C_ADDRESS 0x76 // Broken :(


// init on board i2c device (BCM2708)
int i2cInit(){
    int masterDev;
    if ((masterDev = open("/dev/i2c-1", O_RDWR)) < 0){
        printf("Error: Couldn't open device! %d\n", masterDev);
        return 1;
    }
    return masterDev;
}

// helper function
void print(char* string){
    printf("%s\n", string);
}

// send data to thingspeak
int post2thingspeak(float* values, int size){
    CURL *curl = curl_easy_init();
    if(curl){
        char value2char[10];
        char message[100];
        char num[1];

        // convert float to char array and append to message
        strcpy(message, "https://api.thingspeak.com/update?api_key=7WM9WPRIQDV1FW0S");
        if(size > 1){
            // append fields
            for(int i = 0; i < size; i++){
                strcat(message, "&field");
                sprintf(num, "%d", i + 1);
                strcat(message, num);
                strcat(message, "=");
                sprintf(value2char, "%.3f", values[i]);
                strcat(message, value2char);
            }
        } else{
            strcat(message, "&field1=");
            sprintf(value2char, "%f", values[0]);
            strcat(message, value2char);
        }
        printf("http GET: %s\n", message);
        // send http GET with curl
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, message);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

// get data from ADS1115
float getVoltage(int meancount, int meandelay, int channel){
    int masterDev = i2cInit();
    int16_t result;          // 16bit adc result
    uint8_t buffer[10];      // transmit and receive buffer
    float valuesum = 0;      // mean value sum

    // init connection
    if (ioctl(masterDev, I2C_SLAVE, ADS1115_ADDR) < 0){
        printf("Error: Couldn't find device on address!\n");
        return (float)1;
    }
    
    // measure multiple times and calculate the mean value to reduce noise
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

        valuesum += (float)result * 4.096/32768.0;
        usleep(meandelay);
    }
    close(masterDev);

    // calculate mean value of voltage and return
    return valuesum/meancount;
}

// Convert measured voltage from ADS1115 in % moisture
float convertU2Moist(float voltage){
    return voltage = (1 - (voltage - 1.063)/(2.782 - 1.063)) * 100;
}

int main(){
    float results[2] = {0};
    
    // get data from i2c device
    results[0] = getVoltage(10, 1000, ADS1115_CHANNEL_1);
    results[0] = convertU2Moist(results[0]);
    printf("Moisture: %0.3f %\n", results[0]);

    results[1] = getVoltage(10, 1000, ADS1115_CHANNEL_0);
    printf("Voltage: %0.3f V\n", results[1]);

    // send data to thingspeak
    int size = sizeof(results)/sizeof(results[0]);
    post2thingspeak(results, size);

    return 0;
}
