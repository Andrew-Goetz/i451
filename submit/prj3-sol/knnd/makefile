CC = gcc
OBJECT_FILES = main.o
EXECUTABLE = knnd

#all:	$(OBJECT_FILES)
#	$(CC) -I "../../../../cs551/include/" $(OBJECT_FILES) -o $(EXECUTABLE)
#
#main.o:	main.c
#	$(CC) -c main.c -L "../../../../cs551/lib/" -lknn_ocr -lcs551

all:	$(OBJECT_FILES)
	$(CC) -L "../../../../cs551/lib/" $(OBJECT_FILES) -lknn_ocr -lcs551 -o $(EXECUTABLE)

main.o:	main.c
	$(CC) -c main.c -I "../../../../cs551/include/"

clean:
	rm -f *.o $(EXECUTABLE)
