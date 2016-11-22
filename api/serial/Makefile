SERIAL_BACKEND = serial_backend.o serial_functor.o
SERIAL_FLAGS   = -O3

all: $(SERIAL_BACKEND)

%.o: %.cpp 
	g++ $(DEBUG) -std=c++11 $(SERIAL_FLAGS) $^ -c 

clean:
	rm -rf *.o
