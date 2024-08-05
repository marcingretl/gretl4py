ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall #-Weverything -std=c++17
MAKE = gmake
GRETLLD = $(shell pkg-config --libs gretl)
GRETLIN = $(shell pkg-config --cflags gretl)
PYBIND = $(shell python3 -m pybind11 --includes)
MODULE_PREFIX = _gretl
MODULE_SUFIX = $(shell python3-config --extension-suffix)
MODULE = $(MODULE_PREFIX)$(MODULE_SUFIX)

SRCS = gretl_to_python.cpp gretl_calls.cpp gretl_python_data.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: tags clean depend

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

$(MODULE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -shared -o $@ -fPIC $(GRETLLD)

tags:
	etags *.cpp *.h

clean:
	@rm -f $(MODULE) *.o

depend:
	makedepend -Y. *.cpp 2>/dev/null

# DO NOT DELETE

gretl_calls.o: gretl_calls.h gretl_python_data.h
gretl_python_data.o: gretl_python_data.h
gretl_to_python.o: gretl_calls.h
