all: fzbz
	./fzbz fizzbuzz.fzbz

fzbz: fzbz.cc peglib.h
	clang++ -o fzbz fzbz.cc

peglib.h:
	wget https://raw.githubusercontent.com/yhirose/cpp-peglib/master/peglib.h

