ext=so
libprefix=lib
ifeq ($(OS),Windows_NT)
  ext=dll
  libprefix=
endif
LD=ld.exe

OPT_FLAGS ?= -O3
MODULE_DIRECTORY ?= ./cpp_modules

all: CXXFLAGS= -DNDEBUG $(OPT_FLAGS) -pthread -std=c++26  -Iinclude -MMD 
all: LDFLAGS= -shared  -Wl,--allow-multiple-definition -L`pwd` 
ifeq ($(OS),Windows_NT)
all: LDFLAGS=" -Wl,--export-all-symbols ${LDFLAGS}"
endif
all: build/src/io/json/Path.o  build/src/io/json/Binder.o
	@mkdir -p $(@D)
	#ld --help
	$(CXX) $(LDFLAGS) -o $(libprefix)jsonthresher.$(ext) $(wildcard build/src/io/json/*.o) 

build/src/io/json/Path.o: CXXFLAGS= -DNDEBUG $(OPT_FLAGS) -fPIC -pthread -std=c++26 -Iinclude -Isrc -MMD
build/src/io/json/Path.o: src/io/json/Path.cpp 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/io/json/Path.o src/io/json/Path.cpp
	
build/src/io/json/Binder.o: CXXFLAGS= -DNDEBUG $(OPT_FLAGS) -fPIC -pthread -std=c++26 -Iinclude -Isrc -I$(MODULE_DIRECTORY)/expected/include -I$(MODULE_DIRECTORY)/graph-v3/include -I$(MODULE_DIRECTORY)/include -MMD
build/src/io/json/Binder.o: src/io/json/Binder.cpp 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/io/json/Binder.o src/io/json/Binder.cpp

