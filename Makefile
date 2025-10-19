CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pedantic
TARGET = order_book_test
SOURCES = main.cpp order_book.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = order_book.h

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: run
