#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <stdlib.h>


int main()
{

	//getaddrinfo
	struct addrinfo       hints;
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;

	//store result 
	struct addrinfo*    result;
	int                 error_flag;
	if(0 != (error_flag = getaddrinfo(NULL, "8888", &hints, &result)))
	{
		fprintf(stderr, "error happen in getaddrinfo:%s\n", gai_strerror(error_flag));
		exit(EXIT_FAILURE);
	}


	//result for bind
	struct addrinfo*   next;
	int ser_socket;
	for (next = result; next != NULL; next = next->ai_next)
	{
		if(-1 == (ser_socket = socket(next->ai_family, next->ai_socktype, next->ai_protocol)))
		{
			continue;
		}

		if(0 == bind(ser_socket, next->ai_addr, next->ai_addrlen))
		{
			printf("bind success! \n");
			printf("port = %u", ntohs(((struct sockaddr_in *)next->ai_addr)->sin_port));
			break;
		}
		close(ser_socket);
	}
	
	if(NULL == next)
	{
		printf("there is not binding any addr\n");
		exit(EXIT_FAILURE);
	}
	
	//listen
	if(0 != listen(ser_socket, 10))
	{
		printf("error happen in listen: \n");
	}
	printf("listen success\n");

	freeaddrinfo(next);

	// accept client
	while(1)
	{
		struct sockaddr_in  Addr = { 0 };
		int               cli_sock;
		int               sock_len = sizeof(Addr);
	//	if((cli_sock = accept(ser_socket, (struct sockaddr*)&Addr, &sock_len)) < 0)
		if((cli_sock = accept(ser_socket, NULL, NULL)) < 0)
		{
			printf("error hanppen : accept\n");
		}
		printf("accept success\n");
		char buf[1024];
		while(recv(cli_sock, (void*)buf, 1024, 0) > 0)
		{
			printf("data from client: %s, server\n", buf);
			if(send(cli_sock, (void*)buf, sizeof(buf), 0) == -1)
			{
				return 1;
			}
		}
	}
	return 0;
}
