all:
	gcc client/client.c client/parcer.c -o client_prog
	gcc server/server.c server/parcer.c -o server_prog
clean:
	rm client_prog
	rm server_prog
	ipcrm -Q 125
	ipcrm -Q 126
	rm History.txt
