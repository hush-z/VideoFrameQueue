CXX=g++
CFLAGS=-std=c++11
LDFLAGS=`pkg-config --libs opencv`
TARGET=process
$(TARGET):
	$(CXX) $(CFLAGS)  process.cpp -o $(TARGET) $(LDFLAGS)
clean:
	rm $(TARGET)	
