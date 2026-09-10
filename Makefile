all: cpp
	./fzbz fizzbuzz.fzbz

cpp: fzbz.cc peglib.h Makefile
	clang++ -std=c++17 -o fzbz fzbz.cc -Wall -Wextra

peglib.h:
	wget https://raw.githubusercontent.com/yhirose/cpp-peglib/master/peglib.h

cul: fzbz.cul Makefile
	culebra fzbz.cul fizzbuzz.fzbz

