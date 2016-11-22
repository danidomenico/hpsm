CXX= g++
LINK = $(CXX)
EXE = hotspot.sequential
CXXFLAGS = -g -O3 -std=c++11

#CXXFLAGS += -DVERBOSE

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

default: $(EXE)

$(EXE): $(OBJ)
	$(LINK) $(OBJ) -o $@

%.o: %.cpp 
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *o $(EXE)

clean-o:
	rm -rf *o