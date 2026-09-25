#!/bin/sh
# Omega 0.80.2 (curses shim + X11). Saves, .omegarc: save/.
cd "$(dirname "$0")"
D=$(pwd)
mkdir -p save
export XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}"
export HOME="$D/save" OMEGALIB=../omegalib/
export OMEGA_LINES="${OMEGA_LINES:-40}" OMEGA_TEXT="${OMEGA_TEXT:-20}"
cd save
[ -f omega.sav ] && exec "$D/omega" omega.sav
exec "$D/omega"
