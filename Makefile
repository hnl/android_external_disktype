###
###	Makefile for disktype
###

RM = rm -f
CC = gcc

OBJS   = main.o lib.o \
         buffer.o file.o cdimage.o compressed.o \
         detect.o apple.o amiga.o atari.o dos.o cdrom.o unix.o archives.o
TARGET = disktype

CPPFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS   = -Wall
LDFLAGS  =
LIBS     =

system = $(shell uname)
ifeq ($(system),Darwin)
  CPPFLAGS += -DUSE_MACOS_TYPE
  LIBS     += -framework CoreServices
endif

# real making

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

$(OBJS): %.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

# cleanup

clean:
	$(RM) *.o *~ *% $(TARGET)

distclean: clean
	$(RM) .depend

# automatic dependencies

depend: dep
dep:
	for i in $(OBJS:.o=.c) ; do $(CC) $(CPPFLAGS) -MM $$i ; done > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif

# eof
