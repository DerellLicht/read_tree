# read_tree - generic utility for searching folder tree for files
This will be a generic utility to recursively search through all folders
below specified target folder, and displaying the tree structure.

As of June 2025, this program supports Unicode filenames for input and output

<hr>

This project is licensed under Creative Commons CC0 1.0 Universal;  
https://creativecommons.org/publicdomain/zero/1.0/

The person who associated a work with this deed has dedicated the work to the
public domain by waiving all of his or her rights to the work worldwide under
copyright law, including all related and neighboring rights, to the extent
allowed by law.

You can copy, modify, distribute and perform the work, even for commercial
purposes, all without asking permission. 

<hr>

#### building the application
This application is built using the MinGW toolchain; 
I recommend the [TDM](http://tdm-gcc.tdragon.net/) distribution, 
to avoid certain issues with library accessibility. 
The makefile also requires certain Cygwin tools (rm, make, etc).

#### NOTE: this program requires my ```der_libs``` submodule
If you clone the repository without the --recursive flag, 
you can recover the submodule later, with this command:

```git submodule update --init --recursive```
<br>
