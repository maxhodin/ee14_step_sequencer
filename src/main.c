#include "ee14lib.h"
#include <string.h>

#define PERIPH_ADDRESS 104

// Address definition as const variables to ensure 8-bit size
unsigned char MPU_6050_SLEEP [2]= {0x6B,0x00};
unsigned char MPU_6050_ACCEL_XOUT_H[1] = {59};
unsigned char MPU_6050_ACCEL_XOUT_L [1]= {60};
unsigned char MPU_6050_ACCEL_YOUT_H [1]= {61};
unsigned char MPU_6050_ACCEL_YOUT_L [1]= {62};
unsigned char MPU_6050_ACCEL_ZOUT_H [1]= {63};
unsigned char MPU_6050_ACCEL_ZOUT_L[1]=  {64};
unsigned char MPU_6050_GYRO_XOUT_H [1]=  {67};
unsigned char MPU_6050_GYRO_XOUT_L [1]= {68};
unsigned char MPU_6050_GYRO_YOUT_H [1]= {69};
unsigned char MPU_6050_GYRO_YOUT_L[1] = {70};
unsigned char MPU_6050_GYRO_ZOUT_H[1] = {71};
unsigned char MPU_6050_GYRO_ZOUT_L[1] = {72};

// Declarations of acceleration and gyroscope variables to be measured
unsigned char ACCEL_XOUT_H[1];
unsigned char ACCEL_XOUT_L[1];
unsigned char ACCEL_YOUT_H[1];
unsigned char ACCEL_YOUT_L[1];
unsigned char ACCEL_ZOUT_H[1];
unsigned char ACCEL_ZOUT_L[1];
unsigned char GYRO_XOUT_H[1] ;
unsigned char GYRO_XOUT_L[1] ;
unsigned char GYRO_YOUT_H[1];
unsigned char GYRO_YOUT_L[1] ;
unsigned char GYRO_ZOUT_H[1] ;
unsigned char GYRO_ZOUT_L [1];

// Used by printf
// Implemented using blocking serial write (takes on order of 10s-100s of ms)
int _write(int file, char *data, int len) {
    // Eventually you'll want to change this to serial_write_nonblocking
    serial_write(USART2, data, len);
    return len;
}

// Converts string of 2 char Bytes into int16_t (signed)
// Arg: pointer to char array of length 2 Bytes + \0
// Returns: 16 bit signed int
int16_t stoi_16(char *str){
    int16_t MS_half = (int16_t) str[0];
    int16_t LS_half = (int16_t) str[1];
    return LS_half + (MS_half << 8);
}

// Updates acceleration and gyroscope varaibles via MPU-6050
// No arguments and no return
// Should be called once per main loop
void read_data(){
    // For each register, a write command is given with the address of the register
    // This is immediately followed by a read command
    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_XOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_XOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_XOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_XOUT_L,1);
    
    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_YOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_YOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_YOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_YOUT_L,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_ZOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_ZOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_ACCEL_ZOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,ACCEL_ZOUT_L,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_XOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_XOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_XOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_XOUT_L,1);
    
    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_YOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_YOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_YOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_YOUT_L,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_ZOUT_H, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_ZOUT_H,1);

    i2c_write(I2C1, PERIPH_ADDRESS, MPU_6050_GYRO_ZOUT_L, 1);
    i2c_read(I2C1, PERIPH_ADDRESS,GYRO_ZOUT_L,1);

}

int main(){
    i2c_init(I2C1, D5, D4);
    host_serial_init(9600);

    // Disable the sleep functionality on the MPU-6050
    i2c_write(I2C1,PERIPH_ADDRESS,MPU_6050_SLEEP,2);

    int i = 0;
    // Main loop 1000 times
    while(i < 500){
        read_data();
        int16_t accel_x = (int16_t) ((ACCEL_XOUT_H[0] << 8) | ACCEL_XOUT_L[0]);
        int16_t accel_y = (int16_t) ((ACCEL_YOUT_H[0] << 8) | ACCEL_YOUT_L[0]);
        int16_t accel_z = (int16_t) ((ACCEL_ZOUT_H[0] << 8) | ACCEL_ZOUT_L[0]);
        int16_t gyro_x = (int16_t) ((GYRO_XOUT_H[0] << 8) | GYRO_XOUT_L[0]);
        int16_t gyro_y = (int16_t) ((GYRO_YOUT_H[0] << 8) | GYRO_YOUT_L[0]);
        int16_t gyro_z = (int16_t) ((GYRO_ZOUT_H[0] << 8) | GYRO_ZOUT_L[0]);

        printf("%d,%d,%d,%d,%d,%d\n", accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
        i++;
    }
    
    return 0;
}