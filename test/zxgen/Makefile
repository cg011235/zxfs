#############################
# zxfs (zero x file system) #
#############################

#
#	test/zxgen/Makefile
#
#	Copyright (c) 2013 cg011235 <cg011235@gmail.com>
#
#	Make file for zxgen program
#

CC=gcc
CCFLAGS=-pthread
LDFLAGS=
SOURCES="zxgen.c"
EXECUTABLE='zxgen'	

all:
	${CC} ${CCFLAGS} ${LDFLAGS} ${SOURCES} -o ${EXECUTABLE}

clean:
	rm -f ${EXECUTABLE}
