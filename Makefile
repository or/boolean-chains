OPT_FLAGS = -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fno-sanitize=all -fno-builtin-memcpy -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti

PROFILE_FLAGS = -O0 -fprofile-instr-generate=default.profraw
OPTIMIZED_PROFILE_FLAGS = -fprofile-instr-use=default.profdata

all: target/boolean-chains target/boolean-chains-full-fast target/boolean-chains-full-fast-plan

target/boolean-chains: src/boolean-chains.cpp src/*.h Makefile
	clang++ -o target/boolean-chains src/boolean-chains.cpp $(OPT_FLAGS) 2>&1

target/boolean-chains-full-fast: src/boolean-chains-full-fast.cpp Makefile
	clang++ -o target/boolean-chains-full-fast src/boolean-chains-full-fast.cpp $(OPT_FLAGS) 2>&1

target/boolean-chains-full-fast-plan: src/boolean-chains-full-fast.cpp Makefile
	clang++ -DPLAN_MODE=1 -o target/boolean-chains-full-fast-plan src/boolean-chains-full-fast.cpp $(OPT_FLAGS) 2>&1

target/boolean-chains-full-fast-profile: src/boolean-chains-full-fast.cpp Makefile
	clang++ -o target/boolean-chains-full-fast-profile src/boolean-chains-full-fast.cpp $(OPT_FLAGS) $(PROFILE_FLAGS) 2>&1

target/boolean-chains-full-fast-optimized: src/boolean-chains-full-fast.cpp Makefile
	clang++ -o target/boolean-chains-full-fast-optimized src/boolean-chains-full-fast.cpp $(OPT_FLAGS) $(OPTIMIZED_PROFILE_FLAGS) 2>&1
