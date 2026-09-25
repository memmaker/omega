#!/bin/sh
# Build Omega for the browser (Emscripten + Asyncify) into web/dist;
# port/be_web.c + web/omega.js draw the text screen. Deploy with web/deploy.sh.
set -e
cd "$(dirname "$0")/.."
OUT=web/dist
rm -rf "$OUT" && mkdir -p "$OUT"
SRCS=$(ls *.c | grep -v -e '^compress.c$' -e '^fixstr.c$')
emcc -O2 -std=gnu89 -w -fcommon -DUNIX -DSYSV -DOMEGA_SHIM -Dusleep=wc_usleep -Iport \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion \
	-Wno-error=incompatible-pointer-types -Wno-error=return-mismatch \
	$SRCS port/wcurses.c port/tiles.c port/be_web.c \
	--preload-file omegalib@/omegalib -o "$OUT/omega-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 -sSTACK_SIZE=1048576 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=32MB \
	-sEXPORTED_FUNCTIONS=_main \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,ENV,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web
cp web/index.html web/omega.js web/tiles.png "$OUT/"
python3 web/make-help.py > "$OUT/help.html"
ls -la "$OUT"
