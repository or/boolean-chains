all: target/boolean-chains target/boolean-chains2

target/boolean-chains: src/boolean-chains.cpp src/*.h Makefile
	clang++ -o target/boolean-chains src/boolean-chains.cpp -std=c++20 -stdlib=libc++ -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains2: src/boolean-chains2.cpp src/*.h Makefile
	clang++ -o target/boolean-chains2 src/boolean-chains2.cpp -std=c++20 -stdlib=libc++ -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1
