FizzBuzzLang
============

A Programming Language just for writing [Fizz Buzz](https://en.wikipedia.org/wiki/Fizz_buzz) program. :)

```
for n from 1 to 100
  puts n % 3 == 0 ? (n % 5 == 0 ? 'FizzBuzz' : 'Fizz') : (n % 5 == 0 ? 'Buzz' : n)
```

Build and Run demo
------------------

```bash
> make
./fzbz fizzbuzz.fzbz
1
2
Fizz
4
Buzz
...
```

PEG grammar
-----------

```peg
# Syntax Rules
EXPRESSION              <- TERNARY
TERNARY                 <- CONDITION ('?' EXPRESSION ':' EXPRESSION)?
CONDITION               <- MULTIPLICATIVE (ConditionOperator MULTIPLICATIVE)?
MULTIPLICATIVE          <- CALL (MultiplicativeOperator CALL)*
CALL                    <- PRIMARY (EXPRESSION)?
PRIMARY                 <- FOR / Identifier / '(' EXPRESSION ')' / String / Number
FOR                     <- 'for' Identifier 'from' Number 'to' Number EXPRESSION

# Token Rules
ConditionOperator       <- '=='
MultiplicativeOperator  <- '%'
Identifier              <- !Keyword < [a-zA-Z][a-zA-Z0-9_]* >
String                  <- "'" < ([^'] .)* > "'"
Number                  <- < [0-9]+ >

Keyword                 <- 'for' / 'from' / 'to'
%whitespace             <- [ \t\r\n]*
```

License
-------

[MIT](https://github.com/yhirose/fizzbuzzlang/blob/master/LICENSE)
