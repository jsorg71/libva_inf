
OBJS=enc1.o

CFLAGS=-O2 -g -I../include -I/opt/yami/include

LDFLAGS=-L..

LIBS=-lva_inf

enc1: $(OBJS)
	$(CC) -o enc1 $(OBJS) $(LDFLAGS) $(LIBS) -Wl,-rpath,..

clean:
	rm -f enc1 $(OBJS)
