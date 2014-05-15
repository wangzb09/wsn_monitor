#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<dirent.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>
using namespace std;


#define MYDBG

#ifdef MYDBG
#define dbgprint(...) printf(__VA_ARGS__)
#else
#define dbgprint(...)
#endif  


//the command of http protocol
#define HTTP_NOCMD	0
#define HTTP_GET	1
#define HTTP_POST	2

#define URL_LEN		1024
#define BUFFLEN		4096
#define MAXCONTENTBUFF	1*1024*1024
#define MAXCONTENTDATA  100*1024*1024

class Http
{
public:
	Http(int sock,sockaddr_in clientaddr);
	void PrintAddr();
	void RecvHttp();
	int Input(char msg[],int len);
	int Response();
	int GetWeb();
	int GetCmd();

private:
	int socket;
	sockaddr_in client_addr;
	int cmd;
	char url[URL_LEN];
	char *content;
	int contentlen;

};

Http::Http(int sock,sockaddr_in clientaddr):socket(sock),client_addr(clientaddr),cmd(HTTP_NOCMD),content(NULL),contentlen(0)
{
}

int Http::GetWeb()
{
	char filename[PATH_MAX]="webs";
	int i,j;
	
	j=strlen(filename);
	for(i=0;i<strlen(url);i++)
	{
		if(url[i]=='?') break;
	
		if(url[i]=='.' && url[i+1]=='.')
		{
			printf("WARNING: Illegal file name in url [%s]\n",url);		//This is an url attempt to read file without permission
			strcpy(filename,"webs/error.htm");		//No matter what the url is ,tell the client this page does not exist
			j=strlen(filename);
			break;		
		}
	
		filename[j]=url[i];
		j++;
	}
	filename[j]=0;
	filename[j+1]=0;
	if(strcmp(filename,"webs/")==0)	//the default web page
	{
		strcpy(filename,"webs/index.htm");
	}
	
	FILE *fp=fopen(filename,"rb");
	if(!fp)
	{
		dbgprint("Cannot find file %s\n",filename);
		strcpy(filename,"webs/error.htm");	//return the default error page
		
		fp=fopen(filename,"rb");
		if(!fp)
		{
			printf("open %s fail\n",filename);
			return 3;
		}
	}
	fseek(fp,0,SEEK_END);
	int flen=ftell(fp);		//get the length of the web page
	fseek(fp,0,SEEK_SET);
	char buff[BUFFLEN];
	sprintf(buff,"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: %d\r\n\r\n",flen);
	if(send(socket,buff,strlen(buff),0)==-1)	//send http head to client
	{
		printf("Send Error\n");
		pthread_exit((void *)1);
	}
	int size;
	while((size=fread(buff,sizeof(char),BUFFLEN,fp))>0)	//send data to client
	{
		if(send(socket,buff,size,0)==-1)
		{
			printf("Send Error\n");
			pthread_exit((void *)1);
		}				
	}
	fclose(fp);
	return 0;
}

int Http::GetCmd()
{
	printf("Command not finished\n");
	return 0;
}

int Http::Input(char msg[],int len)
{
	int i;
	int start=0;	//scan http head
	for(i=0;i<len-1;i++)
	{
		if(msg[i]=='\r' && msg[i+1]=='\n')
		{
			int len=i-start;
			char buff1[URL_LEN]="";
			char buff2[URL_LEN]="";
			char buff3[URL_LEN]="";
			char buff4[URL_LEN]="";
			int argc;
			msg[i]=0;
			if((argc=sscanf(msg+start,"%s%s%s%s",buff1,buff2,buff3,buff4)) >0)	//get method from message
			{
				dbgprint("CMD agrc=%d\n[1]%s[2]%s[3]%s[4]%s[]\n",argc,buff1,buff2,buff3,buff4);
				if(strcmp(buff1,"GET")==0)
				{
					if(argc>=2)
					{
						cmd=HTTP_GET;	//the is the GET method
						strcpy(url,buff2);	//the url to get
					}
					else
					{
						printf("GET params not enough\n");
						return 1;
					}
				}
				else if(strcmp(buff1,"POST")==0)	//POST method
				{
					if(argc>=2)
					{
						cmd=HTTP_GET;	//the is the POST method
						strcpy(url,buff2);	//the url to get
					}
					else
					{
						printf("POST params not enough\n");
						return 1;
					}
				}
				else if(strcmp(buff1,"Content-Length:")==0)
				{
					if(argc>=2)
					{
						sscanf(buff2,"%d",&contentlen);		//get the length of content data
						if(contentlen<0)
						{
							printf("Content-Length format wrong\n");
							return 2;
						}
					}
					else
					{
						printf("Content-Length: params not enough\n");
						return 1;
					}
				}
			}
			start=i+2;
			i++;
		}
	}
	return 0;
}
int Http::Response()
{
	switch(cmd)
	{
		case HTTP_GET:
		{
			if(strncmp(url,"/cmd?",5)==0)
			{
				GetCmd();
			}
			else
			{
				GetWeb();
			}
			break;
		}
		case HTTP_POST:
		{
			GetWeb();
			break;
		}
		case HTTP_NOCMD:
		{
			printf("No command in HTTP head\n");
			return 1;
		}
		default:
		{
			printf("Unknown http command\n");
			return 2;
		}
	}
	return 0;
}

