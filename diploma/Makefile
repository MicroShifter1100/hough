CC         := g++
CFLAGS     := -I/usr/local/include/opencv4 -L ./  -Wl,-rpath,./  -O0
LIBRARIES  := -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_dnn -lopencv_videoio -lopencv_imgcodecs

.PHONY: all clean

all: clean test

test:
	$(CC) $(CFLAGS) -o test draft2.cpp $(LIBRARIES)

test_parallel:
	$(CC) $(CFLAGS) --openmp -o test_parallel draft3.cpp $(LIBRARIES)

clean:
	rm ./test