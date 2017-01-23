

TARGET_DIR := ../bin

NUM_PRG=18
NAME_PRG=prog_$(NUM_PRG)

ESDK=${EPIPHANY_HOME}
ETOOLS=${ESDK}/tools

SRC_DIR := ../src

SUBMAKEFILES := bj-host.mk bj-ecore.mk 

default: all

install: 
	@echo "Copy the files in ../bin to the desired install directories."

help: 
	@echo "See documentation in <base_dir>/docs."
