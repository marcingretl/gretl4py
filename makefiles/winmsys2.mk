ifndef CXX
 CXX = g++
endif
ifeq ($(CXX), clang++)
 CXXFLAGS = -std=c++17 -fms-extensions -fdelayed-template-parsing
else
 CXXFLAGS = -std=c++17
endif

MAKE = gmake
GRETLLD = $(shell pkg-config --libs gretl)
GRETLIN = $(shell pkg-config --cflags gretl)
PYBIND = $(shell python3 -m pybind11 --includes)
PYTHONLD = $(shell python3-config --embed --ldflags)
PYTHONIN = $(shell python3-config --cflags)
SUFFIX = $(shell python3-config --extension-suffix)
MODULE = gretl/_gretl$(SUFFIX)
ZIPFILE = gretl4py.zip

VPATH = ./src ./src/simdjson

PYFILES = gretl/gretl4py_addons.py gretl/gretl4py_classes.py
SRCS = gretl4py.cpp gretl_calls.cpp gretl4py_data.cpp simdjson.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: tags clean depend

%.o: %.cpp gretl4py.h gretl4py_data.h gretl4py.h
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN) $(PYTHONIN)

$(MODULE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -shared -o $@ -fPIC $(GRETLLD) $(PYTHONLD)

$(ZIPFILE): $(MODULE) $(PYFILES)
	zip -r $@ gretl

install: $(ZIPFILE)
	./install.sh

tags:
	etags src/*.cpp src/*.h

clean:
	@rm -f $(MODULE) *.o $(ZIPFILE)

depend:
	makedepend -Y. src/*.cpp 2>/dev/null

# DO NOT DELETE

src/gretl4py.o: src/gretl4py.h
src/gretl4py_data.o: src/gretl4py_data.h src/simdjson/simdjson.h
src/gretl_calls.o: src/gretl4py.h src/gretl4py_data.h src/simdjson/simdjson.h
