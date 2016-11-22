CXX= g++
LINK = $(CXX)
EXE = euler3d.sequential
CXXFLAGS = -g -O3 -std=c++11

CXXFLAGS += -DSEQUENTIAL

#CXXFLAGS += -DVERBOSE 
#CXXFLAGS += -pg
#LDFLAGS = -pg

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

OBJ_ROOT = ../utils.o ../serial.o ../euler3d.o
OBJ_ROOT_LINK = utils.o serial.o euler3d.o

default: $(EXE)

$(EXE): $(OBJ) $(OBJ_ROOT)
	$(LINK) $(LDFLAGS) $(OBJ) $(OBJ_ROOT_LINK) -o $@

%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *o $(EXE)

clean-o:
	rm -rf *o
