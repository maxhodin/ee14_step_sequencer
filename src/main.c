#include "ee14lib.h"
#include <string.h>

#define inv_7fac 0.0001984
#define inv_5fac 0.008333
#define inv_3fac 0.16667


// Lookup table for sine 
const uint8_t sine_LUT [256] = {1.0, 1.0, 1.0, 1.0, 2.0, 2.0,
                                 2.0, 3.0, 3.0, 4.0, 5.0, 6.0, 
                                 6.0, 7.0, 8.0, 10.0, 11.0, 12.0, 
                                 13.0, 15.0, 16.0, 17.0, 19.0, 21.0, 
                                 22.0, 24.0, 26.0, 28.0, 30.0, 32.0, 
                                 34.0, 36.0, 38.0, 40.0, 43.0, 45.0, 
                                 47.0, 50.0, 52.0, 55.0, 57.0, 60.0, 
                                 63.0, 65.0, 68.0, 71.0, 74.0, 77.0, 
                                 79.0, 82.0, 85.0, 88.0, 91.0, 94.0, 
                                 97.0, 100.0, 103.0, 106.0, 109.0, 112.0, 
                                 116.0, 119.0, 122.0, 125.0, 128.0, 131.0, 
                                 134.0, 137.0, 140.0, 144.0, 147.0, 150.0, 
                                 153.0, 156.0, 159.0, 162.0, 165.0, 168.0, 
                                 171.0, 174.0, 177.0, 179.0, 182.0, 185.0, 
                                 188.0, 191.0, 193.0, 196.0, 199.0, 201.0, 
                                 204.0, 206.0, 209.0, 211.0, 213.0, 216.0, 
                                 218.0, 220.0, 222.0, 224.0, 226.0, 228.0, 
                                 230.0, 232.0, 234.0, 235.0, 237.0, 239.0, 
                                 240.0, 241.0, 243.0, 244.0, 245.0, 246.0, 
                                 248.0, 249.0, 250.0, 250.0, 251.0, 252.0, 
                                 253.0, 253.0, 254.0, 254.0, 254.0, 255.0, 
                                 255.0, 255.0, 255.0, 255.0, 255.0, 255.0, 
                                 254.0, 254.0, 254.0, 253.0, 253.0, 252.0, 
                                 251.0, 250.0, 250.0, 249.0, 248.0, 246.0, 
                                 245.0, 244.0, 243.0, 241.0, 240.0, 239.0, 
                                 237.0, 235.0, 234.0, 232.0, 230.0, 228.0, 
                                 226.0, 224.0, 222.0, 220.0, 218.0, 216.0, 
                                 213.0, 211.0, 209.0, 206.0, 204.0, 201.0, 
                                 199.0, 196.0, 193.0, 191.0, 188.0, 185.0, 
                                 182.0, 179.0, 177.0, 174.0, 171.0, 168.0, 
                                 165.0, 162.0, 159.0, 156.0, 153.0, 150.0, 
                                 147.0, 144.0, 140.0, 137.0, 134.0, 131.0, 
                                 128.0, 125.0, 122.0, 119.0, 116.0, 112.0, 
                                 109.0, 106.0, 103.0, 100.0, 97.0, 94.0, 91.0, 
                                 88.0, 85.0, 82.0, 79.0, 77.0, 74.0, 71.0, 68.0, 
                                 65.0, 63.0, 60.0, 57.0, 55.0, 52.0, 50.0, 47.0, 
                                 45.0, 43.0, 40.0, 38.0, 36.0, 34.0, 32.0, 30.0, 
                                 28.0, 26.0, 24.0, 22.0, 21.0, 19.0, 17.0, 16.0, 
                                 15.0, 13.0, 12.0, 11.0, 10.0, 8.0, 7.0, 6.0, 6.0, 
                                 5.0, 4.0, 3.0, 3.0, 2.0, 2.0, 2.0, 1.0, 1.0, 1.0};


// This function is called by printf() to handle the text string
// We want it to be sent over the serial terminal, so we just delegate to that function
int _write(int file, char *data, int len) {
    // Eventually you'll want to change this to serial_write_nonblocking
    serial_write(USART2, data, len);
    return len;
}

// NOTE: TIME THIS AND COMPARE TO MATH.H's SIN()
// Taylor Series of Sine (first 4 terms), adapted from White Chapter 12 Designing and Modifying Algorithms
// Returns 8-bit result where the MSB is 2^-1
// Argument: 32 bit angle theta
// uint8_t pseudo_sine(uint32_t theta){
//     //roughly 55 clock cycles per floating point multiply using software floats
//     float theta_sq = (theta * theta);
//     float temp = -inv_7fac * theta_sq;
//     temp = theta_sq * (inv_5fac + temp);
//     temp = theta_sq * (-inv_3fac + temp);
//     temp = theta * (1-temp);
//     return (uint8_t) temp * 256; // should be between 0 and 1, (pseudo)"shifted by 8"
// }

int main(){
    host_serial_init(9600);
    dac_config_single(0);
    // About 2 us to get SINE and about 5 us to write to DAC
    // About 820 Hz sine @ 209.92 kHz polling rate max
    while(1){
        for(int i=0; i<256; i++){
            uint8_t val = sine_LUT[i];
            dac_write(val);
        }
    }
    return 0;
}