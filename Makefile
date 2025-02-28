all: target/boolean-chains target/boolean-chains-full-fast target/boolean-chains-full-fast-plan

target/boolean-chains: src/boolean-chains.cpp src/*.h Makefile
	clang++ -o target/boolean-chains src/boolean-chains.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-fast: src/boolean-chains-full-fast.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full-fast src/boolean-chains-full-fast.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-fast-plan: src/boolean-chains-full-fast.cpp src/*.h Makefile
	clang++ -DPLAN_MODE=1 -o target/boolean-chains-full-fast-plan src/boolean-chains-full-fast.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops 2>&1

target/boolean-chains-full-fast-profile: src/boolean-chains-full-fast.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full-fast src/boolean-chains-full-fast.cpp -std=c++20 -O0 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-generate=default.profraw 2>&1

target/boolean-chains-full-fast-optimized: src/boolean-chains-full-fast.cpp src/*.h Makefile
	clang++ -o target/boolean-chains-full-fast src/boolean-chains-full-fast.cpp -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fprofile-instr-use=default.profdata 2>&1
