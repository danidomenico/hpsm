#pragma once

#ifndef SEQUENTIAL
#include "hpsm.hpp"
#endif
#include "utils.hpp"

#include <cmath>

/*
 * Functions
 */
#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
void compute_flux_contribution(float density, float_3 momentum, float density_energy, 
							   float pressure, float_3 velocity, 
							   float_3* fc_momentum_x, float_3* fc_momentum_y, float_3* fc_momentum_z, 
							   float_3* fc_density_energy);

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
void compute_velocity(float density, float_3 momentum, float_3* velocity);

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_speed_sqd(float_3 velocity);

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_pressure(float density, float density_energy, float speed_sqd);

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_speed_of_sound(float density, float pressure);