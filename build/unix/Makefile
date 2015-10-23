
CXX = g++
CFLAGS = -Wall -g -std=c++11 -O2

PROJECT_ROOT = ../../
SRC_DIR = $(PROJECT_ROOT)src/
BIN_DIR = $(PROJECT_ROOT)bin/
SRC_TEST = $(SRC_DIR)brute_test.cpp

OBJS_TEST = $(SRC_TEST:.cpp=.o)

TESTS_BIN = brute_test

.PHONY: depend clean

all:	$(TESTS_BIN)
	@echo  Simple compiler named mycc has been compiled

$(TESTS_BIN): $(OBJS_TEST) 
	$(CXX) $(CFLAGS) -o $(BIN_DIR)$(TESTS_BIN) $(OBJS_TEST)
	@echo compiling

.cpp.o:
	$(CXX) $(CFLAGS) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRC_TEST)
	makedepend $(INCLUDES) $^

