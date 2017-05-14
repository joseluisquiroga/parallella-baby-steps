#!/bin/bash

# Last update: 2016 dec 19. (JLQ).

ESDK=${EPIPHANY_HOME}
ELIBS="-L ${ESDK}/tools/host/lib"
EINCS="-I ${ESDK}/tools/host/include"
ELDF=${ESDK}/bsps/current/fast.ldf

SCRIPT=$(readlink -f "$0")
EXEPATH=$(dirname "$SCRIPT")
cd $EXEPATH

PWD_VAL=`pwd`
echo "PWD="$PWD_VAL
echo "EXEPATH="$EXEPATH

# clean all
rm dev_test.elf dev_test.o core_loader_znq.o DEST_shd_mem_dump.dat SOURCE_shd_mem_dump.dat

set -e

if [ -z "${CROSS_COMPILE+xxx}" ]; then
case $(uname -p) in
	arm*)
		# Use native arm compiler (no cross prefix)
		CROSS_COMPILE=
		;;
	   *)
		# Use cross compiler
		CROSS_COMPILE="arm-linux-gnueabihf-"
		;;
esac
fi

# Build HOST side application
${CROSS_COMPILE}gcc -Wall -c core_loader_znq.c ${EINCS} ${ELIBS} 
${CROSS_COMPILE}gcc -Wall -c dev_test.c ${EINCS} ${ELIBS} 
${CROSS_COMPILE}gcc -o dev_test.elf dev_test.o core_loader_znq.o ${ELIBS} -le-hal -le-loader -lpthread



