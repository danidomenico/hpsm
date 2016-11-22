#pragma once

#include "parallel_const.hpp"

//BACKEND
#if defined BACKEND_CUDA
	const Parallel_Backend BACKEND_OPT = PARALLEL_BACK_CUDA;
	#define PARALLEL_FUNCTION __device__

#elif defined BACKEND_OPENMP
	const Parallel_Backend BACKEND_OPT = PARALLEL_BACK_OPENMP;
	#define PARALLEL_FUNCTION

#elif defined BACKEND_SERIAL
	const Parallel_Backend BACKEND_OPT = PARALLEL_BACK_SERIAL;
	#define PARALLEL_FUNCTION

#elif defined BACKEND_STARPU
	const Parallel_Backend BACKEND_OPT = PARALLEL_BACK_STARPU;
	#define PARALLEL_FUNCTION __host__ __device__

#else
	const Parallel_Backend BACKEND_OPT = PARALLEL_BACK_SERIAL;
	#define PARALLEL_FUNCTION

#endif

//NUMBER OF FUNCTORS
#if defined NUM_FUNCTORS
	const unsigned NUMBER_FUNCTORS = NUM_FUNCTORS;
#else
	const unsigned NUMBER_FUNCTORS = 5;
#endif