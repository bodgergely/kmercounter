IDIR =.
CC=g++
CFLAGS=-I$(IDIR) -O2 -DDEBUG -pthread -g --std=c++11

ODIR=../obj

LIBS=-lm


cout: count.cpp
	g++ -o ../bin/count count.cpp $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o