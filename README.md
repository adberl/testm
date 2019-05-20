Info
---------------------

A wrapper around the pdb (python debug) module, used to test code with. It's not hugely practical, was made more to settle an argument.

Usage
---------------------

To compile the program, simply run:
```
gcc -Wall -g testm.c -o testm.o
```

Afterwards, you can execute it on the ``example_commands.txt`` and ``func_test.py`` by running the following:
```
./testm.o func_test.py example_commands.txt
```

``example_commands.txt`` general layout is as follows:

```
<number of tests>
<testname> <function name> <function arguments> <expected return value>
<testname2> ...
```
