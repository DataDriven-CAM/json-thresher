LD=ld.exe

all: LDFLAGS= -L../ -ljsonthresher -pthread -Wl,--allow-multiple-definition
all: test
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o test $(wildcard build/src/*.o) C:/sylvanmats/jsonthresher/jsonthresher.dll  

test: CXXFLAGS= -DNDEBUG -O3 -pthread -std=c++23 -I../include -I../cpp_modules/doctest -MMD -Wl,--allow-multiple-definition
test: LDFLAGS= 

test: src/test.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o build/src/test.o src/test.cpp 


