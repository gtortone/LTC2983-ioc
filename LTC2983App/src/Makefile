TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = LTC2983
# LTC2983.dbd will be created and installed
DBD += LTC2983.dbd

# LTC2983.dbd will be made up from these files:
LTC2983_DBD += base.dbd

# Include dbd files from all support applications:
LTC2983_DBD += LTC2983dev.dbd

# Add all the support libraries needed by this IOC
#LTC2983_LIBS += xxx

# LTC2983_registerRecordDeviceDriver.cpp derives from LTC2983.dbd
LTC2983_SRCS += LTC2983_registerRecordDeviceDriver.cpp
LTC2983_SRCS += devLTC2983.cpp spiLTC2983.cpp
USR_CXXFLAGS += -fpermissive -O2

# Build the main IOC entry point on workstation OSs.
LTC2983_SRCS_DEFAULT += LTC2983Main.cpp
LTC2983_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#LTC2983_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
LTC2983_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

