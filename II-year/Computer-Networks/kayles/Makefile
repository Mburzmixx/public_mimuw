CXX 			= g++
CXXFLAGS 	= -std=c++20 -Wall -Wextra -Werror
LDFLAGS 	= 

.PHONY: all clean

SRCS = kayles_client.cpp kayles_server.cpp parsing_utility.cpp connection_utility.cpp
OBJS_CLIENT = kayles_client.o parsing_utility.o connection_utility.o err.o
OBJS_SERVER = kayles_server.o parsing_utility.o connection_utility.o err.o server_utility.o


all: kayles_client kayles_server

kayles_client: $(OBJS_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_CLIENT)

kayles_server: $(OBJS_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS_SERVER)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o kayles_client kayles_server

format:
	clang-format -i *.cpp *.hpp
