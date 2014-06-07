#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<dirent.h>
#include<sys/stat.h>

#include"database.h"
#include"global.h"
using namespace std;

Data::Data():NumOfCells(0),Cells(NULL),LenOfCell(NULL),Dirty(0),Next(NULL)
{
}
Data::Data(const Data & dat)
{
	NumOfCells=dat.NumOfCells;
	Dirty=dat.Dirty;
	Next=NULL;		//This pointer is not copied to prevent error
	
	if(NumOfCells)
	{
		Cells=new char *[NumOfCells];
		LenOfCell=new int[NumOfCells];
		int i;
		for(i=0;i<NumOfCells;i++)
		{
			LenOfCell[i]=dat.LenOfCell[i];
			Cells[i]=new char[LenOfCell[i]];
			memcpy(Cells[i],dat.Cells[i],LenOfCell[i]);
		}
	}
}
Data::~Data()
{
	DeleteData();
}
int Data::DumpData(char *libname)
{
	char filepath[PATH_MAX];
	char filename[PATH_MAX];
	
	Dirty=0;	
	if(NumOfCells<=0)
	{
		printf("Empty cell, dump failed\n");
		return 1;
	}
	memcpy(filename,Cells[0],LenOfCell[0]);
	filename[LenOfCell[0]]=0;
	sprintf(filepath,"./database/%s/%s",libname,filename);
	
	FILE *fp=fopen(filepath,"wb");
	int i;
	for(i=1;i<NumOfCells;i++)
	{
		fwrite(&LenOfCell[i],sizeof(int),1,fp);
		fwrite(Cells[i],sizeof(char),LenOfCell[i],fp);
	}
	fclose(fp);
	return 0;
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
int Data::AddCell(int len,char *dat)
{
	dbgprint("AddCell len=%d dat=%s\n",len,dat);
	
	int num=NumOfCells+1;
	char **CellsNew=new char*[num];
	int *LenOfCellNew=new int[num];
	int i;
	for(i=0;i<NumOfCells;i++)
	{	//Copy existing data
		LenOfCellNew[i]=LenOfCell[i];
		CellsNew[i]=new char[LenOfCell[i]];
		memcpy(CellsNew[i],Cells[i],LenOfCell[i]);
	}
	//set new data
	LenOfCellNew[NumOfCells]=len;
	CellsNew[NumOfCells]=new char[len];
	memcpy(CellsNew[NumOfCells],dat,len);
	//Free memory and Point data pointer to new memory
	DeleteData();
	NumOfCells=num;
	Cells=CellsNew;
	LenOfCell=LenOfCellNew;
	return 0;
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
	
	DeleteData();	//Clear existing data
	
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
		printf("SetData alloc data for LenOfCell failed\n");
		delete []Cells;
		Cells=NULL;
		NumOfCells=0;
		return 2;
	}
	NumOfCells=num;			//set NumOfCells
	int i;
	for(i=0;i<num;i++)		//Set LenOfCell to 0, to enable DeleteData() function
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
Library::Library(const char *name):Head(NULL),Next(NULL)
{
	Name=new char[strlen(name)+1];
	strcpy(Name,name);
}
Library::~Library()
{
	DeleteLibrary();
	if(Name) delete []Name;
}
void Library::DeleteLibrary()
{
	Data *p;
	while(Head)
	{
		p=Head->Next;
		delete Head;
		Head=p;
	}
}
Data *Library::SearchData(int index,int len,const char *dat)
{
	Data *p=Head;
	while(p)
	{		
		if(index < p->NumOfCells)
			if(len == p->LenOfCell[index])
				if(memcmp(dat,p->Cells[index],len)==0)
					return p;
		p=p->Next;
	}
	return NULL;
}

int Library::DelData(int index,int len,const char *dat)
{
	if(!Head)
	{
		printf("Library %s is empty, cannot delete data\n",Name);
		return 1;
	}
	Data *p=Head;
	Data *pf=p;
	while(p)
	{		
		if(index < p->NumOfCells)
			if(len == p->LenOfCell[index])
				if(memcmp(dat,p->Cells[index],len)==0)
				{
					char filebuff[PATH_MAX];
					char filename[PATH_MAX];
					memcpy(filebuff,p->Cells[0],p->LenOfCell[0]);
					filebuff[p->LenOfCell[0]]=0;
					sprintf(filename,"./database/%s/%s",Name,filebuff);
					
					int ret=remove(filename);
					if(ret!=0)
					{
						printf("Delete file %s failed\n",filename);
						return 2;
					}
					if(p==Head)
					{
						Head=p->Next;
						delete pf;
					}
					else
					{
						pf->Next=p->Next;
						delete p;
					}
					dbgprint("Delete data with key %s in Library %s\n",filebuff,Name);
					return 0;
				}
		pf=p;
		p=p->Next;
	}
	char *buff=new char[len+1];
	memcpy(buff,dat,len);
	buff[len]=0;
	printf("Data %s not found in Library %s\n",buff,Name);
	delete buff;
	return 3;	
}
int Library::LoadLibrary()
{
	dbgprint("LoadLibrary %s\n",Name);
	
	char libpath[PATH_MAX];
	sprintf(libpath,"./database/%s",Name);
	
	DIR *pdir=opendir(libpath);
	if(!pdir)
	{
		printf("Open Library %s failed\n",Name);
		return 1;
	}
	struct dirent *pent;
	while((pent=readdir(pdir))!=NULL)
	{
		if(pent->d_type==DT_REG)
		{
			char filepath[PATH_MAX];
			sprintf(filepath,"%s/%s",libpath,pent->d_name);
			FILE*fp=fopen(filepath,"rb");
			if(!fp)
			{
				printf("Open file %s failed\n",filepath);
				return 2;
			}
			Data *pdata=new Data;
			pdata->AddCell(strlen(pent->d_name),pent->d_name);		//we don not use unicode
			int len;
			char buff[BUFFLEN];
			while(fread(&len,sizeof(len),1,fp))
			{
				if(!fread(buff,sizeof(char),len,fp))
				{
					printf("Read file %s failed\n",filepath);
					delete pdata;
					return 3;
				}
				pdata->AddCell(len,buff);
				
			}
			fclose(fp);
			if(pdata->NumOfCells<=0)
			{
				printf("Read file %s failed\n",filepath);
					delete pdata;
					return 3;
			}
			pdata->Next=Head;
			Head=pdata;
			dbgprint("Load data with key [%s] to lib [%s]\n",pent->d_name,Name);			
		}
	}
	closedir(pdir);	
	
	return 0;
}
Data * Library::AddMemData(const Data &dat)
{
	Data *pdata=SearchData(0,dat.LenOfCell[0],dat.Cells[0]);
	if(pdata)
	{
		char buff[BUFFLEN];
		memcpy(buff,dat.Cells[0],dat.LenOfCell[0]);
		buff[dat.LenOfCell[0]]=0;
		printf("Data with key [%s] in Library [%s] already exists\n",buff,Name);
		return NULL;
	}
	//Create data and add to library
	pdata=new Data(dat);
	pdata->Next=Head;
	Head=pdata;
	
	pdata->Dirty=1;		//It is not written to file
	
	return pdata;
}
int Library::AddData(const Data &dat)
{
	Data *pdata=AddMemData(dat);
	if(pdata==NULL) return 2;
	return pdata->DumpData(Name);
}
Database::Database():Head(NULL)
{
	if(pthread_mutex_init(&DatabaseMutex,NULL)!=0)
	{
		printf("Data base create mutex failed\n");	
	}	
}
Database::~Database()
{
	DeleteDatabase();
	pthread_mutex_destroy(&DatabaseMutex);
}
void Database::DeleteDatabase()
{
	Library *p;
	while(Head)
	{
		p=Head->Next;
		delete Head;
		Head=p;
	}
}
Library *Database::SearchLibrary(const char *name)
{
	Library *p=Head;
	while(p)
	{
		//dbgprint("SearchLibrary name=[%s] p->Name=[%s]\n",name,p->Name);
		
		if(strcmp(name,p->Name)==0) return p;
		p=p->Next;
	}
	return NULL;
}
int Database::CreateLibrary(const char *name)
{
	Library *p=SearchLibrary(name);
	if(p)
	{
		printf("Library %s already exists\n",name);
		return 1;
	}
	//Create a library and add it to database
	p=new Library(name);
	p->Next=Head;
	Head=p;
	
	char libpath[PATH_MAX];
	sprintf(libpath,"./database/%s",name);
	if(mkdir(libpath,0755))
	{
		printf("Create LibDir %s failed\n",name);
		return 2;
	}	
	dbgprint("Create Library %s\n",name);
	return 0;
}

int Database::LoadDatabase()
{
	DIR *pdir=opendir("./database");
	if(!pdir)
	{
		printf("Open database failed\n");
		return 1;
	}
	struct dirent *pent;
	while((pent=readdir(pdir))!=NULL)
	{
		if(pent->d_name[0]=='.') continue;	//Current or upper level dir
		if(pent->d_type==DT_DIR)
		{
			Library *plib=SearchLibrary(pent->d_name);
			if(plib)
			{
				dbgprint("Error: Library %s already exists.\n",pent->d_name);
				return 2;
			}
			//Create Library and load it
			plib=new Library(pent->d_name);
			plib->LoadLibrary();
			
			//Add library to database
			plib->Next=Head;
			Head=plib;
		}
	}
	closedir(pdir);
	return 0;
}
int Database::DelLibrary(const char *name)
{
	Library *p=Head;
	Library *pf=Head;
	while(p)
	{		
		if(strcmp(name,p->Name)==0)
		{
			char rmlibpath[PATH_MAX];
			sprintf(rmlibpath,"rm -r ./database/%s",p->Name);
			//Delete library and all the data in the library		
			system(rmlibpath);
			if(p==Head)
			{
				Head=p->Next;
				delete p;
			}
			else
			{
				pf->Next=p->Next;
				delete p;
			}
			dbgprint("Delete Library %s\n",name);
			return 0;
		}
		
		pf=p;
		p=p->Next;
	}
	printf("Library %s does not exist, cannot delete\n",name);
	return 2;
}
int Database::DBAddData(const char *libname,const Data &dat)
{
	int result=0;
	
	pthread_mutex_lock(&DatabaseMutex);		//Lock database
	
	Library *plib=SearchLibrary(libname);	//find the library
	if(plib==NULL)
	{
		dbgprint("Library %s doesnot exist, add data failed\n",libname);
		result=100+1;
	}
	else
	{
		result=plib->AddData(dat);				//Add data to library
	}
	
	pthread_mutex_unlock(&DatabaseMutex);	//Unlock database
	return result;	
}
int Database::DBCreateLib(const char *name)
{
	pthread_mutex_lock(&DatabaseMutex);		//Lock database
	
	int result=CreateLibrary(name);
	
	pthread_mutex_unlock(&DatabaseMutex);	//Unlock database
	
	return result;
}
int Database::DBGetAllValue(const char *libname,int *x,float *y)
{
	int result=0;
	
	pthread_mutex_lock(&DatabaseMutex);		//Lock database
	
	Library *plib=SearchLibrary(libname);	//find the library
	if(plib==NULL)
	{
		dbgprint("Library %s doesnot exist, get data failed\n",libname);
		result=-1;
	}
	else
	{
		Data *pdat=plib->Head;
		while(pdat)
		{
			char buff[BUFFLEN];
			if(pdat->NumOfCells<2)
			{
				dbgprint("Data cannot be dump to memory\n");
				result=-2;
				break;
			}
			memcpy(buff,pdat->Cells[0],pdat->LenOfCell[0]);
			buff[pdat->LenOfCell[0]]=0;
			x[result]=atoi(buff);
			y[result]=atof(pdat->Cells[1]);
			result++;
			pdat=pdat->Next;		
		}
	}
	
	pthread_mutex_unlock(&DatabaseMutex);	//Unlock database
	return result;	
}

