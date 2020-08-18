CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
# LDFLAGS=-Wl,-v

9cc: $(OBJS)
		$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc tests/extern.o
		./9cc -o tmp.s tests/tests.c
		#./9cc -o tmp2.s example/8queensproblem.c
		gcc -xc -c -o tmp2.o ./example/8queensproblem.c
		gcc -static -o tmp tmp.s tmp2.o tests/extern.o
		./tmp

clean:
		rm -rf 9cc *.o *~ tmp* tests/*~ tests/*.o

.PHONY: test clean
