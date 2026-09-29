# Omega rebirth with the curses shim + X11 frontend. Run from the repo root:
# make -f port/Makefile -j8   (builds ./omega; objects in build-x11/)
X11 = /opt/X11
SRCS = $(wildcard src/*.cpp) port/tiles.cpp
OBJS = $(SRCS:%.cpp=build-x11/%.o) build-x11/port/wcurses.o build-x11/port/be_x11.o
CXXFLAGS = -O2 -g -std=c++23 -Iport -DUSER_DEFINED_OMEGALIB
CFLAGS = -O2 -g -std=c99 -Iport -I$(X11)/include -I$(X11)/include/freetype2
omega: $(OBJS)
	$(CXX) $(OBJS) -L$(X11)/lib -lX11 -lXft -o omega
build-x11/%.o: %.cpp port/curses.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@
build-x11/%.o: %.c port/curses.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
