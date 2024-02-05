# read_tree - generic utility for searching folder tree for files
This will be a generic utility to recursively search through all folders
below specified target folder, and locate all files meeting the
specified file extension.
This is intended as a template for reading all files in current folder tree,
then performing some task on them.  The print statement at the end
can be replaced with a function call to perform the desired operation
on each discovered file.

The program now supports build-time options to act on folder specs or filtered file specs.
These options are selected via options in the Makefile:

```
# program-operation flags
# these flags determine whether the resulting program will operate
# on each FILE located, using the command-line extention selection,
# or if it will operate on each FOLDER selected, and ignore filenames.
# It is expected that only one of these two options will be enabled.
# Selecting both, will give ambiguous results...
# Selecting neither, just won't give any results at all.

USE_FILES = YES
USE_FOLDERS = NO
```

****************************************************************************************
This project is licensed under Creative Commons CC0 1.0 Universal;  
https://creativecommons.org/publicdomain/zero/1.0/

The person who associated a work with this deed has dedicated the work to the
public domain by waiving all of his or her rights to the work worldwide under
copyright law, including all related and neighboring rights, to the extent
allowed by law.

You can copy, modify, distribute and perform the work, even for commercial
purposes, all without asking permission. 

