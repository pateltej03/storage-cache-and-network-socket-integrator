CC=gcc
CFLAGS=-c -Wall -I. -fpic -g -fbounds-check -fsanitize=address
LDFLAGS=-L. -fsanitize=address
LIBS=-lcrypto

OBJS=tester.o util.o mdadm.o cache.o net.o

%.o:	%.c %.h
	$(CC) $(CFLAGS) $< -o $@

tester:	$(OBJS) jbod.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(OBJS) tester
