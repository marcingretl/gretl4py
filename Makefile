CC = clang
CXX = clang++
CFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall #-Weverything
CXXFLAGS = -march=native -mtune=native -std=c++11 -O3 -pipe -g -Wall#-Weverything
MAKE = gmake
GRETLLD = $(shell pkg-config --libs gretl)
GRETLIN = $(shell pkg-config --cflags gretl)
PYBIND = $(shell python3 -m pybind11 --includes)
MODULE_PREFIX = gretl
MODULE_SUFIX = $(shell python3-config --extension-suffix)
MODULE_NAME = $(MODULE_PREFIX)$(MODULE_SUFIX)

.PHONY: clean

all: part1 part2 $(MODULE_NAME)

.PHONY: part1
part1:
gretl_python_data.o: gretl_python_data.cpp
	ccache $(CXX) $(CXXFLAGS) -c gretl_python_data.cpp -o gretl_python_data.o -fPIC $(PYBIND) $(GRETLIN)

.PHONY: part2
part1:
gretl_calls.o: gretl_calls.cpp
	ccache $(CXX) $(CXXFLAGS) -c gretl_calls.cpp -o gretl_calls.o -fPIC $(PYBIND) $(GRETLIN)

$(MODULE_NAME): gretl_to_python.cpp gretl_calls.o gretl_python_data.o
	ccache $(CXX) $(CXXFLAGS) gretl_to_python.cpp gretl_calls.o gretl_python_data.o -shared -o $(MODULE_NAME) -fPIC $(PYBIND) $(GRETLLD)

clean:
	@rm -f $(MODULE_NAME) gretl_calls.o gretl_python_data.o