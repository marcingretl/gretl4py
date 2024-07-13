ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -std=c++17 -g -Wall #-Weverything
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
	$(CXX) $(CXXFLAGS) -c gretl_python_data.cpp -o gretl_python_data.o -fPIC $(PYBIND) $(GRETLIN)

.PHONY: part2
part1:
gretl_calls.o: gretl_calls.cpp
	$(CXX) $(CXXFLAGS) -c gretl_calls.cpp -o gretl_calls.o -fPIC $(PYBIND) $(GRETLIN)

$(MODULE_NAME): gretl_to_python.cpp gretl_calls.o gretl_python_data.o
	$(CXX) $(CXXFLAGS) gretl_to_python.cpp gretl_calls.o gretl_python_data.o -shared -o $(MODULE_NAME) -fPIC $(PYBIND) $(GRETLLD)

demoscript: all demo/demo.py
	cd demo ; python3 demo.py

clean:
	@rm -f $(MODULE_NAME) gretl_calls.o gretl_python_data.o
