#include "euler3d.hpp"

/*
 * Functions
 */
#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
void compute_flux_contribution(float density, float_3 momentum, float density_energy, 
							   float pressure, float_3 velocity, 
							   float_3* fc_momentum_x, float_3* fc_momentum_y, float_3* fc_momentum_z, 
							   float_3* fc_density_energy) {
	fc_momentum_x->x = velocity.x * momentum.x + pressure;
	fc_momentum_x->y = velocity.x * momentum.y;
	fc_momentum_x->z = velocity.x * momentum.z;

	fc_momentum_y->x = fc_momentum_x->y;
	fc_momentum_y->y = velocity.y * momentum.y + pressure;
	fc_momentum_y->z = velocity.y * momentum.z;

	fc_momentum_z->x = fc_momentum_x->z;
	fc_momentum_z->y = fc_momentum_y->z;
	fc_momentum_z->z = velocity.z * momentum.z + pressure;

	float de_p = density_energy + pressure;
	fc_density_energy->x = velocity.x * de_p;
	fc_density_energy->y = velocity.y * de_p;
	fc_density_energy->z = velocity.z * de_p;
}

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
void compute_velocity(float density, float_3 momentum, float_3* velocity) {
	velocity->x = momentum.x / density;
	velocity->y = momentum.y / density;
	velocity->z = momentum.z / density;
}

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_speed_sqd(float_3 velocity) {
	return velocity.x * velocity.x + 
		   velocity.y * velocity.y + 
		   velocity.z * velocity.z;
}

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_pressure(float density, float density_energy, float speed_sqd) {
	return (GAMMA - 1.0f) * 
		   (density_energy - float(0.5f) * density * speed_sqd);
}

#ifndef SEQUENTIAL
PARALLEL_FUNCTION
#endif
float compute_speed_of_sound(float density, float pressure) {
	return std::sqrt(GAMMA * pressure / density);
}