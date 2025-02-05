#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <recSup.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <errlog.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <epicsExport.h>
#include <epicsMutex.h>

#include <aiRecord.h>
#include <aoRecord.h>
#include <iocsh.h>
#include <string.h>
#include "spiLTC2983.h"

static long init_device(int phase);
static long init_ai_record(aiRecord *prec);
static long read_ai(aiRecord *prec);

static long init_ao_record(aoRecord *prec);
static long write_ao(aoRecord *prec);

struct busConfig {

   int adapter_nr;              // SPI bus number
   int adapter_cs;              // SPI chip select number
};

struct {

   long      number;
   DEVSUPFUN report;
   DEVSUPFUN init;
   DEVSUPFUN init_record;
   DEVSUPFUN get_ioint_info;
   DEVSUPFUN read_ai;
   DEVSUPFUN special_linconv;

} devAiLTC2983 = {

   6,
   NULL,
   init_device,
   init_ai_record,
   NULL,
   read_ai,
   NULL
};

struct {

   long            number;
   DEVSUPFUN	   report;
   DEVSUPFUN       init;
   DEVSUPFUN       init_record;
   DEVSUPFUN       get_ioint_info;
   DEVSUPFUN       write_ao;
   DEVSUPFUN       special_linconv;

} devAoLTC2983 = {

   6,
   NULL,
   NULL,
   init_ao_record,
   NULL,
   write_ao,
   NULL
};

// global var for diode ideality factor (CH8 and CH13)
static double ch8_if, ch13_if;

// global vars for SPI bus number, SPI chip select address
static struct busConfig spibus;

// mutex to guarantee exclusive access between thread for different timeperiod
epicsMutexId mutex;

// device configuration

int devLTC2983config(int busno, int cs) {

    spibus.adapter_nr = busno;
    spibus.adapter_cs = cs;
    return(0);
}

static const iocshArg configArg0 = {"spibusno", iocshArgInt};
static const iocshArg configArg1 = {"spics", iocshArgInt};

static const iocshArg *configArgs[] = {
    &configArg0, &configArg1
};

static const iocshFuncDef configFuncDef = {
    "devLTC2983config", 2, configArgs
};

static void devLTC2983configCallFunc(const iocshArgBuf *args) {

    devLTC2983config(args[0].ival, args[1].ival);
}

static void devLTC2983Registrar() {

    iocshRegister(&configFuncDef, devLTC2983configCallFunc);
}

epicsExportRegistrar(devLTC2983Registrar);

