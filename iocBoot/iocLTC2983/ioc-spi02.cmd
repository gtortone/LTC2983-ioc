#!../../bin/linux-arm/LTC2983

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/LTC2983.dbd"
LTC2983_registerRecordDeviceDriver pdbbase

## configure module SPI bus number, SPI chip select
## bus number 1 = /dev/spidev1.[cs]
## bus number 2 = /dev/spidev2.[cs]
devLTC2983config(2, 0)

## Load record instances
## SNAME - sector name of ECL (e.g. S1F, S8B) F = forward , B = backward
dbLoadRecords("db/sector-A.db","SNAME=3")
dbLoadRecords("db/sector-B.db","SNAME=4")

cd ${TOP}/iocBoot/${IOC}
iocInit
