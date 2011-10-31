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
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;

	//store result 
	struct addrinfo*    result;
	int                 error_flag;
	if(0 != (error_flag = getaddrinfo("192.168.154.136", "8888", &hints, &result)))
	{
		fprintf(stderr, "error happen in getaddrinfo:%s\n", gai_strerror(error_flag));
		exit(EXIT_FAILURE);
	}


	//result for connect
	struct addrinfo*   next;
	int ser_socket;
	for(next = result; next != NULL; next = next->ai_next)
	{
		if(-1 == (ser_socket = socket(next->ai_family, next->ai_socktype, next->ai_protocol)))
		{
			continue;
		}

		if(-1 != connect(ser_socket, next->ai_addr, next->ai_addrlen))
		{
			printf("connect success! \n");
			break;
		}
		close(ser_socket);
	}
	
	if(NULL == next)
	{
		printf("there is not connecting\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(next);
	
	// accept client
	while(1)
	{
		char buf[1024] = {"hello"};
		char buf2[1024];
		while(send(ser_socket, (void*)buf, 1024, 0) >= 0)
		{
			if(recv(ser_socket, (void*)buf2, 1024, 0) >= 0)
			{
				printf("data from server: %s, client\n", buf2);
			}
		}
	}
	return 0;
}
