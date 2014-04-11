#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
//#include<netdb.h>
//#include<sys/types.h>
//#include<netinet/in.h>
//#include<sys/socket.h>
//#include<sys/wait.h>
#include<arpa/inet.h>
int main(int argc,char *argv[])
{
 int sockfd,new_fd;
 struct sockaddr_in server_addr;
 struct sockaddr_in client_addr;
 int portnumber;
 socklen_t sin_size;
 char hello[]="HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 20\r\n\r\nHello !中文Are you fine?\n";
 printf("start 程序开始\n\n");
 if (argc !=2)
 {
  fprintf(stderr,"Usage:%s portnumber\a\n",argv[0]);
  exit(1);
  }
  if((portnumber=atoi(argv[1]))<0)                       //第一次运行没问题，后出现connection refused,多加个括号解决
  {
  fprintf(stderr,"Usage:%s portnumber\a\n",argv[0]);
  exit(1);
  }
  if ((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
  {
  fprintf(stderr,"Socket error:%s\n\a",strerror(errno));
  exit(1);
  }
  bzero(&server_addr,sizeof(struct sockaddr_in));
  server_addr.sin_family=AF_INET;
  server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  server_addr.sin_port=htons(portnumber);
  if (bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
  {
  fprintf(stderr,"Bind error:%s\n\a",strerror(errno));
  exit(1);
  }
  if (listen(sockfd,5)==-1)
  {
  fprintf(stderr,"Listen error:%s\n\a",strerror(errno));
  exit(1);
  }
  while(1)
  {
  sin_size=sizeof(struct sockaddr_in);
  if((new_fd=accept(sockfd,(struct sockaddr*)(&client_addr),&sin_size))==-1)
  {
  fprintf(stderr,"Accept error:%s\n\a",strerror(errno));
  exit(1);
  }
  fprintf(stderr,"Server get connection from %s\n",inet_ntoa(client_addr.sin_addr));
  /*
    if(fork()==0){
   	char text[2000];
   	int len;
   	while ( (len=recv(new_fd, text, 1024, 0)) > 0) 
	{
		text[len]=0;
		printf("_RECV_\n%s\n",text);
		//close(new_fd);
		//exit(0);
		
	}
	printf("RECV FINISH__\n");
	exit(0);
   }
   */ 
  if(send(new_fd,hello,strlen(hello),0)==-1)
  {
   fprintf(stderr,"Write Error:%s\n",strerror(errno));
   exit(1);
   }

	printf("SEND FINISH___\n");
   close(new_fd);
   }
   close(sockfd);
   exit(0);
   }

