CROSS_COMPILE 	?=
CROSS_PREFIX	?=
CC		:= $(CROSS_COMPILE)gcc
CXX		:= $(CROSS_COMPILE)g++
CFLAGS 	?= -I. -g -O0 -std=gnu11

OPENCV_LIB = $(shell pkg-config --cflags --libs opencv)
ifeq ($(OPENCV_LIB), )
OPENCV_LIB = $(shell pkg-config --cflags --libs opencv4)
endif

CXXFLAGS?= -I. -g -std=gnu++11 ${OPENCV_LIB}
LDFLAGS	?=
LIBS	:= -larducam_mipicamera -lpthread #-lwiringPi
OLIB	:= lib
examples:= video
%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(examples)

video: video.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	-rm -f *.o
	-rm -f $(examples)

.PHONY: install

install: 
	sudo install -m 644 $(OLIB)/libarducam_mipicamera.so /usr/lib/

