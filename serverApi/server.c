#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int main()
{
	int ser_socket;
	if(-1 == (ser_socket = socket(AF_INET, SOCK_STREAM, 0)))
	{
		return 1;
	}

	//bind socket to address
	struct sockaddr_in        sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port   = htons(8888);
	if(1 != inet_pton(AF_INET, "192.168.154.136", &sock_addr.sin_addr))
	{
		printf("error happen : inet_pton\n");
	}
	if(0 != bind(ser_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)))
	{
		printf("error happen : bind\n");
	}
	
	//listen
	if(0 != listen(ser_socket, 10))
	{
		printf("error happen : listen\n");
	}

	// accept client
	while(1)
	{
		struct sockaddr_in  Addr;
		int               cli_sock;
		//int               sock_len;
		socklen_t         sock_len;
		if((cli_sock = accept(ser_socket, (struct sockaddr*)&Addr, &sock_len)) < 0)
		{
			printf("error hanppen : accept\n");
		}
		char buf[1024];
		while(recv(cli_sock, (void*)buf, 1024, 0) > 0)
		{
			printf("data from client: %s, server\n", buf);
			shutdown(cli_sock, 2);
			if(send(cli_sock, (void*)buf, sizeof(buf), 0) == -1)
			{
				return 1;
			}
		}
	}
	return 0;
}
