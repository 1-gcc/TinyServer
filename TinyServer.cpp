
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <map>

#include "Logger.h"

#define SOCKET int
#define SOCK_ERROR -1

Logger log;

class Opt
{
public:

} ;

class ThreadInfo;

typedef std::map<pthread_t,ThreadInfo*> ThreadMap;

class ThreadInfo
{
public:
	SOCKET acc;
	pthread_t threadId;
	int threadNr;
	ThreadMap * threadMap;
	ThreadInfo(ThreadMap*p,SOCKET acc,int nr)
	{
		this->acc = acc;
		threadMap = p;
		threadNr = nr;
	}
} ;

void * threadFun(void * p);

class Thread {
public:

	pthread_t myPthread;
	bool suspended;
	pthread_mutex_t m_SuspendMutex;
	pthread_cond_t m_ResumeCond;

	void start() {
		suspended = false;
		pthread_create(&myPthread, NULL, threadFun, (void*)this );
	}

	Thread() { }

	void suspendMe() {
		pthread_mutex_lock(&m_SuspendMutex);
		suspended = true;
		do {
			pthread_cond_wait(&m_ResumeCond, &m_SuspendMutex);
		} while (suspended);
		pthread_mutex_unlock(&m_SuspendMutex);
	}

	void resume() {
		pthread_mutex_lock(&m_SuspendMutex);
		suspended = false;
		pthread_cond_signal(&m_ResumeCond);
		pthread_mutex_unlock(&m_SuspendMutex);
	}
};


void * threadFun(void * p)
{
	ThreadInfo * _this = (ThreadInfo*)p;
	SOCKET acc = _this->acc;
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
		char intbuf[ 512 ];
		char resp[] = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n"
							"Content-Length: ";
		char conn[] = "Connection: close\r\n";
		char resp_end[] = "\r\n\r\n\n";
		char line_end[] = "\r\n";
		char content[] = "<html><body><h1>Data</h1></body></html>";
		sprintf(intbuf,"%d",(int)strlen(content));
		write(acc,resp,strlen(resp)) ;
		write(acc,intbuf,strlen(intbuf));
		write(acc,line_end,strlen(line_end));
		write(acc,conn,strlen(conn));
		write(acc,resp_end,strlen(resp_end));
		write(acc,content,strlen(content));

		break;
	}
	close(acc);
	shutdown(acc,0);
	return NULL;
}
int server(unsigned short port,Opt&opt)
{
	struct sockaddr_in6 addr;
	struct sockaddr_in6 client_addr;
	memset(&addr,0,sizeof(addr));
	memset(&client_addr,0,sizeof(client_addr));
	
	SOCKET sock = socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
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
	ThreadMap * threadMap = new ThreadMap;

	for(;;)
	{
		{
			char buffer[ 1024 ] ;
		printf("Accepting incoming connect <%s> port %d\n",
				inet_ntop(AF_INET6,&addr.sin6_addr,
				buffer,sizeof(buffer) - 1),
				ntohs(addr.sin6_port));
		}

		ThreadInfo * info = NULL;

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
			time_t timer;
			time(&timer);
			struct tm  * tbuf;
			tbuf = gmtime(&timer);
			char buffer2[ 512 ];
			strftime( buffer2 , sizeof(buffer2) ,
					"%Y%m%d %H%M%S",tbuf);
			char buffer[ 1024 ] ;
			log.trace("%s%02d:incoming connect from <%s> Port %d\n",
				buffer2,(timer%1000)/10,
				inet_ntop(AF_INET6,&client_addr.sin6_addr,
				buffer,sizeof(buffer) - 1),
				ntohs(client_addr.sin6_port));
			FILE * fp = fopen("accept.log","ab");
			if(fp)
			{
				fprintf(fp,
				"%s:incoming connect from <%s> Port %d\n",
				buffer2,
				inet_ntop(AF_INET6,&client_addr.sin6_addr,
				buffer,sizeof(buffer) - 1),
				ntohs(client_addr.sin6_port));
				fclose(fp) ;
			}
		}
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr,256*1024);
		info = new ThreadInfo(threadMap,acc,0);
		int ret = pthread_create(&info->threadId, &attr,
                          threadFun,(void*)info);
		threadMap->insert(std::make_pair(info->threadId,info));
		pthread_attr_destroy(&attr);

		
		
	}
}
int main(int argc,char**argv)
{
	log.load();
	
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
