#include "LTC2983.h"
#include <unistd.h>
#include <errno.h>

bool LTC_SPI_init(uint8_t bus, uint8_t cs) {

   std::stringstream device;
   int ret;

   device << "/dev/spidev" << int(bus) << "." << int(cs);

   fd = open(device.str().c_str(), O_RDWR);
   if (fd < 0) {
      printf("LTC_SPI_init: can't open device\n");
      return false;
   }

   // SPI mode
   ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
   if (ret == -1) {
      printf("LTC_SPI_init: can't set spi mode\n");
      return false;
   }

   // SPI bits per word
   ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
   if (ret == -1) {
      printf("LTC_SPI_init: can't set bits per word\n");
      return false;
   }

   ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
   if (ret == -1) {
      printf("LTC_SPI_init: can't get bits per word\n");
      return false;
   }

   // SPI SCK speed
   ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
   if (ret == -1) {
      printf("LTC_SPI_init: can't set max speed hz\n");
      return false;
   }

   ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
   if (ret == -1) {
      printf("LTC_SPI_init: can't get max speed hz\n");
      return false;
   }

   return true;
}

bool LTC_SPI_close(void) {

   close(fd);
   return(true);
}

bool LTC_reg_read(uint16_t addr, uint8_t &value) {

   int ret;
   uint8_t tx[] = {READ, (uint8_t)((addr & 0xFF00) >> 8), (uint8_t)(addr & 0x00FF)};
   uint8_t rx;

   struct spi_ioc_transfer tr[2] =
   {
      {
         .tx_buf = (unsigned long)tx,
         .rx_buf = (unsigned long)NULL,
         .len = ARRAY_SIZE(tx),
         .speed_hz = speed,
         .delay_usecs = delay,
         .bits_per_word = bits, 
         .cs_change = 0,
      },
      {
         .tx_buf = (unsigned long)NULL,
         .rx_buf = (unsigned long)&rx,
         .len = 1,
         .speed_hz = speed,
         .delay_usecs = delay,
         .bits_per_word = bits, 
         .cs_change = 0,
      },
   };

   ret = ioctl(fd, SPI_IOC_MESSAGE(2), tr);
   if (ret < 0) {
      printf("LTC_reg_read(uint8_t): can't send spi message\n");
      return false;
   }

   value = rx; 

   return true;
}

bool LTC_reg_read(uint16_t addr, uint32_t &value) {

   int ret;
   uint8_t tx[] = {READ, (uint8_t)((addr & 0xFF00) >> 8), (uint8_t)(addr & 0x00FF)};
   uint8_t rx[4];

   struct spi_ioc_transfer tr[2] =
   {
      {
         .tx_buf = (unsigned long)tx,
         .rx_buf = (unsigned long)NULL,
         .len = ARRAY_SIZE(tx),
         .speed_hz = speed,
         .delay_usecs = delay,
         .bits_per_word = bits, 
         .cs_change = 0,
      },
      {
         .tx_buf = (unsigned long)NULL,
         .rx_buf = (unsigned long)&rx,
         .len = ARRAY_SIZE(rx),
         .speed_hz = speed,
         .delay_usecs = delay,
         .bits_per_word = bits, 
         .cs_change = 0,
      },
   };

   ret = ioctl(fd, SPI_IOC_MESSAGE(2), tr);
   if (ret < 0) {
      printf("LTC_reg_read(uint32_t): can't send spi message (errno=%d)\n", errno);
      return false;
   }

   value = rx[3];
   value += (rx[2] << 8);
   value += (rx[1] << 16);
   value += (rx[0] << 24);

   return true;
}

bool LTC_reg_write(uint16_t addr, uint8_t value) {

   struct spi_ioc_transfer tr;
   int ret;
   uint8_t tx[4];

   tx[0] = WRITE;
   tx[1] = (addr & 0xFF00) >> 8;        // MSB
   tx[2] = (addr & 0x00FF);             // LSB
   tx[3] = value;

   tr.tx_buf = (unsigned long)tx;
   tr.rx_buf = (unsigned long)NULL;
   tr.len = ARRAY_SIZE(tx);
   tr.delay_usecs = delay;
   tr.speed_hz = speed;
   tr.bits_per_word = bits;
   tr.cs_change = 0;

   ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
   if (ret < 0) {
      printf("LTC_reg_write(uint8_t): can't send spi message\n");
      return false;
   }

   return true;
}

