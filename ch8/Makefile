
BUILDLIBS=`net-snmp-config --libs`
CFLAGS=-g -O0
OBJS=nmsapp.o list.o tools.o

all:$(OBJS)
	gcc -o nmsapp $(CFLAGS) $(BUILDLIBS) $(OBJS)

clean:
	rm -f *.o nmsapp
