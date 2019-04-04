CC=gcc
CFLAGS=-g -Wall
LDFLAGS=
DEPS=checksum.h

SRC=checksum.c icmp_ex.c ping.c
OBJ=checksum.o icmp_ex.o ping.o
BIN=icmp_ex ping

all: $(BIN)

$(BIN): $(SRC) $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) $(LDfLAGS) -c -o $@ $<

icmp_ex: checksum.o icmp_ex.o
	$(CC) $(CFLAGS) $(LDFLAGS)    -o icmp_ex checksum.o icmp_ex.o

ping: checksum.o ping.o
	$(CC) $(CFLAGS) $(LDFLAGS)    -o ping checksum.o ping.o

mySignal: mySignal.c
	gcc mySignal.c -o mySignal -pthread

clean:
	rm -f mySignal
	rm -f $(BIN) *.o
