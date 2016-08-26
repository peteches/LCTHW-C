CFLAGS=-Wall -g

ex=$(shell basename ${PWD})

all: clean build

build:
	cc ${ex}.c -o ${ex}

clean:
	rm -f ${ex}
