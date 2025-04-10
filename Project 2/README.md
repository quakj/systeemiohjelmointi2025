<h2>my-cat.c</h2>

Implements a simple version of the cat command. It reads and prints the content of one or more files to the standard output. If no files are provided, it simply exits with a return code of 0.

<h3>Usage</h3>

```
my-cat [file...]
```
[file...]: One or more files to print through.

<h3>Example</h3>
Print the contents of a file sample.txt:

sample.txt:
```
Hello, World!
This is a file.
```
```
my-cat sample.txt
```
Output:
```
Hello, World!
This is a file.
```

Print the contents of multiple files file1.txt and file2.txt:
```
my-cat file1.txt file2.txt
```

<h3>Error Handling</h3>

- Cannot Open File
```
my-cat: cannot open file
```



<h2>my-grep.c</h2>

Implements a simple version of the grep command using the Boyer-Moore algorithm for pattern searching. The program reads input (from either a file or stdin) and searches for a specified pattern, printing the line whenever the pattern is found (multiple matches on a line will only result in a single line of ouput). 

<h3>Usage</h3>

```
my-grep <searchterm> [file...]
```
searchterm: The term to search for within the provided files.
[file...]: One or more files to search through. If no files are provided, the program will read from the standard input.

<h3>Example</h3>
Search for the term ABC in the file test.txt

input.txt:
```
this file contains
ABC
this line doesn't match
but this ABC does
```
```
my-grep ABC input.txt
```
Output:
```
ABC
but this ABC does
```

Search for pattern in multiple files:
```
my-grep pattern file1.txt file2.txt
```
If no files are provided, it reads from standard input:
```
my-grep pattern
```

<h3>Error Handling</h3>

- No Search Term Provided
```
my-grep: searchterm [file ...]
```
- File Handling Errors
```
my-grep: cannot open file
```



<h2>my-zip.c & my-unzip.c</h2>

My-zip and my-unzip are simple command-line utilities for compressing and decompressing text files using Run-Length Encoding.

<h3>Usage</h3>
my-zip.c
```
./my-zip file1 [file2 ...] > output_file
```

my-unzip.c
```
./my-unzip compressed_file > output.txt
```

<h3>Example</h3>

my-zip.c
input.txt:
```
aaabbc
```
```
./my-zip input.txt > compressed.bin
```
compressed.bin:
```
[3, 'a'] [2, 'b'] [1, 'c']
```

my-unzip.c

```
./my-unzip compressed_file > output.txt
```

output.txt:
```
aaabbc
```

<h3>Error Handling</h3>

- No File Provided
```
my-zip: file1 [file2 ...]
my-unzip: file1 [file2 ...]
```
- File Handling Errors
```
my-zip: cannot open file
my-unzip: cannot open file
```