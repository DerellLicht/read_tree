USE_DEBUG = NO
USE_64BIT = YES
USE_UNICODE = YES
USE_CLANG = YES

include der_libs\tool_select.mak
include der_libs\release.mak

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -O3
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

CFLAGS += -Ider_libs
IFLAGS += -Ider_libs

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LFLAGS += -DUNICODE -D_UNICODE
endif

# This is required for *some* versions of makedepend
IFLAGS += -DNOMAKEDEPEND

CPPSRC=read_tree.cpp \
der_libs/common_funcs.cpp \
der_libs/conio_min.cpp \
der_libs/qualify.cpp

OBJS = $(CPPSRC:.cpp=.o)

BASE=read_tree
BINX = $(BASE).exe

LIBS=-lshlwapi

DIST_ZIP := $(BASE)V$(VERSION).zip

# Force these action-only targets to always run
.PHONY: dist

#**************************************************************************
%.o: %.cpp
	$(TOOLS)\$(GNAME) $(CFLAGS) $< -o $@

all: $(BINX)

clean:
	rm -f $(OBJS) *.exe *~ *.zip

dist:
	rm -f *.zip
	zip $(DIST_ZIP) $(BINX) readme.md LICENSE.txt CHANGELOG.md

wc:
	wc -l $(CPPSRC)

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:/llvm/bin/clang-tidy.exe $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC)

$(BINX): $(OBJS)
	$(TOOLS)/$(GNAME) $(OBJS) $(LFLAGS) -o $(BINX) $(LIBS) 

# DO NOT DELETE

read_tree.o: der_libs/common.h der_libs/conio_min.h der_libs/qualify.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/conio_min.o: der_libs/common.h der_libs/conio_min.h
der_libs/qualify.o: der_libs/common.h der_libs/qualify.h
