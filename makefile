CXX = g++				# compiler
CXXFLAGS = -std=c++11   # compiler flags

OBJECTS1 = binder.o     # object files forming executable
EXEC1 = binder			# given executable name

OBJECTS2 = rpc_server.o message_lib.o
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