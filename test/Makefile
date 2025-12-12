CC=gcc
CFLAGS=`pkg-config --cflags check`
LIBS=`pkg-config --libs check`
RM=rm

all: memarena

memarena: memarena.c ../src/memarena.c
	$(CC) $(CFLAGS)  memarena.c ../src/memarena.c -o memarena $(LIBS)

clean:
	$(RM) memarena
