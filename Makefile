target/boolean-chains: src/boolean-chains.cpp src/*.h
	clang++ -o target/boolean-chains -O2 src/boolean-chains.cpp -std=c++20 -stdlib=libc++ 2>&1

all: target/boolean-chains
