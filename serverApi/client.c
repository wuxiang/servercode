#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int main()
{
	int cli_socket;
	if(-1 == (cli_socket = socket(AF_INET, SOCK_STREAM, 0)))
	{
		return 1;
	}

	//connect
	struct sockaddr_in        sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port   = htons(8888);
	if(1 != inet_pton(AF_INET, "192.168.154.136", &sock_addr.sin_addr))
	{
		printf("error happen : inet_pton\n");
	}
	if(0 != connect(cli_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)))
	{
		printf("error happen : connect\n");
	}
	
	// accept client
	while(1)
	{
		char buf[1024] = {"hello"};
		char buf2[1024];
		while(send(cli_socket, (void*)buf, 1024, 0) >= 0)
		{
			if(recv(cli_socket, (void*)buf2, 1024, 0) >= 0)
			{
				printf("data from server: %s, client\n", buf2);
			}
		}
	}
	return 0;
}
