#!/bin/sh
# Build Omega rebirth for the browser (Emscripten + Asyncify) into web/dist;
# port/be_web.c + web/omega.js draw the text screen. Deploy with web/deploy.sh.
# Objects in build-web/ (rebuilt when older than their source or port/curses.h).
set -e
cd "$(dirname "$0")/.."
OUT=web/dist OBJ=build-web
rm -rf "$OUT" && mkdir -p "$OUT" "$OBJ/src" "$OBJ/port"
# exit() -> wc_exit (be_web.c): end the page instead of the runtime.
# USER_DEFINED_OMEGALIB: OMEGALIB=/omegalib/ from web/omega.js.
CXXFLAGS="-O2 -std=c++23 -fexceptions -Iport -Dexit=wc_exit -DUSER_DEFINED_OMEGALIB"
for f in src/*.cpp port/tiles.cpp port/wcurses.c port/be_web.c; do
	o=$OBJ/${f%.*}.o
	[ "$o" -nt "$f" ] && [ "$o" -nt port/curses.h ] || echo "$f $o"
done | xargs -n 2 -P "$(sysctl -n hw.ncpu 2>/dev/null || nproc)" sh -c '
	case $0 in *.c) exec emcc -O2 -std=c99 -Iport -c "$0" -o "$1";;
	*) exec em++ '"$CXXFLAGS"' -c "$0" -o "$1";; esac'
em++ -O2 -fexceptions $OBJ/src/*.o $OBJ/port/*.o \
	--preload-file lib@/omegalib --exclude-file "lib/saves" -o "$OUT/omega-core.js" \
	-sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 -sSTACK_SIZE=2097152 \
	-sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=64MB \
	-sEXPORTED_FUNCTIONS=_main \
	-sEXPORTED_RUNTIME_METHODS=FS,IDBFS,ENV,addRunDependency,removeRunDependency \
	-sFORCE_FILESYSTEM -lidbfs.js -sENVIRONMENT=web
cp web/index.html web/omega.js web/tiles.png web/gromega.png "$OUT/"
# text fonts: the index page's fonts/ (served at ../fonts/ next to the games)
if [ -d ~/Games/roguelikes-index/fonts ]; then
	(cd ~/Games/roguelikes-index/fonts && ls *.woff | sed 's/\.woff$//') | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read().split()))' > "$OUT/fonts.json"
else echo '[]' > "$OUT/fonts.json"; fi
# help page: ~/Desktop/.../build-docs.py + lib/help12.txt (make-help.py)
python3 web/make-help.py > "$OUT/help.html" || { echo "build.sh: no help.html (make-help.py failed)" >&2; rm -f "$OUT/help.html"; }
ls -la "$OUT"
