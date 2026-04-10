
OBJS=src/va_inf.o

CFLAGS=-O2 -Wall -Wextra -fPIC -Iinclude
CFLAGS+=$(shell PKG_CONFIG_PATH=/opt/yami/lib/pkgconfig pkg-config --cflags libva-drm)

LIBS=
LIBS+=$(shell PKG_CONFIG_PATH=/opt/yami/lib/pkgconfig pkg-config --libs libva-drm)

libva_inf.so: $(OBJS)
	$(CC) -shared -o libva_inf.so $(OBJS) $(LDFLAGS) $(LIBS) -Wl,-rpath,/opt/yami/lib

clean:
	rm -f libva_inf.so $(OBJS)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $<

