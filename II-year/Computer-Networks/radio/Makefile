CXX 			:= g++
CXXFLAGS 	:= -std=c++20 -Wall -Wextra -O2 -pedantic #-Werror
LDFLAGS 	:= -lssl -lcrypto

.PHONY: all clean format

TARGET := sikradio
SRCS 	 := radio.cpp controller.cpp http_client.cpp http_utility.cpp stream_processor.cpp input_handler.cpp connection.cpp logger.cpp arg_parsing.cpp 
OBJ := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

format:
	clang-format -i *.cpp *.hpp

