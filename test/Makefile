MODULE_DIRECTORY ?= ../cpp_modules

#LD=ld.exe

OPT_FLAGS ?= -O3

-include ../build/src/io/json/Binder.d
-include ../build/src/io/json/Path.d

all: LDFLAGS= -fuse-ld=gold -L../ -ljsonthresher -pthread
all: build/src/compile-time-test.o build/src/test.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $(wildcard build/src/*.o) -o test  

build/src/compile-time-test.o: CXXFLAGS= -DNDEBUG $(OPT_FLAGS) -freflection -pthread -std=c++26 -I../include -I$(MODULE_DIRECTORY)/expected/include -I$(MODULE_DIRECTORY)/graph-v3/include  -I$(MODULE_DIRECTORY)/urlcpp -I$(MODULE_DIRECTORY)/fmt/dist/include -I$(MODULE_DIRECTORY)/doctest  -MMD
build/src/compile-time-test.o: LDFLAGS= 
build/src/compile-time-test.o: src/compile-time-test.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/compile-time-test.o src/compile-time-test.cpp


build/src/test.o: CXXFLAGS= -DNDEBUG $(OPT_FLAGS) -pthread -std=c++26 -I../include -I$(MODULE_DIRECTORY)/doctest -I$(MODULE_DIRECTORY)/expected/include -I$(MODULE_DIRECTORY)/graph-v3/include -I$(MODULE_DIRECTORY)/fmt/dist/include -MMD
build/src/test.o: src/test.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/test.o src/test.cpp 



