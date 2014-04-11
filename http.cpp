#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>
using namespace std;


#define MYDBG

#ifdef MYDBG
#define dbgprint(...) printf(__VA_ARGS__)
#else
#define dbgprint(x)
#endif  

 char hello[]="HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 20\r\n\r\nHello !中文Are you fine?\n";

class Http
{
public:
	Http(int sock,sockaddr_in clientaddr);
	void PrintAddr();
	void RecvHttp();
	int Input(char msg[],int len);

private:
	int socket;
	sockaddr_in client_addr;

};

Http::Http(int sock,sockaddr_in clientaddr):socket(sock),client_addr(clientaddr)
{
}

int Http::Input(char msg[],int len)
{
	char buff[5000];
	strncpy(buff,msg,len);
	buff[len]=0;
	printf("MSG[%s]\n",buff);
	return 0;
}

void Http::PrintAddr()
{
	printf("socket=%d\nclient=[%s:%d]\n",socket,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
	if(send(socket,hello,strlen(hello),0)==-1)
	{
		printf("Send Error\n");
		pthread_exit((void *)1);
	}
}

#define BUFFLEN 4096
void Http::RecvHttp()
{
	dbgprint("Get connection from [%s:%d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
	
	int headlen,len;
	char buff[BUFFLEN];
	int start_buff,start_search,i;
	start_buff=start_search=0;
	bool flag=true;
	while(flag)
	{
		len=recv(socket, buff+start_buff, BUFFLEN-start_buff, 0);
		if(len>0)
		{
			for(i=start_search;i<start_buff+len-3;i++)
			{
				if(buff[i]=='\r' && buff[i+2]=='\r' && buff[i+1]=='\n' && buff[i+3]=='\n')
				{
					headlen=i;
					start_search=i+4;
					start_buff=start_buff+len;
					flag=false;
					break;
				}
			}
			if(flag)
			{
				start_buff+=len;
				start_search=i;	
				if(start_buff>=BUFFLEN)
				{
					printf("Err: Http head is longer than %d bytes [%s:%d]\n",BUFFLEN,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
					close(socket);
					pthread_exit((void *)1);
					
				}
			}
			
		}
		else
		{
				printf("Err:Socket is closed, not http format, total len=%d [%s:%d]\n",start_buff,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
				close(socket);
				pthread_exit((void *)1);			
		}
	}
	buff[headlen]=0;
	printf("[MSG]\n%s\n",buff);
	PrintAddr();
	dbgprint("Disconnect with [%s:%d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
	close(socket);
	
}

//创建分离式线程
int create_dethread(pthread_t *pthread,void *(*start_routine) (void *), void *arg)
{
	pthread_attr_t attr;
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);	//设置线程具有分离式属性
	int ret=pthread_create(pthread, &attr, start_routine, arg);		//创建线程
	pthread_attr_destroy (&attr);
	return ret;
}

void *thread_usr(void *arg)
{
	int sock_listen=*(int *)arg;
	char str[100];
	while(1)
	{
		cin>>str;
		if(!strcmp(str,"e"))
		{
			close(sock_listen);
			printf("exit\n");
			exit(0);
		}
	}

	
}

void *thread_http(void *arg)
{
	Http *http=(Http *)arg;
	http->RecvHttp();
	return 0;
}

int main(int argc, char *argv[])
{
	int portnumber=80;
	if(argc>=2)
	{
		if((portnumber=atoi(argv[1]))<0)
		{
			printf("portnumber(%s) error\n",argv[1]);
			exit(1);
		}
	}
	printf("Server start, portnumber=%d\n",portnumber);
	
	int sock_listen;
	if((sock_listen=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		printf("Socket error:%s\n",strerror(errno));
		exit(1);
	}
	sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(portnumber);
	if (bind(sock_listen,(sockaddr *)(&server_addr),sizeof(server_addr))==-1)
	{
		printf("Bind error:%s\n",strerror(errno));
		exit(1);
	}
	if (listen(sock_listen,3)==-1)
	{
		printf("Listen error:%s\n",strerror(errno));
		exit(1);
	}
	int c;
	pthread_t thread;
	if((c=create_dethread(&thread,thread_usr,&sock_listen))!=0)
	{
		close(sock_listen);
		printf("Create Thread Error, return %d\n",c);
		exit(1);
	}	
	while(1)
	{

		int sock_accept;
		sockaddr_in client_addr;
		socklen_t size_sin=sizeof(sockaddr_in);
		if((sock_accept=accept(sock_listen,(sockaddr*)(&client_addr),&size_sin))==-1)
		{
			printf("Accept error:%s\n",strerror(errno));
			exit(1);
		}
		dbgprint("Main thread get connection from [%s:%d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

		Http *http_new=new Http(sock_accept,client_addr);
		if((c=create_dethread(&thread,thread_http,http_new))!=0)
		{
			close(sock_accept);
			printf("Create Thread Error, return %d\nThe main thread is continue...\n",c);
		}
	}
	close(sock_listen);
	
	return 0;
}
