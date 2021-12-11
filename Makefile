CC := clang
CFLAGS := -g -Wall -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address

all: server client

clean:
	rm -rf server client

server: server.c message.h message.c socket.h file.c file.h
	$(CC) $(CFLAGS) -o server server.c message.c file.c -lform -lncurses -lpthread

client: client.c message.h message.c file.c file.h
	$(CC) $(CFLAGS) -o client client.c message.c file.c -lform -lncurses -lpthread


zip:
	@echo "Generating Project.zip file to submit to Gradescope..."
	@zip -q -r Project.zip . -x .git/\* .vscode/\* .clang-format .gitignore Project
	@echo "Done. Please upload Project.zip to Gradescope."