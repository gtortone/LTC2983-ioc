#!../../bin/linux-arm/LTC2983

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/LTC2983.dbd"
LTC2983_registerRecordDeviceDriver pdbbase

## configure module SPI bus number, SPI chip select
## bus number 1 = /dev/spidev1.[cs]	uSOP label = JSPI0
## bus number 2 = /dev/spidev2.[cs]	uSOP label = JSPI1
devLTC2983config(1, 0)

## Load record instances
## SNAME - sector name of ECL (e.g. S1F, S8B) F = forward , B = backward
dbLoadRecords("db/sector-A.db","SNAME=1")
dbLoadRecords("db/sector-B.db","SNAME=2")

cd ${TOP}/iocBoot/${IOC}
iocInit
