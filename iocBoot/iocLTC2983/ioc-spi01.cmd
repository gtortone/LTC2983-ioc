#!../../bin/linux-arm/LTC2983

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/LTC2983.dbd"
LTC2983_registerRecordDeviceDriver pdbbase

## autoSaveRestore setup
save_restoreSet_Debug(0)
set_savefile_path("autosave", "");
set_requestfile_path("autosave", "")
set_pass1_restoreFile("pv-spi01-a.sav")
set_pass1_restoreFile("pv-spi01-b.sav")
save_restoreSet_NumSeqFiles(0)
save_restoreSet_DatedBackupFiles(0)

## set here sector names to monitor
epicsEnvSet("SNAME_A","1")
epicsEnvSet("SNAME_B","2")
epicsEnvSet("TLABEL_01","TEMP01")
epicsEnvSet("TLABEL_02","TEMP02")
epicsEnvSet("TLABEL_03","TEMP03")

## configure module SPI bus number, SPI chip select
## bus number 1 = /dev/spidev1.[cs]	uSOP label = JSPI0
## bus number 2 = /dev/spidev2.[cs]	uSOP label = JSPI1
devLTC2983config(1, 0)

## Load record instances
## SNAME - sector name of ECL (e.g. S1F, S8B) F = forward , B = backward
dbLoadRecords("db/sector-A.db","SNAME=$(SNAME_A),TEMP01=$(TLABEL_01),TEMP02=$(TLABEL_02),TEMP03=$(TLABEL_03)")
dbLoadRecords("db/sector-B.db","SNAME=$(SNAME_B),TEMP01=$(TLABEL_01),TEMP02=$(TLABEL_02),TEMP03=$(TLABEL_03)")

## IOC access security
## asSetFilename("/opt/LTC2983-ioc/iocBoot/iocLTC2983/policy.example")

cd ${TOP}/iocBoot/${IOC}
iocInit

create_triggered_set("pv-spi01-a.req","$(SNAME_A):CH8:if","SNAME=$(SNAME_A)")
create_triggered_set("pv-spi01-b.req","$(SNAME_B):CH13:if","SNAME=$(SNAME_B)")

## create_monitor_set("pv-spi01.req",30,"SNAME=$(SNAME_A)")
