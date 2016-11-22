#include "utils.hpp"

/*
 * Functions
 */
void read_input_file(const char* data_file_name, int size, 
					float* areas, int* elements_surrounding_elements, float* normals) {
	// read in domain geometry
	std::ifstream file(data_file_name);
	
	int nelr;
	file >> nelr;
	if(size > nelr) {
		std::cout << "Invalid size. The input file has just " << nelr << " inputs\n";
		std::abort();
	}
	
	nelr = size;
	
	// read in data
	for(int i = 0; i < nelr; i++) {
		file >> areas[i];
		for(int j = 0; j < NNB; j++) {
			file >> elements_surrounding_elements[i*NNB + j];  //[i + j*nelr];
			//if(elements_surrounding_elements[i*NNB + j] < 0) 
			if(elements_surrounding_elements[i*NNB + j] < 0 || 
				elements_surrounding_elements[i*NNB + j] > nelr) //Cut input ultil size
				elements_surrounding_elements[i*NNB + j] = -1;
			
			elements_surrounding_elements[i*NNB + j]--; //it's coming in with Fortran numbering

			for(int k = 0; k < NDIM; k++) {
				file >>  normals[i*NNB*NDIM + j*NDIM + k];   //[i + (j + k*NNB)*nelr];
				normals[i*NNB*NDIM + j*NDIM + k] = -normals[i*NNB*NDIM + j*NDIM + k];
			}
		}
	}
}

void initialize_variables(int nelr, float* variables, float* ff_variable) {
	//#pragma omp parallel for default(shared) schedule(static)
	for(int i = 0; i < nelr; i++) {
		for(int j = 0; j < NVAR; j++) 
			variables[i*NVAR + j] = ff_variable[j];
			//variables[i + j*nelr] = ff_variable[j];
	}
}

void initialize_variables(int nelr, float* variables, variable ff_variable) {
	for(int i = 0; i < nelr; i++) {
		for(int j = 0; j < NVAR; j++) {
			//variables[i*NVAR + j] = ff_variable[j];
			//variables[i + j*nelr] = ff_variable[j];
			
			switch (j) {
				case 0:
					variables[i*NVAR + j] = ff_variable.density;
					break;
					
				case 1:
					variables[i*NVAR + j] = ff_variable.momentum_0;
					break;
					
				case 2:
					variables[i*NVAR + j] = ff_variable.momentum_1;
					break;
					
				case 3:
					variables[i*NVAR + j] = ff_variable.momentum_2;
					break;
					
				case 4:
					variables[i*NVAR + j] = ff_variable.density_energy;
					break;
			}
		}
	}
}

long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}