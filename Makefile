ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall #-Weverything -std=c++17
MAKE = gmake
GRETLLD = $(shell pkg-config --libs gretl)
GRETLIN = $(shell pkg-config --cflags gretl)
PYBIND = $(shell python3 -m pybind11 --includes)
MODULE_PREFIX = gretl
MODULE_SUFIX = $(shell python3-config --extension-suffix)
MODULE_NAME = $(MODULE_PREFIX)$(MODULE_SUFIX)

.PHONY: clean

$(MODULE_NAME): gretl_to_python.o gretl_calls.o gretl_python_data.o
	$(CXX) $(CXXFLAGS) $^ -shared -o $(MODULE_NAME) -fPIC $(PYBIND) $(GRETLLD)

gretl_python_data.o: gretl_python_data.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

gretl_calls.o: gretl_calls.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

gretl_to_python.o: gretl_to_python.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

demoscript: all demo/demo.py
	cd demo ; python3 demo.py

tags:
	etags *.cpp *.h

clean:
	@rm -f $(MODULE_NAME) *.o
