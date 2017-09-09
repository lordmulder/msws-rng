MARCH ?= native
MTUNE ?= native

CXXFLAGS = -static -O3 -ffast-math -fomit-frame-pointer -DNDEBUG -march=$(MARCH) -mtune=$(MTUNE)

all:
	mkdir -p ./bin
	g++ $(CXXFLAGS) -I./include -o ./bin/msws_prng src/msws.cpp
	strip ./bin/msws_prng

clean:
	rm -rf ./bin
