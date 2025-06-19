#SHELL=cmd.exe
USE_DEBUG = NO

ifeq ($(USE_DEBUG),YES)
CFLAGS=-Wall -O -g
LFLAGS=
else
CFLAGS=-Wall -O2
LFLAGS=-s
endif
CFLAGS += -DUNICODE -D_UNICODE
CFLAGS += -Wno-write-strings
CFLAGS += -Wno-stringop-truncation

CSRC=serial_test.cpp serial_enum.cpp serial_iface.cpp common_funcs.cpp config.cpp

OBJS = $(CSRC:.cpp=.o)

BIN=serial_test.exe

#************************************************************
%.o: %.c
	gcc $(CFLAGS) -c $<

%.o: %.cpp
	g++ $(CFLAGS) -c $<

all: $(BIN)

clean:
	rm.exe -f *.exe *.zip *.bak $(OBJS) 

lint:
	cmd /C "c:\lint9\lint-nt +v -width(160,4) -ic:\lint9 mingw.lnt -os(_lint.tmp) lintdefs.cpp $(CSRC)"

depend:
	makedepend $(CFLAGS) $(CSRC)

#************************************************************

$(BIN): $(OBJS)
	g++ $(CFLAGS) -s $(OBJS) -o $@ -lsetupapi -lgdi32

# DO NOT DELETE

serial_test.o: common.h serial_enum.h serial_iface.h
serial_enum.o: common.h serial_enum.h
serial_iface.o: common.h serial_enum.h
common_funcs.o: common.h
config.o: common.h serial_iface.h
