all: target/boolean-chains target/boolean-chains-full-fast

target/boolean-chains: src/boolean-chains.cpp src/*.h Makefile
	clang++ -o target/boolean-chains src/boolean-chains.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-fast: src/boolean-chains-full-fast.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full-fast src/boolean-chains-full-fast.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1
