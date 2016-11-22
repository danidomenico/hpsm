#pragma once

#include "utils.hpp"

#include <cmath>

void run_serial_and_verify(const char* data_file_name, int nelr, int iterations, 
						   float* variables_dev, double time_dev);


double run_serial(const char* data_file_name, int nelr, int iterations, 
				  float* variables);
