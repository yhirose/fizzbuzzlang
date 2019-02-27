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
START                   <- _ EXPRESSION _
EXPRESSION              <- TERNARY
TERNARY                 <- CONDITION (_ '?' _ EXPRESSION _ ':' _ EXPRESSION)?
CONDITION               <- MULTIPLICATIVE (_ ConditionOperator _ MULTIPLICATIVE)?
MULTIPLICATIVE          <- CALL (_ MultiplicativeOperator _ CALL)*
CALL                    <- PRIMARY (__ EXPRESSION)?
PRIMARY                 <- FOR / Identifier / '(' _ EXPRESSION _ ')' / String / Number
FOR                     <- 'for' __ Identifier __ 'from' __ Number __ 'to' __ Number __ EXPRESSION

# Token Rules
ConditionOperator       <- '=='
MultiplicativeOperator  <- '%'
Identifier              <- !Keyword [a-zA-Z][a-zA-Z0-9_]*
String                  <- "'" < (!['] .)* > "'"
Number                  <- [0-9]+
~_                      <- Whitespace*
~__                     <- Whitespace+
Whitespace              <- [ \t\r\n]
Keyword                 <- 'for' / 'from' / 'to'
```

License
-------

[MIT](https://github.com/yhirose/fizzbuzzlang/blob/master/LICENSE)
