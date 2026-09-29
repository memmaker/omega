#!/bin/sh
# Omega rebirth (curses shim + X11): make -f port/Makefile, then ./play.sh.
# Data: lib/ (OMEGALIB; saves in lib/saves/<user>/). ~/.omega.toml: save/.
cd "$(dirname "$0")"
D=$(pwd)
mkdir -p save
export XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}"
export HOME="$D/save" OMEGALIB="$D/lib/" OMEGA_BMP="$D/port/tiles.bmp"
export OMEGA_LINES="${OMEGA_LINES:-36}" OMEGA_TEXT="${OMEGA_TEXT:-20}"
exec "$D/omega"
