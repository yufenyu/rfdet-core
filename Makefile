#
# @Done: (1) use personal independent configs
#       (2) use codes '%'(onefile) '*'(allfile) '$<'(source) '$@'(target)
#

include makefile.local.config
CC=gcc
CPP=g++
#CFLAGS=-c -O3 -fPIC -g -rdynamic
#USING_GDB=-D_GDB_DEBUG
CFLAGS=-c -O0 -fPIC -g -I ./Heap-Layers-master -DNDEBUG -std=c++0x 
LDFLAGS=-ldl -pthread -O0

#SRC=globals.cpp signal.cpp difflog.cpp runtime.cpp heaps.cpp vecotrclock.cpp hook.cpp simpleheap.cpp globalprivate.cpp
OBJS=hbdet.o slice.o runtime.o heaps.o hook.o modification.o
OBJS+=thread.o writeset.o detsync.o strategy.o detruntime.o
#HEADERS=utils.h runtime.h defines.h vectorclock.h hook.h simpleheap.h globalprivate.h

#TARGET=libhbdet_zx.so
INSHEADER=adhocsync.h

#OBJDIR="${shell cd}/obj"

all:${TARGET}


${TARGET}: ${OBJS}
	${CPP} -shared ${LDFLAGS} ${OBJS} -o ${TARGET}

%.o: %.cpp *.h
	${CPP} ${CFLAGS} -o $@ $<
	
#hbdet.o: hbdet.cpp ${HEADERS}
#	g++ ${CFLAGS} hbdet.cpp -o hbdet.o
	
#runtime.o: runtime.cpp ${HEADERS}
#	g++ ${CFLAGS} runtime.cpp -o runtime.o

#globals.o: globals.cpp globals.h ${HEADERS}
#	g++ ${CFLAGS} globals.cpp -o globals.o

#signal.o: signal.cpp signal.h ${HEADERS}
#	g++ ${CFLAGS} signal.cpp -o signal.o

#difflog.o: difflog.cpp difflog.h ${HEADERS}
#	g++ ${CFLAGS} difflog.cpp -o difflog.o
	
#heaps.o: heaps.cpp heaps.h ${HEADERS}
#	g++ ${CFLAGS} heaps.cpp -o heaps.o
	
#vectorclock.o: vectorclock.cpp vectorclock.h ${HEADERS}
#	g++ ${CFLAGS} vectorclock.cpp -o vectorclock.o
	
clean:
	rm -f *.o
	rm -f ${TARGET}
	
install:
	cp ${TARGET} /home/zhouxu/lib/
#	cp ${INSHEADER} /home/zhouxu/include
