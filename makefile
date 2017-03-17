CXX = g++				# compiler
CXXFLAGS = -std=c++11 -pthread  # compiler flags

OBJECTS1 = binder.o message_lib.o argT.o skeletonData.o functionData.o serverData.o
EXEC1 = binder

OBJECTS2 = rpc_server.o rpc_client.o message_lib.o argT.o skeletonData.o functionData.o serverData.o
EXEC2 = librpc.a

OBJECTS = ${OBJECTS1} ${OBJECTS2}
DEPENDS = ${OBJECTS:.o=.d}
EXECS = ${EXEC1} ${EXEC2}

#############################################################

.PHONY : all clean

all : ${EXECS}				# build all executables

${EXEC1} : ${OBJECTS1}
	${CXX} ${CXXFLAGS} $^ -o $@

${EXEC2} : ${OBJECTS2}
	${CXX} ${CXXFLAGS} $^ -c
	ar -cvq $@ ${OBJECTS2}

#############################################################

${OBJECTS} : ${MAKEFILE_NAME}

-include ${DEPENDS}			# include *.d files containing program dependences

clean :					# remove files that can be regenerated
	rm -f *.d *.o ${EXEC1} ${EXEC2}