void Http::PrintAddr()
{
	printf("socket=%d\nclient=[%s:%d]\n",socket,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
}


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
		len=recv(socket, buff+start_buff, BUFFLEN-start_buff, 0);		//receive data from client
		if(len>0)
		{
			for(i=start_search;i<start_buff+len-3;i++)
			{
				if(buff[i]=='\r' && buff[i+2]=='\r' && buff[i+1]=='\n' && buff[i+3]=='\n')	//the http head is finished
				{
					headlen=i+2;	//the length of http head(There is only 1 '\r\n' at the end)
					start_search=i+4;
					start_buff=start_buff+len;
					flag=false;
					break;
				}
			}
			if(flag)	//the http head is not finished
			{	//update the buffer information
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
	dbgprint("[MSG]\n%s\n",buff);
	if(Input(buff,headlen)!=0)		//analyse http header and get information
	{
		printf("HTTP HEAD ERROR, Disconnect with [%s:%d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
		close(socket);
		return;
	}
	//get the content of http
	if(contentlen>0)
	{
		//receive content data
		if(contentlen<=MAXCONTENTBUFF)
		{
			if(content) delete content;
			content=new char[contentlen];
			if(!content)
			{
				printf("Malloc memory for content failed\n");
				close(socket);
				return;
			}
			dbgprint("start_search=%d\nstart_buff=%d\n",start_search,start_buff);
			
			start_buff=start_buff-start_search;
			if(start_buff<0)
			{
				dbgprint("start_buff errror\n");
				return;
			}
			
			if(start_buff>0)
			{
				
				memcpy(content,buff+start_search,start_buff);
			}
			content[start_buff]=0;
			while(start_buff<contentlen)
			{
				len=recv(socket, content+start_buff, contentlen-start_buff, 0);		//receive data from client			
				if(len>0)
				{
					start_buff+=len;	//update content length
					content[start_buff]=0;
				}
				else
				{
					printf("Err:Socket is closed, content ont enough, total len=%d [%s:%d]\n",start_buff,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
					close(socket);
					pthread_exit((void *)1);			
				}
			}
		}
		else if(contentlen<=MAXCONTENTDATA)
		{
			dbgprint("Content is long, ignore\n");
			close(socket);
			return;
		}
		else
		{
			dbgprint("Content is too long\n");
			close(socket);
			return;
		}
		
	}
	dbgprint("[CONTENT]\n%s\n",content);
	
	if(Response()!=0)		//response to the client according to the command
	{
		printf("Response failed to [%s:%d]\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
		close(socket);
		return;
	}
	//PrintAddr();
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


class Data
{
public:
	Data();
	~Data();
	void DeleteData();			//Delete Data and free memory
	int SetData(int num,...);	//Set Data,format is Setdata(num,[length_0,dat_0],...,[length_(num-1),dat_(num-1)]);
	
	int NumOfCells;		//Number of data cells
	char **Cells;		//array to store the address of each data cell
	int *LenOfCell;		//array to store the length of each data cell
	
	bool Dirty;			//show if the data should be dump to files
	Data *Next;			//the pointer of next data line
};

Data::Data():NumOfCells(0),Cells(NULL),LenOfCell(NULL),Dirty(0),Next(NULL)
{
}
Data::~Data()
{
	DeleteData();
}
void Data::DeleteData()
{
	int i;
	if(NumOfCells)
	{
		for(i=0;i<NumOfCells;i++)
			if(LenOfCell[i]) delete []Cells[i];		//delete data cells which are not empty
		delete []Cells;			//delete the array to save the address of cells
		delete []LenOfCell;		//delete the array to save the length of cells
		//set all to empty
		NumOfCells=0;
		Cells=NULL;
		LenOfCell=NULL;
	}
}
int Data::SetData(int num,...)
{
	void *args;
	args=(void *)(&num + 1);
	if(num<1)
	{
		printf("SetData params not enough\n");
		return 1;
	}
	
	if(Cells) delete []Cells;
	if(LenOfCell) delete []LenOfCell;
	
	Cells=new char*[num];
	if(!Cells)
	{
		printf("SetData alloc data for Cells failed\n");
		NumOfCells=0;
		return 2;
	}
	LenOfCell=new int[num];
	if(!LenOfCell)
	{
		printf("SetData alloc data for LengthOfCell failed\n");
		delete []Cells;
		Cells=NULL;
		NumOfCells=0;
		return 2;
	}
	NumOfCells=num;			//set NumOfCells
	int i;
	for(i=0;i<num;i++)		//Set LengthOfCell to 0, to enable DeleteData() function
		LenOfCell[i]=0;
	
	for(i=0;i<num;i++)
	{	//get all the params and alloc memory
		int size;
		char *dat;
		size=*((int *)args);
		args=((int*)args)+1;
		dat=*(char **)args;
		args=((char**)args)+1;
		
		if(size<0)
		{
			printf("SetData param [%d] error\n",i);
			int j;
			DeleteData();
			return 3;			
		}
		else if(size==0)
		{	//This is an empty cell
			LenOfCell[i]=0;
			Cells[i]=NULL;
		}
		else
		{
			Cells[i]=new char[size];
			if(!Cells[i])
			{
				printf("SetData alloc memory for Cells[%d] failed\n",i);
				DeleteData();
				return 2;
			}
			LenOfCell[i]=size;
			memcpy(Cells[i],dat,size);	//set data
		}
	}
	return 0;
}

class Database
{
public:


private:
};

int main()
{
	Data d1;
	d1.SetData(3,4,"ABC",4,"123",3,"Ad");
	int i;
	for(i=0;i<d1.NumOfCells;i++)
	{
		printf("[%d] len=%d dat=[%s]\n",i,d1.LenOfCell[i],d1.Cells[i]);
	}
	
	return 0;
}

int main_t(int argc, char *argv[])
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
