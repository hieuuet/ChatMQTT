
CC = gcc
CFLAGS = -Wall -pthread

s:
	$(CC) $(CFLAGS) gcc -pthread server.c packet.c db.c -l sqlite3 -o s

db:
	$(CC) $(CFLAGS) dbtest.c db.c packet.c -l sqlite3 -o db 

c:
	rm -rf s db
