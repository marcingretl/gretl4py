CC = gcc -O2 -g -Wall
GCFLAGS = `pkg-config --cflags glib-2.0`
GLIBS = `pkg-config --libs glib-2.0`

.PHONY: test clean

model_funcs: model_funcs.c
	$(CC) $(GCFLAGS) -o $@ $< $(GLIBS)

test: model_funcs
	./query.sh > query.out

clean:
	rm -f model_funcs query.out *~
