# Omega with the curses shim + X11 frontend. Run from the repo root:
# make -f port/Makefile  (builds ./omega)
SRCS = $(filter-out compress.c fixstr.c,$(wildcard *.c))
override CFLAGS += -O2 -g -DUNIX -DSYSV -DOMEGA_SHIM -Dusleep=wc_usleep -Iport -I/opt/X11/include -I/opt/X11/include/freetype2 \
	-std=gnu89 -w -fcommon -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -Wno-int-conversion -Wno-incompatible-pointer-types
omega: $(SRCS) port/wcurses.c port/tiles.c port/be_x11.c port/curses.h *.h
	$(CC) $(CFLAGS) $(SRCS) port/wcurses.c port/tiles.c port/be_x11.c -L/opt/X11/lib -lX11 -lXft -o omega
