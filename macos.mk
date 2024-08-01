ifndef CXX
CXX = g++
endif
CXXFLAGS = -march=native -mtune=native -O3 -pipe -g -Wall -std=c++17
MAKE = gmake
GPATH = /Applications/Gretl.app/Contents/Resources
GRETLLD = -L$(GPATH)/lib -lgretl-1.0 -lglib-2.0 -lxml2 -lfftw3 -lomp -lm
GRETLIN = -I$(GPATH)/include -I$(GPATH)/include/glib-2.0 -I$(GPATH)/lib/glib-2.0/include
PYBIND = $(shell python -m pybind11 --includes)
MODULE_PREFIX = gretl
MODULE_SUFFIX = $(shell python-config --extension-suffix)
MODULE_NAME = $(MODULE_PREFIX)$(MODULE_SUFFIX)

.PHONY: tags clean depend

$(MODULE_NAME): gretl_to_python.o gretl_calls.o gretl_python_data.o
	$(CXX) $(CXXFLAGS) $^ -v -shared -undefined dynamic_lookup -o $(MODULE_NAME) -fPIC $(PYBIND) $(GRETLLD)
	./macos/fixlinks.sh $(MODULE_NAME)

gretl_python_data.o: gretl_python_data.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

gretl_calls.o: gretl_calls.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

gretl_to_python.o: gretl_to_python.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -fPIC $(PYBIND) $(GRETLIN)

tags:
	etags *.cpp *.h

clean:
	@rm -f $(MODULE_NAME) *.o

depend:
	makedepend -Y. *.cpp 2>/dev/null

# DO NOT DELETE

gretl_calls.o: gretl_calls.h gretl_python_data.h
gretl_python_data.o: gretl_python_data.h
gretl_to_python.o: gretl_calls.h
