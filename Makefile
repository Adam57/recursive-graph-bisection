TYPE = -std=c++11 -O3 -Wall 

all: run.o reorder.o
	g++ $(TYPE)  run.o reorder.o -o run -lpthread
clean:
	rm run *.o

CPPC= g++ $(TYPE) -c  

run.o: run.cpp reorder.h
	$(CPPC) run.cpp

reorder.o: reorder.h
	$(CPPC) reorder.cpp
