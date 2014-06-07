#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<dirent.h>
#include<arpa/inet.h>

#include"global.h"
#include"http.h"
#include"database.h"
using namespace std;

Http::Http(int sock,sockaddr_in clientaddr):socket(sock),client_addr(clientaddr),cmd(HTTP_NOCMD),content(NULL),contentlen(0)
{
}
Http::~Http()
{
	if(content) delete content;
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
	#define CMDNUM 5
	
	const char *keys[CMDNUM]={"user=","&app=","&arg1=","&arg2=","&arg3="};
	const char *cmds[CMDNUM];
	char params[CMDNUM][URL_LEN];	
	char *cmd=url+5;
		
	int i;
	cmds[0]=strstr(cmd,keys[0]);
	if(cmds[0]!=cmd)
	{
		dbgprint("CMD 0 format error [%s]\n",cmd);
		return 1;
	}
	for(i=1;i<CMDNUM;i++)
	{
		cmds[i]=strstr(cmd,keys[i]);
		if(cmds[i]==NULL || cmds[i]<cmds[i-1]+strlen(keys[i-1]))
		{
			dbgprint("CMD %d format error [%s]\n",i,cmd);
			return 1;
		}
	}
	
	if(cmds[CMDNUM-1]+strlen(keys[CMDNUM-1])>cmd+strlen(cmd))
	{
		dbgprint("CMD format error [%s]\n",cmd);
		return 1;
	}
	for(i=0;i<CMDNUM;i++)
	{
		int len;
		if(i!=CMDNUM-1) len=cmds[i+1]-cmds[i]-strlen(keys[i]);
		else len=cmd+strlen(cmd)-cmds[CMDNUM-1]-strlen(keys[CMDNUM-1]);
		
		strncpy(params[i],cmds[i]+strlen(keys[i]),len);
		params[i][len]=0;
	}
	dbgprint("COMMAND\n");
	for(i=0;i<CMDNUM;i++)
	{
		dbgprint("[%s]=[%s]  ",keys[i],params[i]);
	}
	dbgprint("\n");
	
	char libname[URL_LEN];
	sprintf(libname,"%s[%s]",params[1],params[0]);

	if(!strcmp(params[2],"1"))
	{
		Data dat;
		dat.SetData(2,strlen(params[3]),params[3],strlen(params[4])+1,params[4]);
		if(mydb.DBAddData(libname,dat)==0)
		{
			printf("[CMD] Add data lib=%s params3=%s params4=%s\n",libname,params[3],params[4]);
		}
		else
		{
			printf("[CMD] Add data failed\n");
		}
	}
	else
	{
		printf("Invalid CMD [%s]\n",params[2]);
	}
	return 0;
}

