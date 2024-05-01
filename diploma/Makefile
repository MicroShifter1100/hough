CC         := g++
CFLAGS     := -I/usr/local/include/opencv4 -L /usr/local/lib -O3
LIBRARIES  := -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_dnn -lopencv_videoio -lopencv_imgcodecs

.PHONY: all clean

all: clean test

test:
	$(CC) $(CFLAGS) -o test draft.cpp $(LIBRARIES)

clean:
	rm ./test