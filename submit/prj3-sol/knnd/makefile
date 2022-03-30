CC = gcc
FLAGS = -03 -f -Wall
OBJECT_FILES = main.o
EXECUTABLE = knnd

all:    $(OBJECT_FILES)
        $(CC) $(OBJECT_FILES) -o $(EXECUTABLE)
                                                                                                                                                  
main.o: main.c
        $(CC) -c main.c

clean:
		rm -f *.o $(EXECUTABLE)
