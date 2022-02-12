smallsh - a simple bash shell in C

**To compile:**

```make``` 

or

```gcc -std=gnu99 -Wall -o smallsh main.c```

**Once the program has been compiled, type:**

```./smallsh```

**The shell accepts commands in the following syntax:**

```command [arg1 arg 2 ...] [< input_file] [> output_file] [&]```

where items in square brackets are optional.  The shell does not support any quoting or "|" pipe operators in the command.

