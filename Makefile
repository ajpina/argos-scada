INST_DIR    = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
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
DEP_DIR    := .deps


# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Define what we want to build and how to build it ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

all :
	@echo "We assume that the support tools are already in place ..."
	@echo "We need fossil (SCM), Tcl/Tk, ???"
	make -C $(SRC_DIR)


# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Finally the project actions which are not based on actual files ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.PHONY :  clean

clean :
	make -C $(SRC_DIR) clean




