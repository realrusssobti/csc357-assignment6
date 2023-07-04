CC = gcc
CLASS = -Wall -Wextra -pedantic -std=c99

all: httpd kvstore

httpd_exec: httpd
	$(CC) $(CFLAGS) -o httpd httpd.c


clean:
	rm -f httpd *.o *~ *.bak


kvstore: kvstore.c hashtable.c
	$(CC) $(CLASS) -o kvstore kvstore.c hashtable.c
