CC	:= gcc
CFLAGS := -g -Wall

TARGETS := libmf.a app1 mfserver

# Make sure that 'all' is the first target
all: $(TARGETS)

MF_SRC := mf.c mq.c
MF_OBJS := $(MF_SRC:.c=.o)

libmf.a: $(MF_OBJS)
	ar rcs $@ $(MF_OBJS)

MF_LIB := -L. -lrt -lpthread -lmf

mf.o: mf.c mf.h mq.h
	gcc -c $(CFLAGS) -o $@ mf.c

mq.o: mq.c mq.h
	gcc -c $(CFLAGS) -o $@ mq.c

app1.o: app1.c mf.h mq.h
	gcc -c $(CFLAGS) -o $@ app1.c

app1: app1.o libmf.a
	gcc $(CFLAGS) -o $@ app1.o $(MF_LIB)

mfserver.o: mfserver.c mf.h mq.h
	gcc -c $(CFLAGS) -o $@ mfserver.c

mfserver: mfserver.o libmf.a
	gcc $(CFLAGS) -o $@ mfserver.o $(MF_LIB)

test: test.c
	gcc -g -Wall -o test test.c

clean:
	rm -rf core *.o *.out *~ $(TARGETS)

	
	
