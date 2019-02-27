FizzBuzzLang
============

A Programming Language only useful to write [Fizz Buzz](https://en.wikipedia.org/wiki/Fizz_buzz). :)

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

License
-------

[MIT](https://github.com/yhirose/fizzbuzzlang/blob/master/LICENSE))
