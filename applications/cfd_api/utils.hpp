#pragma once

#include <cstdlib>
#include <sys/time.h>

#include <fstream>
#include <iostream>

/*
 * Types
 */
struct float_3 { float x, y, z; };

struct variable {
	float density;
	float momentum_0;
	float momentum_1;
	float momentum_2;
	float density_energy;
};

/*
 * Constants
 */

//Options
const float GAMMA    = 1.4;
//const int iterations = 2000;
const int VOLUME_REP = 30;

const int NDIM = 3;
const int NNB  = 4;

const int RK                    = 3; //3rd order RK 
const float FF_MACH             = 1.2;
const float DEG_ANGLE_OF_ATTACK = 0.0f;

//Not options
const int VAR_DENSITY        = 0;
const int VAR_MOMENTUM       = 1;
const int VAR_DENSITY_ENERGY = VAR_MOMENTUM + NDIM;
const int NVAR               = VAR_DENSITY_ENERGY + 1;

/*
 * Templated functions
 */
template<typename T> 
T* alloc(int N) {
	return new T[N];
}

template<typename T>
void dealloc(T* array) {
	delete[] array;
}

/*
 * Functions
 */
void read_input_file(const char* data_file_name, int size, 
					float* areas, int* elements_surrounding_elements, float* normals);

void initialize_variables(int nelr, float* variables, float* ff_variable);

void initialize_variables(int nelr, float* variables, variable ff_variable);

long get_time();