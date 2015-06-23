#!../../bin/linux-arm/LTC2983

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/LTC2983.dbd"
LTC2983_registerRecordDeviceDriver pdbbase

## configure module SPI bus number, SPI chip select
devLTC2983config(2, 0)

## Load record instances
dbLoadRecords("db/sector-8F.db")

cd ${TOP}/iocBoot/${IOC}
iocInit