bool LTC_reg_write(uint16_t addr, uint32_t value) {

   int ret;
   uint8_t tx[7];

   tx[0] = WRITE;
   tx[1] = (addr & 0xFF00) >> 8;        // MSB
   tx[2] = (addr & 0x00FF);             // LSB
   tx[3] = (value & 0xFF000000) >> 24;	
   tx[4] = (value & 0x00FF0000) >> 16;	
   tx[5] = (value & 0x0000FF00) >> 8;	
   tx[6] = value & 0x000000FF;	

   struct spi_ioc_transfer tr = 
   {
      .tx_buf = (unsigned long)tx,
      .rx_buf = (unsigned long)NULL,
      .len = ARRAY_SIZE(tx),
      .speed_hz = speed,
      .delay_usecs = delay,
      .bits_per_word = bits, 
      .cs_change = 0,
   };

   ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
   if (ret < 0) {
      printf("LTC_reg_write(uint32_t): can't send spi message\n");
      return false;
   }

   return true;
}

void LTC_ch_config(int ch, uint32_t chdata) {

   uint16_t addr = 0x200 + 4*(ch - 1);
   LTC_reg_write(addr, chdata);
}

void LTC_ch_add(int ch) {

   uint32_t regval;
   // add channel to multiple conversion mask register
   LTC_reg_read(MULCONV_REG, regval);
   LTC_reg_write(MULCONV_REG, regval | (uint32_t)(1 << (ch - 1)));
}

void LTC_mul_convert(void) {

   LTC_reg_write(0x00, (uint8_t)(0b10000000));
   while(!LTC_conv_done());     // wait for conversion to complete
}

void LTC_ch_convert(int ch) {

   LTC_reg_write(0x00, (uint8_t)(0b10000000 | ch));
   while(!LTC_conv_done());	// wait for conversion to complete
}

bool LTC_conv_done(void) {

   uint8_t value = 0;
   LTC_reg_read(0x00, value);
   return(value & 0b01000000); 
}

void print_fault_data(unsigned char fault_byte) {

  printf("FAULT DATA=");

  if((fault_byte & SENSOR_HARD_FAILURE) > 0) printf("*SENSOR HARD FALURE*");
  if((fault_byte & ADC_HARD_FAILURE) > 0) printf("*ADC_HARD_FAILURE*");
  if((fault_byte & CJ_HARD_FAILURE) > 0) printf("*CJ_HARD_FAILURE*");
  if((fault_byte & CJ_SOFT_FAILURE) > 0) printf("*CJ_SOFT_FAILURE*");
  if((fault_byte & SENSOR_ABOVE) > 0) printf("*SENSOR_ABOVE*");
  if((fault_byte & SENSOR_BELOW) > 0) printf("*SENSOR_BELOW*");
  if((fault_byte & ADC_RANGE_ERROR) > 0) printf("*ADC_RANGE_ERROR*");
  if((fault_byte & VALID) != 1) printf("!!!!!!! INVALID READING !!!!!!!!!");
  if(fault_byte == 0b11111111) printf("&&&&&&&&&& CONFIGURATION ERROR &&&&&&&&&&&&");
  printf("\n");
}

void LTC_get_raw(uint32_t baseaddr, int ch, uint32_t &value) {

   uint16_t addr = baseaddr + 4*(ch - 1);
   LTC_reg_read(addr, value);  
   print_fault_data((value & 0xFF000000) >> 24);
}

int32_t LTC_raw_to_signed(uint32_t value) {

   long x = 0L;
   bool sign;  

   x = value & 0x00FFFFFF;

   // Convert a 24-bit two's complement number into a 32-bit two's complement number
   if(value & 0x00800000)
      sign = true; 
   else sign = false;	
  
   if (sign) 
      x = x | 0xFF000000;

   return int32_t(x);
}

float LTC_voltage_read(int ch) {

   uint32_t rval;
   int32_t sval;

   LTC_get_raw(READ_CH_BASE, ch, rval);
   sval = LTC_raw_to_signed(rval);

   return(float(sval / 2097152.0));	// divide by 2^21
}

float LTC_temperature_read(int ch) {

   uint32_t rval;
   int32_t sval;

   LTC_get_raw(READ_CH_BASE, ch, rval);
   sval = LTC_raw_to_signed(rval);

   return(float(sval / 1024.0));	// divide by 2^10 
}
