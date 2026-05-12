
LIB_PREFIX=/opt/yami

CFLAGS=-Wall -Wextra -O2 -g -I$(LIB_PREFIX)/include

LDFLAGS=-L$(LIB_PREFIX)/lib

LIBS=-lva_inf

enc1: enc1.o
	$(CC) -o enc1 enc1.o $(LDFLAGS) $(LIBS) -Wl,-rpath,$(LIB_PREFIX)/lib

enc2: enc2.o
	$(CC) -o enc2 enc2.o $(LDFLAGS) $(LIBS) -lstdc++ -lpthread -lm -lopenh264 -Wl,-rpath,$(LIB_PREFIX)/lib

enc3: enc3.o
	$(CC) -o enc3 enc3.o -L$(LIB_PREFIX)/lib -lyami_inf -lstdc++ -lpthread -lm -lopenh264 -Wl,-rpath,$(LIB_PREFIX)/lib

gen_nv12: gen_nv12.o
	$(CC) -o gen_nv12 gen_nv12.o

clean:
	rm -f enc1
