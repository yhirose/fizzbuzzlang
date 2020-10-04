all: fzbz
	./fzbz fizzbuzz.fzbz

fzbz: fzbz.cc peglib.h
	clang++ -std=c++17 -o fzbz fzbz.cc

peglib.h:
	wget https://raw.githubusercontent.com/yhirose/cpp-peglib/cpp17/peglib.h

