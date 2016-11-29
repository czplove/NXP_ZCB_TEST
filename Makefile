# ------------------------------------------------------------------
# ZCB makefile
# ------------------------------------------------------------------
# Author:    nlv10677
# Copyright: NXP B.V. 2014. All rights reserved
# ------------------------------------------------------------------

# LDFLAGS += -ldaemon -lpthread
CFLAGS += -DTARGET_RASPBERRYPI
LDFLAGS += -lpthread -lm

TARGET = zcb_test

INCLUDES = -I./

OBJECTS = zcb_test_main.o \
	      Serial.o \
		  SerialLink.o \
	      test_utils.o

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -Wall -std=gnu99 -g -c $< -o $@


all:  build

build: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET) -lc

clean:
	-rm -f $(OBJECTS)
	-rm -f $(TARGET)