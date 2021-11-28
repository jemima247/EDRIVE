CC := clang
CFLAGS := -g -Wall -Werror -Wno-unused-function -Wno-unused-variable

all: server client

clean:
	rm -rf server client

server: server.c message.h message.c socket.h
	$(CC) $(CFLAGS) -o server server.c message.c -lform -lncurses -lpthread

client: client.c message.h message.c
	$(CC) $(CFLAGS) -o client client.c message.c -lform -lncurses -lpthread