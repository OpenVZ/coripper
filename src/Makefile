INSTALL = install
BINDIR = $(DESTDIR)/usr/bin

CPP = g++
CPPFLAGS = -pipe -Werror -Wall -Wextra -Winline -Wcast-align -Wno-unused-parameter -Wunused-variable -g2
INC = -I../include

default: all

.stamp-debug:
	$(MAKE) clean depend-cpp
	touch $@

.stamp-cpp:
	$(MAKE) clean depend-cpp
	touch $@

debug: CPPFLAGS += -DDEBUG -g
debug: .stamp-debug coripper

all: .stamp-cpp coripper

coripper: coripper.o core_segments.o core_reader.o main.cpp
	$(CPP) $(CPPFLAGS) $(INC) main.cpp *.o -lelf -o $@

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INC) -c $< -o $@

install:
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) coripper $(BINDIR)/coripper

depend-cpp:
	$(CPP) $(CPPFLAGS) $(INC) -M $ *.cpp >.depend

-include .depend

clean:
	rm -rf *.o coripper .depend .stamp*

.PHONY: clean depend-cpp install all debug default
