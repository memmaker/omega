#!/bin/sh
# Native terminal build (release): the curses shim drawn by port/be_term.c
# with ANSI escapes, no curses library. Run it next to omegalib/ (or set
# OMEGALIB). sh build-term.sh [CC=...] [EXE=.exe] [EXTRA=...] [TLIBS=...]
set -e
cd "$(dirname "$0")"
for a; do eval "${a%%=*}=\"\${a#*=}\""; done
SRCS=$(ls *.c | grep -v -e '^compress.c$' -e '^fixstr.c$')
${CC:-cc} -O2 -std=gnu89 -w -fcommon -DUNIX -DSYSV -DOMEGA_SHIM -Dusleep=wc_usleep -Dexit=wc_exit -Iport \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion \
	-Wno-error=incompatible-pointer-types $EXTRA \
	$SRCS port/wcurses.c port/tiles.c port/be_term.c $TLIBS -o omega-term$EXE
