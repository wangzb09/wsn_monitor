#makefile for wsn server
sources	=	./sources/http.cpp ./sources/database.cpp ./sources/global.cpp ./sources/main.cpp
headers = ./sources/http.h ./sources/database.h ./sources/global.h
target	=	wsn
cc = g++
flags = -lpthread

$(target) : $(sources) $(headers)
	$(cc) $(flags) -o $@ $(sources)

clean :	
	rm -f $(target)

all : clean $(target)


