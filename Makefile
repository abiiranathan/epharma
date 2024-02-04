CC=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Werror -pedantic -g -Wno-missing-field-initializers
LDFLAGS=-lsqlite3 -lpthread -lgtest -lgtest_main
TARGET=epharma
SRCS=epharma.cpp epharma_test.cpp
OBJS=$(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean