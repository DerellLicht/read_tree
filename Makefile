# makefile for media_list app
SHELL=cmd.exe
USE_DEBUG = NO
USE_64BIT = NO

# program-operation flags
# these flags determine whether the resulting program will operate
# on each FILE located, using the command-line extention selection,
# or if it will operate on each FOLDER selected, and ignore filenames.
# It is expected that only one of these two options will be enabled.
# Selecting both, will give ambiguous results...
# Selecting neither, just won't give any results at all.
USE_FILES = YES
USE_FOLDERS = NO

ifeq ($(USE_64BIT),YES)
TOOLS=d:\tdm64\bin
else
TOOLS=c:\mingw\bin
endif

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
CxxFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -s -O3 -c
CxxFLAGS = -Wall -s -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
ifeq ($(USE_64BIT),YES)
CFLAGS += -DUSE_64BIT
CxxFLAGS += -DUSE_64BIT
endif
ifeq ($(USE_FILES),YES)
CFLAGS += -DOPERATE_ON_FILES
endif
ifeq ($(USE_FOLDERS),YES)
CFLAGS += -DOPERATE_ON_FOLDERS
endif

LIBS=-lshlwapi

CPPSRC=read_tree.cpp common.cpp qualify.cpp

OBJS = $(CSRC:.c=.o) $(CPPSRC:.cpp=.o)

#**************************************************************************
%.o: %.cpp
	$(TOOLS)\g++ $(CFLAGS) $<

%.o: %.cxx
	$(TOOLS)\g++ $(CxxFLAGS) $<

ifeq ($(USE_64BIT),NO)
BIN = read_tree.exe
else
BIN = read_tree64.exe
endif

all: $(BIN)

clean:
	rm -f *.o *.exe *~ *.zip

dist:
	rm -f read_files.zip
	zip read_tree.zip $(BIN) Readme.md

wc:
	wc -l *.cpp

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) $(LiFLAGS) -ic:\lint9 mingw.lnt -os(_lint.tmp) $(CPPSRC)"

depend: 
	makedepend $(CSRC) $(CPPSRC)

$(BIN): $(OBJS)
	$(TOOLS)\g++ $(OBJS) $(LFLAGS) -o $(BIN) $(LIBS) 

# DO NOT DELETE

read_tree.o: common.h qualify.h
common.o: common.h
qualify.o: qualify.h
