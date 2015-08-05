#
# GNU makefile, application portion.
#
# "make" or "make all" to build executable.
# "make clean" to delete object code.
#

#
#  Modified to try finding the f(x) = 2x function
#  Kazuto Tominaga <tomi@acm.org>
#  December 7, 2002
#

KERNELDIR = ../../kernel
CC = gcc
CFLAGS = -g -I/usr/local/include
LIBS = -L/usr/local/lib -lcpgplot -lpgplot -L/usr/X11R6/lib -lX11 -lg2c -lpng -lm
TARGET = gp

uobjects = function.o app.o lambda.o lambint.o church.o
uheaders = appdef.h app.h function.h lambda.h

include $(KERNELDIR)/GNUmakefile.kernel
