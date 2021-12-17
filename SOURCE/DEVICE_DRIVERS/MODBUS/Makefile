INST_DIR    = $(shell dirname $(realpath ../..))
MST_DIR     = $(abspath $(dir $(INST_DIR)))
PRJ_NAME    = argos-scada
PRJ_DIR     = $(MST_DIR)/$(PRJ_NAME)
BIN_DIR     = $(PRJ_DIR)/BIN
DAT_DIR     = $(PRJ_DIR)/DAT
REF_DIR     = $(PRJ_DIR)/REFERENCES
SCR_DIR     = $(PRJ_DIR)/SCRIPTS
SRC_DIR     = $(PRJ_DIR)/SOURCE
INC_DIR     = $(PRJ_DIR)/INCLUDE
OBJ_DIR     = $(PRJ_DIR)/OBJECT
LIB_DIR     = $(PRJ_DIR)/LIB


all :
	@echo "SOURCE/DEVICE_DRIVERS/MODBUS: $(PRJ_DIR)"
	make -C MODBUS_EMULATOR
	make -C MODBUS_EMULATOR_TEST


# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Finally the project actions which are not based on actual files ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.PHONY :  clean

clean :
	make -C MODBUS_EMULATOR      clean
	make -C MODBUS_EMULATOR_TEST clean





