all:
	mkdir -p ./bin
	g++ -O3 -I./include -o ./bin/msws_prng src/msws.cpp

clean:
	rm -rf ./bin
