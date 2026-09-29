#!/bin/sh
# Native terminal build (rebirth, C++23) against system ncurses.
# Run from a directory containing lib/ (or set OMEGALIB with -DUSER_DEFINED_OMEGALIB).
# sh build-term.sh [CXX=...] [EXE=.exe] [EXTRA=...] [TLIBS=...]
set -e
cd "$(dirname "$0")"
for a; do eval "${a%%=*}=\"\${a#*=}\""; done
${CXX:-c++} -O2 -std=c++23 -DNDEBUG $EXTRA src/*.cpp ${TLIBS:--lncurses} -o omega-term$EXE
