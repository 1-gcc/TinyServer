
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#define SOCKET int
#define SOCK_ERROR -1

class Opt
{
public:
	
} ;
int server(unsigned short port,Opt&opt)
{
	struct sockaddr_in6 addr;
	struct sockaddr_in6 client_addr;
	memset(&addr,0,sizeof(addr));
	memset(&client_addr,0,sizeof(client_addr));
	
	SOCKET sock = socket(AF_INET6,SOCK_STREAM,0);
	if(sock == SOCK_ERROR)
	{
		fprintf(stderr,"socket error\n");
		return -1;
	}
	int sockopt = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&sockopt,sizeof(int));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;

	client_addr.sin6_port = htons(port) ;
	client_addr.sin6_addr = in6addr_any;
	socklen_t alen = sizeof(client_addr);

	int err = bind(sock,(struct sockaddr*)&addr,sizeof(addr));
	if(err == SOCK_ERROR)
	{
		fprintf(stderr,"bind error %d \n", errno);
		exit(2);
	}
	listen(sock,4);
	for(;;)
	{
		{
			char buffer[ 1024 ] ;
		printf("Accepting incoming connect <%s> port %d\n",
				inet_ntop(AF_INET6,&addr.sin6_addr,
				buffer,sizeof(buffer) - 1),
				ntohs(addr.sin6_port));
		}

		SOCKET acc = accept(sock,
				(struct sockaddr*)&client_addr,
				&alen);
		if(acc == SOCK_ERROR)
		{
			fprintf(stderr,"accept error\n") ;
			break;
		}
		else
		{
			char buffer[ 1024 ] ;
			printf("incoming connect from <%s> Port %d\n",
				inet_ntop(AF_INET6,&client_addr.sin6_addr,
				buffer,sizeof(buffer) - 1),
				ntohs(client_addr.sin6_port));
		}
		for(;;)
		{
			char buffer[ 1024 ] ;
			int ret = read(acc,buffer,sizeof(buffer));
			if(ret == 0)
				break;
			if(ret == SOCK_ERROR)
			{
				fprintf(stderr,"read error %d\n",errno);
				break;
			}
			FILE * fp = fopen("traffic.bin","ab");
			if(fp)
			{
				fwrite(buffer,1,ret,fp) ;
				fclose(fp) ;
			}
			char * resp = "GET / HTTP/1.1\r\n\r\n<html><body><h1>Data</h1></body></html>\r\n\r\n";

			write(acc,resp,strlen(resp)) ;
		}
		close(acc);
		shutdown(acc,0);
		
		
	}
}
int main(int argc,char**argv)
{
	int port = 80;
	Opt opt ;
	while(--argc)
	{
		printf("%s\n",*++argv);
		if(argc == 1)
		{
			if(sscanf(*argv	,"%d",&port) != 1)
			{
				fprintf(stderr,"parameter error\n");
				exit(1);
			}
		}

	}
	server(port,opt);
}
