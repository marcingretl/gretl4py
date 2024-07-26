CC = gcc -O2 -g -Wall
GRCFLAGS = `pkg-config --cflags gretl`
GRLIBS = `pkg-config --libs gretl`

.PHONY: test clean

model_funcs: model_funcs.c
	$(CC) $(GRCFLAGS) -o $@ $< $(GRLIBS)

test: model_funcs
	./query.sh > query.out

clean:
	rm -f model_funcs query.out *~
