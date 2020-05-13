#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <curl/curl.h>
#include <string.h>

#include <wiringPiI2C.h>
#include <wiringPi.h>

#define BME280_I2C_ADDRESS 0x76
#define MOISTURE_I2C_ADDRESS 0x48
#define TEST_I2C_ADDRESS 0x68

#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47


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


short read_raw_data(int fd, int addr){
    short high_byte,low_byte,value;
    high_byte = wiringPiI2CReadReg8(fd, addr);
    low_byte = wiringPiI2CReadReg8(fd, addr+1);
    value = (high_byte << 8) | low_byte;
    return value;
}


void MPU6050_Init(int fd){
    wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);    /* Write to sample rate register */    
    wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);    /* Write to power management register */
    wiringPiI2CWriteReg8 (fd, CONFIG, 0);       /* Write to Configuration register */
    wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24); /* Write to Gyro Configuration register */
    wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);    /*Write to interrupt enable register */ 
}


int main(){
    float result =  getMoisture(10, 100);
    printf("Result: %f %\n", result);
    //post2thingspeak(result);
    
    int fd = wiringPiI2CSetup(TEST_I2C_ADDRESS);
    MPU6050_Init(fd);
    short data = read_raw_data(fd, ACCEL_XOUT_H);
    float Ax = data/16384.0;
    printf("%.3f\n", Ax);

    return 0;
}

