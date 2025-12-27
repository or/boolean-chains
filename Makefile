DOCKER_ARCH ?= native

# Map Docker arch → clang march flag
ifeq ($(DOCKER_ARCH),amd64)
  MARCH_FLAG = -march=x86-64
else ifeq ($(DOCKER_ARCH),arm64)
  MARCH_FLAG = -march=armv8-a
else ifeq ($(DOCKER_ARCH),arm)
  MARCH_FLAG = -march=armv7-a
else
  MARCH_FLAG =
endif

COMPILER = clang++
OPT_FLAGS = -std=c++20 -O3 $(MARCH_FLAG) -flto -ffast-math -fomit-frame-pointer -funroll-loops -fno-sanitize=all -fno-builtin-memcpy -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti

PROFILE_FLAGS = -O0 -fprofile-instr-generate=default.profraw
OPTIMIZED_PROFILE_FLAGS = -fprofile-instr-use=default.profdata

all: target/full-search target/full-search-plan target/hungry-search target/reverse-hungry-search target/reverse-full-search

target/full-search: src/full-search.cpp Makefile
	$(COMPILER) -o target/full-search src/full-search.cpp $(OPT_FLAGS) 2>&1

target/full-search-plan: src/full-search.cpp Makefile
	$(COMPILER) -DPLAN_MODE=1 -o target/full-search-plan src/full-search.cpp $(OPT_FLAGS) 2>&1

target/full-search-profile: src/full-search.cpp Makefile
	$(COMPILER) -o target/full-search-profile src/full-search.cpp $(OPT_FLAGS) $(PROFILE_FLAGS) 2>&1

target/full-search-optimized: src/full-search.cpp Makefile default.profdata
	$(COMPILER) -o target/full-search-optimized src/full-search.cpp $(OPT_FLAGS) $(OPTIMIZED_PROFILE_FLAGS) 2>&1

target/hungry-search: src/hungry-search.cpp src/*.h Makefile
	$(COMPILER) -o target/hungry-search src/hungry-search.cpp $(OPT_FLAGS) 2>&1

target/reverse-hungry-search: src/reverse-hungry-search.cpp src/*.h Makefile
	$(COMPILER) -o target/reverse-hungry-search src/reverse-hungry-search.cpp $(OPT_FLAGS) 2>&1

target/reverse-full-search: src/reverse-full-search.cpp src/*.h Makefile
	$(COMPILER) -o target/reverse-full-search src/reverse-full-search.cpp $(OPT_FLAGS) 2>&1

target/hungry-search-debug: src/hungry-search.cpp src/*.h Makefile
	$(COMPILER) -o target/hungry-search-debug src/hungry-search.cpp -std=c++20 -g 2>&1

target/full-search-debug: src/full-search.cpp src/*.h Makefile
	$(COMPILER) -o target/full-search-debug src/full-search.cpp -std=c++20 -g 2>&1

target/full-search-x86_64: src/full-search.cpp Makefile
	zig c++ src/full-search.cpp -o target/full-search-x86_64 -std=c++20 -O3 -flto -ffast-math -fomit-frame-pointer -funroll-loops -fno-sanitize=all -fno-builtin-memcpy -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti -target x86_64-linux

target/full-search-arm64: src/full-search.cpp Makefile
	zig c++ src/full-search.cpp -o target/full-search-arm64 -std=c++20 -O3 -flto -ffast-math -fomit-frame-pointer -funroll-loops -fno-sanitize=all -fno-builtin-memcpy -fno-delete-null-pointer-checks -fno-exceptions -fno-rtti -target aarch64-linux
