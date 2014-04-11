#makefile for wsn server
sources	=	http.cpp
target	=	$(sources:.cpp= )
cc = g++
flags = -lpthread

$(target) : $(sources)
	$(cc) -o $@ $< $(flags)

clean :	
	rm -f $(target)

all : clean $(target)


