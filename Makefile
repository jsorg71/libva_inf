
OBJS=src/va_inf.o

CFLAGS=-O2 -Wall -Wextra -fPIC -Iinclude
CFLAGS+=$(shell pkg-config --cflags libva-drm)

LDFLAGS=

LIBS=
LIBS+=$(shell pkg-config --libs libva-drm)

libva_inf.so: $(OBJS)
	$(CC) -shared -o libva_inf.so $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f libva_inf.so $(OBJS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $<

