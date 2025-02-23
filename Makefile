all: target/boolean-chains target/boolean-chains-full target/boolean-chains-full-10

target/boolean-chains: src/boolean-chains.cpp src/*.h Makefile
	clang++ -o target/boolean-chains src/boolean-chains.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full: src/boolean-chains-full.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full src/boolean-chains-full.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-profile:
	clang++ -o target/boolean-chains-full src/boolean-chains-full.cpp -std=c++20 -O0 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-generate=default.profraw 2>&1

target/boolean-chains-full-optimized:
	clang++ -o target/boolean-chains-full src/boolean-chains-full.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-use=default.profdata 2>&1

target/boolean-chains-full-10: src/boolean-chains-full-10.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full-10 src/boolean-chains-full-10.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-10-profile:
	clang++ -o target/boolean-chains-full-10 src/boolean-chains-full-10.cpp -std=c++20 -O0 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-generate=default.profraw 2>&1

target/boolean-chains-full-10-optimized:
	clang++ -o target/boolean-chains-full-10 src/boolean-chains-full-10.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-use=default.profdata 2>&1
