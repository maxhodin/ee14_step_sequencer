#include "ee14lib.h"
#include <string.h>

#define inv_7fac 0.0001984
#define inv_5fac 0.008333
#define inv_3fac 0.16667

// NOTE: TIME THIS AND COMPARE TO MATH.H's SIN()
// Taylor Series of Sine (first 4 terms), adapted from White Chapter 12 Designing and Modifying Algorithms
// Returns 8-bit result where the MSB is 2^-1
// Argument: 32 bit angle theta
uint8_t pseudo_sine(uint32_t theta){
    //roughly 55 clock cycles per floating point multiply using software floats
    float theta_sq = (theta * theta);
    float temp = -inv_7fac * theta_sq;
    temp = theta_sq * (inv_5fac + temp);
    temp = theta_sq * (-inv_3fac + temp);
    temp = theta * (1-temp);
    return (uint8_t) temp * 256; // should be between 0 and 1, (pseudo)"shifted by 8"
}

int main(){
    dac_config_single(0);
    int i = 0;
    while(1){
        for(int j=0;j<100;j++){ // 100 samples
            dac_write(pseudo_sine(i/(628))); // i / 100 * 2pi
            for(int k=0; k<100;k++){} // spin for ~100 cycles
        }
    }
    return 0;
}