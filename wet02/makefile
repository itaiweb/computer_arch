# 046267 Computer Architecture - Winter 20/21 - HW #2
CXX = g++
CXXFLAGS = -std=c++11 -g -Wall
CCLINK = $(CXX) $(CXXFLAGS)
OBJS = cache.o cacheSim.o 

cacheSim: $(OBJS) 
	$(CCLINK) -o cacheSim $(OBJS)

cache.o: cache.cpp cache.h
cacheSim.o: cacheSim.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
