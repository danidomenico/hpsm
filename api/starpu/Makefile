STARPU_BACKEND = starpu_backend.o starpu_functor.o
STARPU_FLAGS   = -O3

all: $(STARPU_BACKEND)

%.o: %.cpp 
	nvcc -x cu -rdc=true -std=c++11 $(DEBUG) $(STARPU_FLAGS) $^ -c 
	
clean:
	rm -rf *.o
