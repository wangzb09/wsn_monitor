#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<dirent.h>
#include<sys/stat.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>

#include"http.h"
#include"database.h"
#include"global.h"
using namespace std;

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
	delete http;
	return 0;
}


void Init()
{
	mydb.LoadDatabase();
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
	
	Init();		//Do initial work
	
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



int main_t()
{
	
	Data d1;
	d1.SetData(3,strlen("中文ABC"),"中文ABC",4,"123",3,"Ad");
	int i;
	/*
	for(i=0;i<d1.NumOfCells;i++)
	{
		printf("[%d] len=%d dat=[%s]\n",i,d1.LenOfCell[i],d1.Cells[i]);
	}
	*/
	/*
	Database db1;
	
	db1.LoadDatabase();
	db1.DBCreateLib("test1");
	db1.SearchLibrary("test1")->AddData(d1);
	
	db1.SearchLibrary("test1")->DelData(0,5,"12345");
	//db1.SearchLibrary("test1")->DelData(0,strlen("中文ABC"),"中文ABC");
	
	//db1.DelLibrary("test1");
	*/
	return 0;
}
