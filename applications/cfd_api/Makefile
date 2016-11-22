PARALLEL_PATH := ../../api
NAME := "euler3d"
#PARALLEL_BACKEND = "OpenMP"
PARALLEL_BACKEND = "StarPU"
#PARALLEL_BACKEND = "Serial"

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

ifneq (,$(findstring StarPU,$(PARALLEL_BACKEND)))
CXX= nvcc
LINK = $(CXX)
EXE = $(NAME).starpu
CXXFLAGS = -g -x cu -rdc=true -Xcompiler -fopenmp
#CXXFLAGS = -g -x cu -rdc=true
LDFLAGS = -Xcompiler -fopenmp
else
CXX= g++
LINK = $(CXX)
EXE = $(NAME).serial
CXXFLAGS = -g 
endif

ifneq (,$(findstring OpenMP,$(PARALLEL_BACKEND)))
CXX= g++
LINK = $(CXX)
EXE = $(NAME).omp
CXXFLAGS = -g -fopenmp
endif

#CXXFLAGS += -DVERBOSE

default: $(EXE)

include $(PARALLEL_PATH)/Makefile.parallel

$(EXE): $(OBJ) $(PARALLEL_LINK_DEPENDS)
	$(LINK) $(PARALLEL_LDFLAGS) $(LDFLAGS) $(OBJ) $(PARALLEL_LIBS) -o $(EXE)

%.o:%.cpp $(PARALLEL_CPP_DEPENDS)
	$(CXX) $(CXXFLAGS) $(PARALLEL_CPPFLAGS) $(PARALLEL_CXXFLAGS) -c $<

clean: parallel-clean
	rm -f *.o $(EXE)
	
clean-o: parallel-clean
	rm -f *.o
