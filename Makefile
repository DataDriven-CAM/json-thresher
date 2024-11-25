ext=so
libprefix=lib
ifeq ($(OS),Windows_NT)
  ext=dll
  libprefix=
endif
LD=ld.exe

all: CXXFLAGS= -DNDEBUG -O3 -pthread -std=c++26  -Iinclude -MMD 
all: LDFLAGS= -shared  -Wl,--allow-multiple-definition -L`pwd` 
ifeq ($(OS),Windows_NT)
all: LDFLAGS=" -Wl,--export-all-symbols ${LDFLAGS}"
endif
all: build/src/io/json/Path.o  build/src/io/json/Binder.o
	@mkdir -p $(@D)
	#ld --help
	$(CXX) $(LDFLAGS) -o $(libprefix)jsonthresher.$(ext) $(wildcard build/src/io/json/*.o) 

build/src/io/json/Path.o: CXXFLAGS= -DNDEBUG -O3 -fPIC -pthread -std=c++26 -Iinclude -Isrc -I./cpp_modules/fmt/dist/include -MMD
build/src/io/json/Path.o: src/io/json/Path.cpp 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/io/json/Path.o src/io/json/Path.cpp
	
build/src/io/json/Binder.o: CXXFLAGS= -DNDEBUG -O3 -fPIC -pthread -std=c++26 -Iinclude -Isrc -I./cpp_modules/fmt/dist/include -I./cpp_modules/graph-v2/include -MMD
build/src/io/json/Binder.o: src/io/json/Binder.cpp 
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/io/json/Binder.o src/io/json/Binder.cpp

