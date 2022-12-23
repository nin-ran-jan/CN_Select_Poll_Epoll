all: client server

client:
	g++ client_seq.cpp -o client_seq -lpthread -w
	g++ client_thread.cpp -o client_thread -lpthread -w

server:
	g++ server_seq.cpp -o server_seq -lpthread -lrt -w
	g++ server_thread.cpp -o server_thread -lpthread -lrt -w
	g++ server_fork.cpp -o server_fork -lpthread -lrt -w
	g++ server_select.cpp -o server_select -lpthread -lrt -w
	g++ server_epoll.cpp -o server_epoll -lpthread -lrt -w
	g++ server_poll.cpp -o server_poll -lpthread -lrt -w

clean:
	rm server_seq client_seq client_thread server_thread server_fork server_select server_epoll server_poll *.txt