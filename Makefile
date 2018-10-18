all:
	#$(CC) -Wall -Werror ess_server.c -O2 -lpthread -o ess_server
	$(CC) -Wall ess_server.c -O2 -lpthread -o ess_server

clean:
	$(RM) -rf ess_server