int Http::GetHtmCmd()
{
	#define CMDNUM 5
	
	const char *keys[CMDNUM]={"user=","&app=","&arg1=","&arg2=","&arg3="};
	const char *cmds[CMDNUM];
	char params[CMDNUM][URL_LEN];	
	char *cmd=url+8;
		
	int i;	
	cmds[0]=strstr(cmd,keys[0]);
	if(cmds[0]!=cmd)
	{
		dbgprint("HTMCMD 0 format error [%s]\n",cmd);
		return 1;
	}
	for(i=1;i<CMDNUM;i++)
	{
		cmds[i]=strstr(cmd,keys[i]);
		if(cmds[i]==NULL || cmds[i]<cmds[i-1]+strlen(keys[i-1]))
		{
			dbgprint("HTMCMD %d format error [%s]\n",i,cmd);
			return 1;
		}
	}
	
	if(cmds[CMDNUM-1]+strlen(keys[CMDNUM-1])>cmd+strlen(cmd))
	{
		dbgprint("HTMCMD format error [%s]\n",cmd);
		return 1;
	}
	for(i=0;i<CMDNUM;i++)
	{
		int len;
		if(i!=CMDNUM-1) len=cmds[i+1]-cmds[i]-strlen(keys[i]);
		else len=cmd+strlen(cmd)-cmds[CMDNUM-1]-strlen(keys[CMDNUM-1]);
		
		strncpy(params[i],cmds[i]+strlen(keys[i]),len);
		params[i][len]=0;
	}
	dbgprint("HTM COMMAND\n");
	for(i=0;i<CMDNUM;i++)
	{
		dbgprint("[%s]=[%s]  ",keys[i],params[i]);
	}
	dbgprint("\n");
	
	char libname[URL_LEN];
	sprintf(libname,"%s[%s]",params[1],params[0]);
	
	if(!strcmp(params[2],"2"))
	{
	
		int *x=new int[BUFFLEN*10];
		float *y=new float[BUFFLEN*10];
		int result;
		
		if((result=mydb.DBGetAllValue(libname,x,y))<=0)
		{
			dbgprint("Access database failed");
			GetWeb();
			return 2;
		}
		
		int minx,maxx;
		minx=maxx=x[0];
		
		int x1[60*24];
		float y1[60*24];
		
		for(i=0;i<60*24;i++)
		{
			x1[i]=-1;
		}
		for(i=0;i<result;i++)
		{
			if(x[i]>=0 &&x[i]<60*24)
			{
				x1[x[i]]=x[i];
				y1[x[i]]=y[i];
			}
		}
		for(i=0;i<60*24;i++)
			if(x1[i]>=0) break;
		int start=i;
		
		char *hbuff=new char[BUFFLEN*10];
		char tb[BUFFLEN];
		sprintf(hbuff,"var time0=0\n var xdat=new Array();\n var dat=new Array();\n");
		int datlen=0;
		for(i=start;i<60*24;i++)
		{
			//if(x1[i]==0) y1[i]=y1[i-1];
			if(x1[i]>=0)
			{
				sprintf(tb,"xdat[%d]=%d\ndat[%d]=%.2f\n",datlen,x1[i],datlen,y1[i]);
				strcat(hbuff,tb);
				datlen++;
			}
		}
		sprintf(tb,"var datlen=%d\n",datlen);
		strcat(hbuff,tb);	
			
		FILE *fp1=fopen("webs/htmcmd1.txt","rb");
		FILE *fp2=fopen("webs/htmcmd2.txt","rb");
	
		int flen1,flen2,hblen;
	
		fseek(fp1,0,SEEK_END);
		flen1=ftell(fp1);		//get the length of the file1
		fseek(fp1,0,SEEK_SET);

		fseek(fp2,0,SEEK_END);
		flen2=ftell(fp2);		//get the length of the file1
		fseek(fp2,0,SEEK_SET);	
	
		hblen=strlen(hbuff);

		char buff[BUFFLEN];

		int len=strlen(hbuff);
		sprintf(buff,"HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: %d\r\n\r\n",flen1+hblen+flen2);
	
		if(send(socket,buff,strlen(buff),0)==-1)	//send http head to client
		{
			printf("Send Error\n");
			pthread_exit((void *)1);
		}
		int size;
		while((size=fread(buff,sizeof(char),BUFFLEN,fp1))>0)	//send data to client
		{
			if(send(socket,buff,size,0)==-1)
			{
				printf("Send Error\n");
				pthread_exit((void *)1);
			}				
		}
		if(send(socket,hbuff,hblen,0)==-1)
		{
			printf("Send Error\n");
			pthread_exit((void *)1);
		}				
		while((size=fread(buff,sizeof(char),BUFFLEN,fp2))>0)	//send data to client
		{
			if(send(socket,buff,size,0)==-1)
			{
				printf("Send Error\n");
				pthread_exit((void *)1);
			}				
		}
		fclose(fp1);
		fclose(fp2);
		delete []x;
		delete []y;
		delete []hbuff;
	}
	else
	{
		printf("Invalid HTM COMMAND [%s]\n",cmd);
	}

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
						cmd=HTTP_POST;	//the is the POST method
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
			else if(strncmp(url,"/htmcmd?",8)==0)
			{
				GetHtmCmd();
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
			if(content) delete []content;
			content=new char[contentlen+1];
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
					printf("Err:Socket is closed, content not enough, total len=%d [%s:%d]\n",start_buff,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
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

