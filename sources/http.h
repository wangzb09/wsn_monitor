#pragma once

//the command of http protocol
#define HTTP_NOCMD	0
#define HTTP_GET	1
#define HTTP_POST	2

#define URL_LEN		1024
#define MAXCONTENTBUFF	1*1024*1024
#define MAXCONTENTDATA  100*1024*1024

class Http
{
public:
	Http(int sock,sockaddr_in clientaddr);
	~Http();
	void PrintAddr();
	void RecvHttp();
	int Input(char msg[],int len);
	int Response();
	int GetWeb();
	int GetCmd();
	int GetHtmCmd();
	
private:
	int socket;
	sockaddr_in client_addr;
	int cmd;
	char url[URL_LEN];
	char *content;
	int contentlen;

};
