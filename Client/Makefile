
CC = gcc
CFLAGS = -Wall

cl:
	$(CC) $(CFLAGS) -pthread client.c packet.c -o cl

# test encode va decode file packet
t:
	$(CC) $(CFLAGS) pktest.c packet.c -o t

c:
	rm -rf cl t