ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall -std=c++17
MAKE = gmake
GPATH = /Applications/Gretl.app/Contents/Resources
GRETLLD = -L$(GPATH)/lib -lgretl-1.0 -lglib-2.0 -lxml2 -lfftw3 -lomp -lm
GRETLIN = -I$(GPATH)/include -I$(GPATH)/include/glib-2.0 -I$(GPATH)/lib/glib-2.0/include
PYBIND = $(shell python3 -m pybind11 --includes)
MODULE_PREFIX = gretl
MODULE_SUFFIX = $(shell python3-config --extension-suffix)
MODULE = $(MODULE_PREFIX)$(MODULE_SUFFIX)

SRCS = gretl_to_python.cpp gretl_calls.cpp gretl_python_data.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: tags clean depend

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

$(MODULE): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -v -shared -undefined dynamic_lookup -o $@ -fPIC $(GRETLLD)

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
