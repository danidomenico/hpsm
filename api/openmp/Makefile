OPENMP_BACKEND = openmp_backend.o openmp_functor.o
OPENMP_FLAGS   = -fopenmp -O3

all: $(OPENMP_BACKEND)

%.o: %.cpp 
	g++ $(DEBUG) -std=c++11 $(OPENMP_FLAGS) $^ -c 

clean:
	rm -rf *.o
