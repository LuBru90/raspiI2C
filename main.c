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

// SHT21 konstants
#define SHT21_ADDR 0x40


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
        char message[200];
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

        // Send http GET with curl
        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_URL, message);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
}

// Get data from ADS1115
float getVoltage(int meancount, int meandelay, int channel){
    int masterDev = i2cInit();
    int16_t result;          // 16bit adc result
    uint8_t buffer[10];      // transmit and receive buffer
    float valuesum = 0;      // mean value sum
    
    // Init connection
    if (ioctl(masterDev, I2C_SLAVE, ADS1115_ADDR) < 0){
        printf("Error: Couldn't find device on address!\n");
        return (float)1;
    }
    
    // Measure multiple times and calculate the mean value to reduce noise
    for(int i = 0; i < meancount; i++){
        // set config register and start conversion
        buffer[0] = 0x01;   // set address pointer - 1 == config register
        buffer[1] = channel;   // set 1. config register [6:15]
        buffer[2] = 0x85;   // set 2. config register [0:5]

        // Write to i2c bus
        if (write(masterDev, buffer, 3) != 3){
            perror("Write to register 1");
            exit(-1);
        }

        // Wait for conversion to complete (msb == 0)
        do {
            if (read(masterDev, buffer, 2) != 2) {
                perror("Read conversion");
                exit(-1);
            }
        } while (!(buffer[0] & 0x80));

        // Read data
        buffer[0] = 0;      
        if (write(masterDev, buffer, 1) != 1){
            perror("Write register select");
            exit(-1);
        }

        // Read result from register
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

    // Calculate mean value of voltage and return
    return valuesum/meancount;
}

// Convert measured voltage from ADS1115 in % moisture
float convertU2Moist(float voltage){
    return voltage = (1 - (voltage - 1.063)/(2.782 - 1.063)) * 100;
}

// Helper function
void getSHT21Register(int masterDev, uint8_t transmit, uint8_t *receive){
    write(masterDev, &transmit, 1);
    int counter = 0;
    while (1){
        usleep(50000);
        if(read(masterDev, receive, 3) != 3){
            if(counter >= 5){
                break;
            }
        }
        counter ++;
        continue;
    }
} 

// get data from sht21: Temperature and relative humidity
void getSHT21Data(float *sht21Data){
    int masterDev = i2cInit();  // file descriptor
    uint8_t receive[5] = {0};

    // Init connection
    if (ioctl(masterDev, I2C_SLAVE, SHT21_ADDR) < 0){
        printf("Error: Couldn't find device on address!\n");
    }
    
    getSHT21Register(masterDev, 0xE3, receive); // trigger temperature measurement

    /* Convert data temperature (14 bit):
       xxxx xxxx << 8 = xxxx xxxx 0000 0000
       xxxx xxxx 0000 0000 | yyyy yyyy = xxxx xxxx yyyy yyyy
       xxxx xxxx yyyy yyyy & 0xFFFC = xxxx xxxx yyyy yy00
       Note: 3. receive[2] => Checksum (not used)
    */
    float data = (uint16_t)((receive[0] << 8 | receive[1]) & 0xFFFC);
    float sensor_tmp = (float) data / 65536.0;
    sht21Data[0] = -46.85 + (175.72 * sensor_tmp);  // Temperature
 
    getSHT21Register(masterDev, 0xE5, receive); // trigger humidity measurement
    close(masterDev);

    data = (uint16_t)((receive[0] << 8 | receive[1]) & 0xFFF0);
    sensor_tmp = (float) data / 65536.0;
    sht21Data[1] = -6.0 + (125.0 * sensor_tmp);     // Humidity
}


int main(){
    float results[4] = {0};
    
    // ADS1115: Get voltages of channel 0 (Moisture) and 1 (Solarcell)
    float moistureVoltage = getVoltage(10, 1000, ADS1115_CHANNEL_1);
    results[0] = convertU2Moist(moistureVoltage);           // Moisture in %
    results[1] = getVoltage(10, 1000, ADS1115_CHANNEL_0);   // Solarvoltage in V
    
    // SHT21/HTU21: Get temperature and relative humidity
    float data[2] = {0};
    getSHT21Data(data);
    results[2] = data[0]; // Temperature in degC
    results[3] = data[1]; // Relative humidity in %
    
    // Print results
    printf("ADS1115 - Moisture: %0.3f %\n", results[0]);
    printf("ADS1115 - Voltage: %0.3f V\n", results[1]);
    printf("SHT21 - Temperature: %.3f C\n", results[2]);
    printf("SHT21 - Humidity: %.3f %%\n", results[3]);

    // Send data to thingspeak
    int size = sizeof(results)/sizeof(results[0]);
    post2thingspeak(results, size);

    return 0;
}
