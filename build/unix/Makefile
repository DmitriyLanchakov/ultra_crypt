
CXX = g++
MPICC = mpic++
CFLAGS = -Wall -g -lstdc++ -std=c++11 -O3 -pthread
CFLAGS_MP = $(CFLAGS) -fopenmp -lgomp

PROJECT_ROOT = ../../
SRC_DIR = $(PROJECT_ROOT)src/
BIN_DIR = $(PROJECT_ROOT)bin/

SRC_TEST = $(SRC_DIR)brute_test.cpp
SRC_MD5 = $(SRC_DIR)brute_md5.cpp $(SRC_DIR)md5.cpp
SRC_MD5_MP = $(SRC_DIR)brute_md5_mp.cxx $(SRC_DIR)md5.cpp

OBJS_TEST = $(SRC_TEST:.cpp=.o)
OBJS_MD5 = $(SRC_MD5:.cpp=.o)
OBJS_MD5_MP = $(SRC_MD5:.cxx=.o)

TESTS_BIN = brute_test
BRUTE_MD5_BIN = brute_md5
BRUTE_MD5_MP_BIN = brute_md5_mp

.PHONY: depend clean

all:	$(TESTS_BIN) $(BRUTE_MD5_BIN) $(BRUTE_MD5_MP_BIN)
	@echo  Simple compiler named mycc has been compiled

$(TESTS_BIN): $(OBJS_TEST) 
	$(CXX) $(CFLAGS) -o $(BIN_DIR)$(TESTS_BIN) $(OBJS_TEST)

$(BRUTE_MD5_BIN): $(OBJS_MD5)
	$(CXX) $(CFLAGS_MP) -o $(BIN_DIR)$(BRUTE_MD5_BIN) $(OBJS_MD5)

$(BRUTE_MD5_MP_BIN): $(SRC_DIR)md5_mp.o
	$(MPICC) $(CFLAGS) -c $(SRC_DIR)brute_md5_mp.cpp -o $(SRC_DIR)brute_md5_mp.o
	$(MPICC) $(CFLAGS) -o $(BIN_DIR)$(BRUTE_MD5_MP_BIN) $(SRC_DIR)brute_md5_mp.o $(SRC_DIR)md5.o

$(SRC_DIR)md5_mp.o:
	$(MPICC) $(CFLAGS) -c $(SRC_DIR)md5.cpp  -o md5_mp.o:

.cpp.o:
	$(CXX) $(CFLAGS_MP) -c $<  -o $@


clean:
	$(RM) $(SRC_DIR)/*.o *~ $(MAIN)

depend: $(SRC_TEST)
	makedepend $(INCLUDES) $^

