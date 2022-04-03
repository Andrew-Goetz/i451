CC = gcc
OBJECT_FILES = main.o
EXECUTABLE = knnc

all:	$(OBJECT_FILES)
	$(CC) -L "../../../../cs551/lib/" $(OBJECT_FILES) -lknn_ocr -lcs551 -o $(EXECUTABLE)

main.o:	main.c
	$(CC) -c main.c -I "../../../../cs551/include/"

clean:
	rm -f *.o $(EXECUTABLE)
