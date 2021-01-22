all: fzbz
	./fzbz fizzbuzz.fzbz

fzbz: fzbz.cc peglib.h Makefile
	clang++ -std=c++17 -o fzbz fzbz.cc -Wall -Wextra

peglib.h:
	wget https://raw.githubusercontent.com/yhirose/cpp-peglib/master/peglib.h

