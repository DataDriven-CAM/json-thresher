ext=so
libprefix=lib
ifeq ($(OS),Windows_NT)
  ext=dll
  libprefix=
endif
LD=ld.exe

all: CXXFLAGS= -DNDEBUG -O3 -pthread -std=c++23  -Iinclude -MMD -Wl,--allow-multiple-definition
all: LDFLAGS= -shared  -Wl,--allow-multiple-definition -L`pwd`/cpp_modules/fmt/dist/lib -lfmt
ifeq ($(OS),Windows_NT)
all: LDFLAGS=" -Wl,--export-all-symbols ${LDFLAGS}"
endif
all: build/src/io/json/Binder.o 
	@mkdir -p $(@D)
	#ld --help
	$(CXX) $(LDFLAGS) -o $(libprefix)jsonthresher.$(ext) $(wildcard build/src/io/json/*.o) 

build/src/io/json/Binder.o: CXXFLAGS= -DNDEBUG -O3 -fPIC -pthread -std=c++23 -Iinclude -Isrc -I./cpp_modules/fmt/dist/include -MMD
build/src/io/json/Binder.o: src/io/json/Binder.cpp 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/io/json/Binder.o src/io/json/Binder.cpp

