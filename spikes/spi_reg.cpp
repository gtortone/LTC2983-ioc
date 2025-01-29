#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include "LTC2983.h"

int main(void) {

   uint8_t i, ret;

   struct timeval stop, start;

   uint16_t addr1 = 0x0000;
   uint8_t val1 = 0;
   uint8_t rdval = 0;
   uint32_t value = 0;

   uint32_t chdata;
   float fval = 0;

   if(LTC_SPI_init(1, 0) == false) {
      printf("LTC_SPI_init: error\n");
      abort();
   }

   //uint8_t v8;
   //uint32_t v32;

   // write 8 bit - read 8 bit
   /*
   LTC_reg_write(0xFF, (uint8_t) 0x22);
   LTC_reg_read(0x00FF, v8);
   printf("v8 = %d\n", v8);
   */

   // write 32 bit - read 32 bit
   /*
   LTC_reg_write(0xF4, (uint32_t) 0x11223344);
   LTC_reg_read(0x00F4, v32);
   printf("v32 = 0x%X\n", v32);
   */

   // configure global parameters
   LTC_reg_write(0xF0, (uint8_t)(TEMP_UNIT__C | REJECTION__50_60_HZ));

   //LTC_reg_write(0xFF, (uint8_t)(50));	// set 5ms mux delay between conversions
   LTC_reg_write(0xFF, (uint8_t)(0));	// set 0ms mux delay between conversions
   LTC_reg_write(MULCONV_REG, (uint32_t)(0)); 	// initialize multiple conversion mask register

   // Channel 2: assign Thermistor 44006 10K@25C
   chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
	(uint32_t) THERMISTOR_RSENSE_CHANNEL__10 |
	(uint32_t) THERMISTOR_DIFFERENTIAL |
	(uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
	(uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
   LTC_ch_config(2, chdata);
   LTC_ch_add(2);


   // Channel 4: assign Thermistor 44006 10K@25C
   chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
	(uint32_t) THERMISTOR_RSENSE_CHANNEL__10 |
	(uint32_t) THERMISTOR_DIFFERENTIAL |
	(uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
	(uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
   LTC_ch_config(4, chdata);
   LTC_ch_add(4);

   // Channel 6: assign Thermistor 44006 10K@25C
   chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
	(uint32_t) THERMISTOR_RSENSE_CHANNEL__10 |
	(uint32_t) THERMISTOR_DIFFERENTIAL |
	(uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
	(uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
   LTC_ch_config(6, chdata);
   LTC_ch_add(6);

   // Channel 7: assign Direct ADC
   chdata = (uint32_t) SENSOR_TYPE__DIRECT_ADC | 
	(uint32_t) DIRECT_ADC_SINGLE_ENDED;
   LTC_ch_config(7, chdata);
   LTC_ch_add(7);

   // Channel 8: assign off-chip DIODE
   chdata = (uint32_t) SENSOR_TYPE__OFF_CHIP_DIODE |
	(uint32_t) DIODE_SINGLE_ENDED |
	(uint32_t) DIODE_NUM_READINGS__3 |
	(uint32_t) DIODE_AVERAGING_OFF |
	(uint32_t) DIODE_CURRENT__20UA_80UA_160UA |
	(uint32_t) (0b0111000101111000110000<< DIODE_IDEALITY_FACTOR_LSB);
   LTC_ch_config(8,chdata);
   LTC_ch_add(8);
   
   // Channel 10: assign Sense Resistor
   chdata = (uint32_t) SENSOR_TYPE__SENSE_RESISTOR |
	(uint32_t) 0b000100111000100000000000000 << SENSE_RESISTOR_VALUE_LSB;		// sense resistor - value: 10000.
   LTC_ch_config(10, chdata);

   printf("\n\n");

   LTC_reg_read(MULCONV_REG, value);
   printf("multiple conversions mask register = %.4X\n", value);

   printf(">>> start multiple conversions\n");
   gettimeofday(&start, NULL);
      LTC_mul_convert();
   gettimeofday(&stop, NULL);
   printf("execution time: %.0f ms\n\n", (float)((stop.tv_sec - start.tv_sec) * 1000.0f + (stop.tv_usec - start.tv_usec) / 1000.0f));

   printf(">>> THERMISTOR#1 (channel 2-1)\n");
   fval = LTC_temperature_read(2);
   printf("READ temperature ch2 (float): %f\n", fval);

   printf("\n\n");

   printf(">>> THERMISTOR#2 (channel 4-3)\n");
   fval = LTC_temperature_read(4);
   printf("READ temperature ch4 (float): %f\n", fval);

   printf("\n\n");
 
   printf(">>> THERMISTOR#3 (channel 6-5)\n");
   fval = LTC_temperature_read(6);
   printf("READ temperature ch6 (float): %f\n", fval);

   printf("\n\n");

   printf(">>> DIRECT ADC channel\n");
   fval = LTC_voltage_read(7);
   printf("READ direct ADC ch7 (float): %f\n", fval);

   printf("\n\n");

   printf(">>> DIODE channel\n");
   fval = LTC_temperature_read(8);
   printf("diode ideality factor: %f\n", (0b0111000101111000110000 / 1048576.0));
   printf("READ diode temperature ch8 (float): %f\n", fval);

   printf("\n\n");

   LTC_SPI_close();   

   return(0);
}
