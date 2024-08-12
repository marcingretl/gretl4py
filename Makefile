ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall#-Weverything -std=c++17
MAKE = gmake
GRETLLD = $(shell pkg-config --libs gretl)
GRETLIN = $(shell pkg-config --cflags gretl)
PYBIND = $(shell python3 -m pybind11 --includes)
SUFFIX = $(shell python3-config --extension-suffix)
MODULE = _gretl$(SUFFIX)

VPATH = ./src

SRCS = gretl4py.cpp gretl_calls.cpp gretl4py_data.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: tags clean depend

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

$(MODULE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -shared -o gretl/$@ -fPIC $(GRETLLD)

tags:
	etags src/*.cpp src/*.h

clean:
	@rm -f gretl/$(MODULE) *.o

depend:
	makedepend -Y. *.cpp 2>/dev/null

# DO NOT DELETE

gretl_calls.o: src/gretl4py.h src/gretl4py_data.h
gretl4py_data.o: src/gretl4py_data.h
gretl4py.o: src/gretl4py.h
