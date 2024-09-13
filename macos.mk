ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall -std=c++17
MAKE = gmake
GPATH = /Applications/Gretl.app/Contents/Resources
GRETLLD = -L$(GPATH)/lib -lgretl-1.0 -lglib-2.0 -lxml2 -lfftw3 -lomp -lm
GRETLIN = -I$(GPATH)/include -I$(GPATH)/include/glib-2.0 -I$(GPATH)/lib/glib-2.0/include
PYBIND = $(shell python3 -m pybind11 --includes)
SUFFIX = $(shell python3-config --extension-suffix)
MODULE = gretl/_gretl$(SUFFIX)
ZIPFILE = gretl4py.zip

VPATH = ./src

PYFILES = gretl/gretl4py_addons.py gretl/gretl4py_classes.py
SRCS = gretl4py.cpp gretl_calls.cpp gretl4py_data.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: tags clean depend

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

$(MODULE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -v -shared -undefined dynamic_lookup -o $@ -fPIC $(GRETLLD)

$(ZIPFILE): $(MODULE) $(PYFILES)
	zip -r -q $@ gretl

install: $(ZIPFILE)
	./install.sh

tags:
	etags src/*.cpp src/*.h

clean:
	rm -f gretl/$(MODULE) *.o

depend:
	makedepend -Y. src/*.cpp 2>/dev/null

# DO NOT DELETE

src/gretl4py.o: src/gretl4py.h
src/gretl4py_data.o: src/gretl4py_data.h
src/gretl_calls.o: src/gretl4py.h src/gretl4py_data.h
