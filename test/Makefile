#LD=ld.exe

all: LDFLAGS= -fuse-ld=gold -L../ -ljsonthresher -pthread -Wl,--allow-multiple-definition
all: build/src/test.o test

build/src/test.o: CXXFLAGS= -Wl,--allow-multiple-definition -DNDEBUG -O3 -pthread -std=c++23 -I../include -I../cpp_modules/doctest -I../cpp_modules/fmt/dist/include -MMD
build/src/test.o: src/test.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/test.o src/test.cpp 

test: LDFLAGS= -fuse-ld=gold -L../ -ljsonthresher -pthread -Wl,--allow-multiple-definition
test: src/test.cpp
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $(wildcard build/src/*.o) -o test  


