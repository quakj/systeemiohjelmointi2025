<h2>Reverse.c</h2>
The reverse program reads lines of text from an input source and prints them in reverse order. It has three modes of operation.

<h2>Usage</h2>

```
./reverse                       # Read from standard input and print reversed lines to standard output
./reverse input.txt             # Read from input.txt and print reversed lines to standard output
./reverse input.txt output.txt  # Read from input.txt and write reversed lines to output.txt
```
To terminate input from standard input use Ctrl + D on Linux

<h2>Example</h2>
Input (input.txt):
```
hello
this
is
a file
```

Command:
```
./reverse input.txt
```

Output:
```
a file
is
this
hello
```

<h2>Error Handling</h2>
The program handles various errors and prints appropriate messages:

- Too many arguments:
```
usage: reverse <input> <output>
```
- Input file cannot be opened:
```
error: cannot open file 'input.txt'
```
- Output file cannot be opened:
```
error: cannot open file 'output.txt'
```
- Input and output files must be different:
```
input and output file must differ
```
- Memory allocation failure:
```
malloc failed
```