/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010, 2011-2012, 2014-2015  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013, 2014  CNRS
 *
 * StarPU is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * StarPU is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include "hpsm.hpp"

#include <iostream>
#include <random>
#include <cmath>
#include <sys/time.h>

using TYPE = float;

void fillArray(TYPE* pfData, int iSize) {
	const TYPE fScale = 1.0f / (TYPE)RAND_MAX;
	for(int i=0; i<iSize; ++i)
		pfData[i] = fScale * rand();
}

void axpy(TYPE a, const TYPE *x, TYPE *y, int size) {
	for(int i=0; i<size; ++i)
		y[i] = a * x[i] + y[i];
}

void checkResult(const TYPE *y_api, const TYPE *y_serial, int size) {
	std::cout << "Verifying...\n";
	bool ok = true;
	for(int i=0; i<size; ++i) {
		TYPE diff = std::abs(y_api[i] - y_serial[i]);
		if(diff > 0.0001f) {
			printf("Error position %d - expected: %f | calculated: %f\n", i, y_serial[i], y_api[i]);
			ok = false;
		}
	}
	
	if(ok)
		std::cout << "Verification OK\n";
}

long long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

using View = hpsm::View<TYPE>;

struct funcAXPY : hpsm::Functor {
	View x, y;
	TYPE a;
	
	funcAXPY(View _x, View _y, TYPE _a) : x(_x), y(_y), a(_a) {
		register_data(x, y);
    }
    
	funcAXPY(const funcAXPY& other) : hpsm::Functor(other), x(other.x), y(other.y), a(other.a) {
		clear_data();
		register_data(x, y);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		y(idx) = x(idx) * a + y(idx);
	};
};

int main(int argc, char **argv) {
	int size, block_size;
	bool serial = false;
	
	if(argc != 4 || (size = atoi(argv[1])) <= 0 || (block_size = atoi(argv[2])) <= 0 ) {
		std::cerr << "Usage: " << argv[0] << " <size> <block_size> <serial>\n";
		std::abort();
	}

#ifdef VERBOSE
	int aux = atoi(argv[3]);
	serial = aux != 0 ? true : false;
#endif

	TYPE a = 3.41;
	TYPE *x, *y, *check;
	
	x = new TYPE[size];
	y = new TYPE[size];
	if(serial)
		check  = new TYPE[size];

	fillArray(x, size);
	fillArray(y, size);
	if(serial) {
		for(int i=0; i<size; i++)
			check[i] = y[i];
	}
		
#ifdef VERBOSE
	std::cout << "Backend: " << parallel_Backend_Str[BACKEND_OPT] << std::endl;
#endif
	
	hpsm::initialize();
	
	View view_x(x, size, block_size, hpsm::AccessMode::In);
	View view_y(y, size, block_size, hpsm::AccessMode::InOut);
	funcAXPY func(view_x, view_y, a); 

#ifdef VERBOSE
	std::cout << "Parallel for:" << std::endl;
#endif
	hpsm::range<1> rg(size);
	
	long long start_time = get_time();
	hpsm::parallel_for(rg, view_y.block_range(), func);
	long long end_time = get_time();
	
	func.remove_data();  
	hpsm::finalize();

	float total_time = ((float) (end_time - start_time)) / (1000*1000);

#ifdef VERBOSE
	printf("AXPY calculated in: %.5f\n", total_time);
#else
	printf("BENCH=axpy;backend=%s;size=%d;block_size=%d;iterations=%d;threads=%d;gpus=%d;time=%.8f\n", 
		parallel_Backend_Str[BACKEND_OPT], size, block_size, 1, hpsm::num_workers_cpu(), hpsm::num_workers_gpu(), total_time);
#endif
	
	if(serial) {
		printf("SERIAL - Calculating AXPY\n");
		
		start_time = get_time();
		axpy(a, x, check, size);
		end_time = get_time();
		
		float total_time_serial = ((float) (end_time - start_time)) / (1000*1000);
		printf("SERIAL - AXPY calculated in: %.5f\n", total_time_serial);
		printf("SERIAL - Speedup: %.5f\n", total_time_serial/total_time);
		
		checkResult(y, check, size);
	}

	delete[] x; delete[] y;
	if(serial)
		delete[] check;
	
	return 0;
}
