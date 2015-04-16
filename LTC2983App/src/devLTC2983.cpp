#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <errlog.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <epicsExport.h>
#include <epicsMutex.h>

#include <aiRecord.h>
#include <iocsh.h>
#include "spiLTC2983.h"

static long init_device(int phase);
static long init_ai_record(aiRecord *prec);
static long read_ai(aiRecord *prec);

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

// global var for rejection and speed config words
static uint8_t g_rejection;

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
      LTC_reg_write(0xF0, (uint8_t)(TEMP_UNIT__C | REJECTION__50_60_HZ));
      LTC_reg_write(0xFF, (uint8_t)(0));   // set 0ms mux delay between conversions
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
           (uint32_t) DIODE_AVERAGING_OFF |
           (uint32_t) DIODE_CURRENT__20UA_80UA_160UA |
           (uint32_t) (0b0111000101111000110000<< DIODE_IDEALITY_FACTOR_LSB);
      LTC_ch_config(8,chdata);
      LTC_ch_add(8);

      // Channel 10: assign Sense Resistor
      chdata = (uint32_t) SENSOR_TYPE__SENSE_RESISTOR |
           (uint32_t) 0b000100111000100000000000000 << SENSE_RESISTOR_VALUE_LSB;           // sense resistor - value: 10000.
      LTC_ch_config(10, chdata);

      // Channel 12: assign Sense Resistor
      chdata = (uint32_t) SENSOR_TYPE__SENSE_RESISTOR |
           (uint32_t) 0b000100111000100000000000000 << SENSE_RESISTOR_VALUE_LSB;           // sense resistor - value: 10000.
      LTC_ch_config(12, chdata);

      // Channel 13: assign off-chip DIODE
      chdata = (uint32_t) SENSOR_TYPE__OFF_CHIP_DIODE |
           (uint32_t) DIODE_SINGLE_ENDED |
           (uint32_t) DIODE_NUM_READINGS__3 |
           (uint32_t) DIODE_AVERAGING_OFF |
           (uint32_t) DIODE_CURRENT__20UA_80UA_160UA |
           (uint32_t) (0b0111000101111000110000<< DIODE_IDEALITY_FACTOR_LSB);
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

epicsExportAddress(dset,devAiLTC2983);
