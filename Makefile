USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES
USE_CLANG = YES

include ..\tool_select.mak 

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings
ifeq ($(USE_64BIT),YES)
CFLAGS += -DUSE_64BIT
endif

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

CFLAGS += -Ider_libs
IFLAGS += -Ider_libs
LiFLAGS += -Ider_libs

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LiFLAGS += -dUNICODE -d_UNICODE
LFLAGS += -dUNICODE -d_UNICODE
endif

LINTFILES=lintdefs.cpp lintdefs.ref.h 

# This is required for *some* versions of makedepend
IFLAGS += -DNOMAKEDEPEND

CPPSRC=read_tree.cpp \
der_libs/common_funcs.cpp \
der_libs/conio_min.cpp \
der_libs/qualify.cpp

OBJS = $(CPPSRC:.cpp=.o)

BIN=read_tree

BINX = $(BIN).exe

LIBS=-lshlwapi

#**************************************************************************
%.o: %.cpp
	$(TOOLS)/g++ $(CFLAGS) $< -o $@

all: $(BINX)

clean:
	rm -f $(OBJS) *.exe *~ *.zip

wc:
	wc -l $(CPPSRC)

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:/llvm/bin/clang-tidy.exe $(CPPSRC)"

lint:
	cmd /C "c:/lint9/lint-nt +v -width(160,4) $(LiFLAGS) -ic:/lint9 mingw.lnt -os(_lint.tmp) $(LINTFILES) $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC)

$(BINX): $(OBJS)
	$(TOOLS)/g++ $(OBJS) $(LFLAGS) -o $(BINX) $(LIBS) 

# DO NOT DELETE

read_tree.o: der_libs/common.h der_libs/conio_min.h der_libs/qualify.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/conio_min.o: der_libs/common.h der_libs/conio_min.h
der_libs/qualify.o: der_libs/common.h der_libs/qualify.h
