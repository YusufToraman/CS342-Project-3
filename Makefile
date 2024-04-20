CC := gcc
CFLAGS := -g -Wall

TARGETS := libmf.a app1 mfserver producer consumer test1 test2

# Make sure that 'all' is the first target
all: $(TARGETS)

MF_SRC := mf.c mq.c
MF_OBJS := $(MF_SRC:.c=.o)

libmf.a: $(MF_OBJS)
	ar rcs $@ $(MF_OBJS)

MF_LIB := -L. -lrt -lpthread -lmf

%.o: %.c mf.h mq.h
	$(CC) -c $(CFLAGS) -o $@ $<

app1: app1.o libmf.a
	$(CC) $(CFLAGS) -o $@ app1.o $(MF_LIB)

mfserver: mfserver.o libmf.a
	$(CC) $(CFLAGS) -o $@ mfserver.o $(MF_LIB)

producer: producer.o libmf.a
	$(CC) $(CFLAGS) -o $@ producer.o $(MF_LIB)

consumer: consumer.o libmf.a
	$(CC) $(CFLAGS) -o $@ consumer.o $(MF_LIB)

test1: test1.o libmf.a
	$(CC) $(CFLAGS) -o $@ test1.o $(MF_LIB)

test2: test2.o libmf.a
	$(CC) $(CFLAGS) -o $@ test2.o $(MF_LIB)

clean:
	rm -rf core *.o *.out *~ $(TARGETS)