static long init_device(int phase) {

   if(phase == 0) {

      mutex = epicsMutexCreate();
 
      if(LTC_SPI_init(spibus.adapter_nr, spibus.adapter_cs) == false) {
         errlogPrintf("ERROR: SPI file open failed\n");
         return(S_dev_noDevSup);
      }

      uint32_t chdata;

      // configure global parameters
      printf("Configure register 0xF0....");
      LTC_reg_write(0xF0, (uint8_t)(TEMP_UNIT__C | REJECTION__50_60_HZ));
      printf(" DONE\n");
      printf("Configure register mux delay....");
      LTC_reg_write(0xFF, (uint8_t)(0));   // set 0ms mux delay between conversions
      printf(" DONE\n");
      LTC_reg_write(MULCONV_REG, (uint32_t)(0));   // initialize multiple conversion mask register

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
           (uint32_t) DIODE_AVERAGING_ON |
           (uint32_t) DIODE_CURRENT__20UA_80UA_160UA | (uint32_t) (1048576 * ch8_if);		// default value: 1.003
           //(uint32_t) (0b11100110010111000100000110001001 << DIODE_IDEALITY_FACTOR_LSB);
      LTC_ch_config(8,chdata);
      LTC_ch_add(8);

      // Channel 10: assign Sense Resistor
      //  SENSE_RESISTOR_VALUE_LSB = 0
      float Rsense1 = 10000.0; //ohm
      const float two_to_10 = 1024.0;
      chdata = (uint32_t) SENSOR_TYPE__SENSE_RESISTOR |
           (uint32_t) ((uint32_t) (Rsense1 * two_to_10)) << SENSE_RESISTOR_VALUE_LSB;          
      LTC_ch_config(10, chdata);
      printf("R_sense_sector1 = %f ohm\n", (chdata & 0x07FFFFFF) / two_to_10 );

      // Channel 12: assign Sense Resistor
      float Rsense2 = 10000.0; //ohm
      chdata = (uint32_t) SENSOR_TYPE__SENSE_RESISTOR |
           (uint32_t) ((uint32_t) (Rsense2 * two_to_10)) << SENSE_RESISTOR_VALUE_LSB;           
      LTC_ch_config(12, chdata);
      printf("R_sense_sector2 = %f ohm\n", (chdata & 0x07FFFFFF) / two_to_10 );

      // Channel 13: assign off-chip DIODE
      chdata = (uint32_t) SENSOR_TYPE__OFF_CHIP_DIODE |
           (uint32_t) DIODE_SINGLE_ENDED |
           (uint32_t) DIODE_NUM_READINGS__3 |
           (uint32_t) DIODE_AVERAGING_ON |
           (uint32_t) DIODE_CURRENT__20UA_80UA_160UA | (uint32_t) (1048576 * ch13_if);		// default value: 1.003
           //(uint32_t) (0b11100110010110111101011100001010 << DIODE_IDEALITY_FACTOR_LSB);
      LTC_ch_config(13,chdata);
      LTC_ch_add(13);

      // Channel 14: assign Direct ADC
      chdata = (uint32_t) SENSOR_TYPE__DIRECT_ADC |
           (uint32_t) DIRECT_ADC_SINGLE_ENDED;
      LTC_ch_config(14, chdata);
      LTC_ch_add(14);

      // Channel 16: assign Thermistor 44006 10K@25C
      chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
           (uint32_t) THERMISTOR_RSENSE_CHANNEL__12 |
           (uint32_t) THERMISTOR_DIFFERENTIAL |
           (uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
           (uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
      LTC_ch_config(16, chdata);
      LTC_ch_add(16);

      // Channel 18: assign Thermistor 44006 10K@25C
      chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
              (uint32_t) THERMISTOR_RSENSE_CHANNEL__12 |
              (uint32_t) THERMISTOR_DIFFERENTIAL |
              (uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
              (uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
      LTC_ch_config(18, chdata);
      LTC_ch_add(18);

      // Channel 20: assign Thermistor 44006 10K@25C
      chdata = (uint32_t) SENSOR_TYPE__THERMISTOR_44006_10K_25C |
           (uint32_t) THERMISTOR_RSENSE_CHANNEL__12 |
           (uint32_t) THERMISTOR_DIFFERENTIAL |
           (uint32_t) THERMISTOR_EXCITATION_MODE__SHARING_ROTATION |
           (uint32_t) THERMISTOR_EXCITATION_CURRENT__AUTORANGE;
      LTC_ch_config(20, chdata);
      LTC_ch_add(20);
   }

   return(0);
}


static long init_ai_record(aiRecord *prec) {

   int retval;
   int chnum;
   int *ch;

   retval = sscanf(prec->inp.value.instio.string, "%d", &chnum);

   if(retval != 1) {

      recGblRecordError(S_db_badField, (void*)prec, "devLTC2983: illegal input channel");
   }

   ch = (int *) malloc(sizeof(int));
   *ch = chnum;

   prec->dpvt = (int *) ch;

   return(0);
}

static long read_ai(aiRecord *prec) {

   uint32_t raw_value;
   int *ch;

   ch = (int *) prec->dpvt;
 
   epicsMutexLock(mutex);
      LTC_ch_convert(*ch);
      LTC_get_raw(READ_CH_BASE, *ch, raw_value);
   epicsMutexUnlock(mutex);

   prec->rval = LTC_raw_to_signed(raw_value);

   return(0);
}

static long init_ao_record(aoRecord *prec) {

//   prec->udf = FALSE;
   return(0);
}

static long write_ao(aoRecord *prec) {

   int retval;
   char param[16];
   uint32_t chdata;

   retval = sscanf(prec->name, "%*[^:]:%s", param); 

   if(retval != 1) {

      errlogPrintf("ERROR: field name not correct\n");      
      prec->udf = TRUE;
      return(S_db_badField);
   }

   prec->udf = FALSE;

   if(strcasecmp(param, (char *) "CH8:if") == 0) {

      ch8_if = prec->val;

      epicsMutexLock(mutex);
         // Channel 8: assign off-chip DIODE
         chdata = (uint32_t) SENSOR_TYPE__OFF_CHIP_DIODE |
              (uint32_t) DIODE_SINGLE_ENDED |
              (uint32_t) DIODE_NUM_READINGS__3 |
              (uint32_t) DIODE_AVERAGING_ON |
              (uint32_t) DIODE_CURRENT__20UA_80UA_160UA | (uint32_t) (1048576 * ch8_if);
         LTC_ch_config(8,chdata);
      epicsMutexUnlock(mutex);

   } else if(strcasecmp(param, (char *) "CH13:if") == 0) {

      ch13_if = prec->val;

      epicsMutexLock(mutex);
         // Channel 13: assign off-chip DIODE
         chdata = (uint32_t) SENSOR_TYPE__OFF_CHIP_DIODE |
              (uint32_t) DIODE_SINGLE_ENDED |
              (uint32_t) DIODE_NUM_READINGS__3 |
              (uint32_t) DIODE_AVERAGING_ON |
              (uint32_t) DIODE_CURRENT__20UA_80UA_160UA | (uint32_t) (1048576 * ch13_if);
         LTC_ch_config(13,chdata);
      epicsMutexUnlock(mutex);
   }

   return(0);
}

epicsExportAddress(dset,devAiLTC2983);
epicsExportAddress(dset,devAoLTC2983);
