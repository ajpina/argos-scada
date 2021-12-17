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
DEP_DIR    := .deps

GCC         = gcc
GPP         = g++

OPTIMIZE    = -O0
DEBUG       = -g
WARNINGS    = -Wall
CFLAGS      = $(WARNINGS) $(DEBUG) $(OPTIMIZE) -D_GNU_SOURCE -I $(INC_DIR) -I ./
LIBS        =

AR          = ar
ARFLAGS     = -rc

COMPILER    = $(GCC)

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Find all the C language source files in this directory
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
SRCS := $(wildcard *.c)

# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Make up the list of resultant object files & dependencies
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)


# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Define what we want to build and how to build it ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TARGET := $(LIB_DIR)/libmodbus.a

$(TARGET) :  $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)
	-cp modbus*.h $(INC_DIR)
	-rm $(INC_DIR)/*-private*



# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Over-ride some of make's internal implicit rules so that dependencies
# will be automatically and correctly re-made as required ...
#
# A lot of research and trial and error went into this so be VERY careful
# when modifying it. You have been warned!
#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


$(OBJ_DIR)/%.o : %.c

$(OBJ_DIR)/%.o : %.c
	$(COMPILER) $(CFLAGS) -I $(INC_DIR) -c -o $@ $<




# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Finally the project actions which are not based on actual files ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

.PHONY :  clean

clean :
	-rm -f $(OBJS)
	-rm -f $(TARGET)
	-rm -f $(INC_DIR)/modbus*
	-rm -f $(REF_DIR)/MODBUS/
	-rm -f $(DEP_DIR)/*

