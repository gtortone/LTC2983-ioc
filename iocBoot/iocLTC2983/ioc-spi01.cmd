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

## sectors name config
# BEGIN ANSIBLE MANAGED BLOCK
epicsEnvSet("SNAME_A","1")
epicsEnvSet("SNAME_B","2")
# END ANSIBLE MANAGED BLOCK

## ECL generic config
epicsEnvSet("LABEL_01","TEMP01")
epicsEnvSet("LABEL_02","TEMP02")
epicsEnvSet("LABEL_03","TEMP03")

## BEAST config
## epicsEnvSet("LABEL_01","CSITL")
## epicsEnvSet("LABEL_02","CSIPURE")
## epicsEnvSet("LABEL_03","LYSO")

## configure module SPI bus number, SPI chip select
## bus number 0 = /dev/spidev0.[cs]	uSOP label = JSPI0   (connectors #1, #2)
## bus number 1 = /dev/spidev1.[cs]	uSOP label = JSPI1   (connectors #3, #4)
devLTC2983config(0, 0)

## Load record instances
## SNAME - sector name of ECL (e.g. S1F, S8B) F = forward , B = backward
dbLoadRecords("db/sector-A.db","SNAME=$(SNAME_A),LABEL01=$(LABEL_01),LABEL02=$(LABEL_02),LABEL03=$(LABEL_03)")
dbLoadRecords("db/sector-B.db","SNAME=$(SNAME_B),LABEL01=$(LABEL_01),LABEL02=$(LABEL_02),LABEL03=$(LABEL_03)")

## IOC access security
## asSetFilename("/opt/LTC2983-ioc/iocBoot/iocLTC2983/policy.example")

cd ${TOP}/iocBoot/${IOC}
iocInit

create_triggered_set("pv-spi01-a.req","$(SNAME_A):CH8:if","SNAME=$(SNAME_A)")
create_triggered_set("pv-spi01-b.req","$(SNAME_B):CH13:if","SNAME=$(SNAME_B)")

## create_monitor_set("pv-spi01.req",30,"SNAME=$(SNAME_A)")
