#pragma once
#include<pthread.h>

class Data
{
public:
	Data();
	Data(const Data &dat);
	~Data();

	int SetData(int num,...);	//Set Data,format is Setdata(num,[length_0,dat_0],...,[length_(num-1),dat_(num-1)]), return 0 if succeed.
	int AddCell(int len,char *dat);	//Add a cell whose data is dat,return 0 if succeed;
	int DumpData(char *libname);	//dump data to file, return 0 if succeed

	friend class Library;
	friend class Database;
private:
	void DeleteData();	//Delete Data and free memory
			
	int NumOfCells;		//Number of data cells
	char **Cells;		//array to store the address of each data cell
	int *LenOfCell;		//array to store the length of each data cell
	
	bool Dirty;			//show if the data should be dump to files
	Data *Next;			//the pointer of next data line
};
class Library
{
public:
	Library(const char *name);
	~Library();
	friend class Database;
private:	
	Data *SearchData(int index,int len,const char *dat);		//Look for data with content dat at the index cell,return NULL if not find
	int AddData(const Data &dat);						//Add Data to library and alloc space to file, return 0 if succeed
	int DelData(int index,int len,const char *dat);			//Delete data whose content at index is dat, and delete file, return 0 if succeed
	
	void DeleteLibrary();							//Delete library contents
	int LoadLibrary();								//Load Library to memory, return 0 if succeed
	Data * AddMemData(const Data &dat);				//Add data to Library, return the address of Data cell,return NULL if failed
	
	char *Name;		//Name of the library
	Data *Head;		//Pointer of data head
	Library *Next;	//Pointer of Next Library 

};
class Database
{
public:
	Database();
	~Database();
	int LoadDatabase();								//Load database to memory, return 0 if succeed
	int DelLibrary(const char *name);				//Delete Library and delete directory on disk
	int DBCreateLib(const char *name);				//Create Library in multi-thread environment
	int DBAddData(const char *libname,const Data &dat);	//Add data to library, return 0 if succeed
	int DBGetAllValue(const char *libname,int *x,float *y);	//Get all the data in library,return the value got
	
private:
	Library *SearchLibrary(const char *name);		//Search for library
	void DeleteDatabase();
	int CreateLibrary(const char *name);			//Create a library with name and alloc space in disk, return 0 if succeed
	
	Library *Head;
	
	pthread_mutex_t DatabaseMutex;
};
