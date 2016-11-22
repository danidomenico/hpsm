OPENMP_BACKEND = openmp/*.o
SERIAL_BACKEND = serial/*.o
STARPU_BACKEND = starpu/*.o

OBJ_COMPILE = parallel_atomic.o parallel_backend.o parallel_utils.o parallel_view.o
OBJ_INCLUDE = $(OBJ_COMPILE) $(OPENMP_BACKEND) $(SERIAL_BACKEND) $(STARPU_BACKEND) 

LINK_LIBS = -lstarpu-1.2 -Xcompiler -fopenmp
#DEFINES = -DBACKEND_OPENMP -Xcompiler -fopenmp
DEFINES = -DBACKEND_STARPU
#DEFINES = -DBACKEND_SERIAL

C_FLAGS = -O3


all: testlib testlibReduction testlibMatrix

testlib: testlib.o $(OBJ_COMPILE) subs
	nvcc -g $< $(OBJ_INCLUDE) -o $@ $(LINK_LIBS)
	
testlibReduction: testlibReduction.o $(OBJ_COMPILE) subs
	nvcc -g $< $(OBJ_INCLUDE) -o $@ $(LINK_LIBS)

testlibMatrix: testlibMatrix.o $(OBJ_COMPILE) subs
	nvcc -g $< $(OBJ_INCLUDE) -o $@ $(LINK_LIBS)

testlib.o: testlib.cpp 
	nvcc -g -x cu -rdc=true -std=c++11 $^ -c $(DEFINES)
	
testlibReduction.o: testlibReduction.cpp 
	nvcc -g -x cu -rdc=true -std=c++11 $^ -c $(DEFINES)
	
testlibMatrix.o: testlibMatrix.cpp 
	nvcc -g -x cu -rdc=true -std=c++11 $^ -c $(DEFINES)
	
%.o: %.cpp 
	nvcc -g -x cu -rdc=true -std=c++11 $(C_FLAGS) $^ -c 
	
subs: 
	cd openmp; make
	cd serial; make
	cd starpu; make
	
	
lib: $(OBJ_COMPILE) subs
	ar rcs libhpsm.a $(OBJ_INCLUDE)
	
clean:
	cd openmp; make clean
	cd serial; make clean
	cd starpu; make clean
	rm -rf *.o *.a testlib testlibReduction testlibMatrix
