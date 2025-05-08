OPT_FLAGS = -std=c++20 -O3 -march=native -flto -ffast-math -fomit-frame-pointer -funroll-loops -fno-sanitize=all -fno-builtin-memcpy -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti

PROFILE_FLAGS = -O0 -fprofile-instr-generate=default.profraw
OPTIMIZED_PROFILE_FLAGS = -fprofile-instr-use=default.profdata

all: target/full-search target/full-search-plan target/hungry-search target/reverse-hungry-search

target/full-search: src/full-search.cpp Makefile
	clang++ -o target/full-search src/full-search.cpp $(OPT_FLAGS) 2>&1

target/full-search-plan: src/full-search.cpp Makefile
	clang++ -DPLAN_MODE=1 -o target/full-search-plan src/full-search.cpp $(OPT_FLAGS) 2>&1

target/full-search-profile: src/full-search.cpp Makefile
	clang++ -o target/full-search-profile src/full-search.cpp $(OPT_FLAGS) $(PROFILE_FLAGS) 2>&1

target/full-search-optimized: src/full-search.cpp Makefile
	clang++ -o target/full-search-optimized src/full-search.cpp $(OPT_FLAGS) $(OPTIMIZED_PROFILE_FLAGS) 2>&1

target/hungry-search: src/hungry-search.cpp src/*.h Makefile
	clang++ -o target/hungry-search src/hungry-search.cpp $(OPT_FLAGS) 2>&1

target/reverse-hungry-search: src/reverse-hungry-search.cpp src/*.h Makefile
	clang++ -o target/reverse-hungry-search src/reverse-hungry-search.cpp $(OPT_FLAGS) 2>&1

target/hungry-search-debug: src/hungry-search.cpp src/*.h Makefile
	clang++ -o target/hungry-search-debug src/hungry-search.cpp -std=c++20 -g 2>&1
