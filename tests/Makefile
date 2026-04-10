
CFLAGS=-Wall -Wextra -O2 -g -I../include -I/opt/yami/include

LDFLAGS=-L..

LIBS=-lva_inf

enc1: enc1.o
	$(CC) -o enc1 enc1.o $(LDFLAGS) $(LIBS) -Wl,-rpath,..

enc2: enc2.o
	$(CC) -o enc2 enc2.o $(LDFLAGS) $(LIBS) -lstdc++ -lpthread -lm -lopenh264 -Wl,-rpath,..

enc3: enc3.o
	$(CC) -o enc3 enc3.o -L/opt/yami/lib -lyami_inf -lstdc++ -lpthread -lm -lopenh264 -Wl,-rpath,/opt/yami/lib

clean:
	rm -f enc1 $(OBJS)
