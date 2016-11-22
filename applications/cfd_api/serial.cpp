#include "serial.hpp"
#include "euler3d.hpp"

/*
 * Serial Functions
 */
void compute_flux_contribution_ser(float density, float_3 momentum, float density_energy, 
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

void compute_velocity_ser(float density, float_3 momentum, float_3* velocity) {
	velocity->x = momentum.x / density;
	velocity->y = momentum.y / density;
	velocity->z = momentum.z / density;
}

float compute_speed_sqd_ser(float_3 velocity) {
	return velocity.x * velocity.x + 
		   velocity.y * velocity.y + 
		   velocity.z * velocity.z;
}

float compute_pressure_ser(float density, float density_energy, float speed_sqd) {
	return (GAMMA - 1.0f) * 
		   (density_energy - float(0.5f) * density * speed_sqd);
}

float compute_speed_of_sound_ser(float density, float pressure) {
	return std::sqrt(GAMMA * pressure / density);
}

void copy_ser(float* dst, float* src, int N) {
	for(int i = 0; i < N; i++) {
		dst[i] = src[i];
	}
}

void compute_step_factor_ser(int nelr, float* variables, float* areas, float* step_factors) {
	for(int i = 0; i < nelr; i++) {
		for(int x=0; x < VOLUME_REP; x++) {
			float density = variables[i*NVAR + VAR_DENSITY]; //[i + VAR_DENSITY*nelr];

			float_3 momentum;
			momentum.x = variables[i*NVAR + (VAR_MOMENTUM+0)]; //[i + (VAR_MOMENTUM+0)*nelr];
			momentum.y = variables[i*NVAR + (VAR_MOMENTUM+1)]; //[i + (VAR_MOMENTUM+1)*nelr];
			momentum.z = variables[i*NVAR + (VAR_MOMENTUM+2)]; //[i + (VAR_MOMENTUM+2)*nelr];

			float density_energy = variables[i*NVAR + VAR_DENSITY_ENERGY]; //[i + VAR_DENSITY_ENERGY*nelr];
			float_3 velocity;
			compute_velocity(density, momentum, &velocity);
			float speed_sqd      = compute_speed_sqd(velocity);
			float pressure       = compute_pressure(density, density_energy, speed_sqd);
			float speed_of_sound = compute_speed_of_sound(density, pressure);

			// dt = float(0.5f) * std::sqrt(areas[i]) /  (||v|| + c).... but when we do time stepping, this later would need to be divided by the area, so we just do it all at once
			step_factors[i] = float(0.5f) / (std::sqrt(areas[i]) * (std::sqrt(speed_sqd) + speed_of_sound));
		}
	}
}

void compute_flux_ser(int nelr, 
				  int* elements_surrounding_elements, float* normals, float* variables, float* fluxes, 
				  float* ff_variable, 
				  float_3 ff_flux_contribution_momentum_x, float_3 ff_flux_contribution_momentum_y, float_3 ff_flux_contribution_momentum_z, 
				  float_3 ff_flux_contribution_density_energy) {
	const float smoothing_coefficient = float(0.2f);
	
	for(int i = 0; i < nelr; ++i) {
		for(int x=0; x < VOLUME_REP; x++) {
			float density_i = variables[i*NVAR + VAR_DENSITY]; //[i + VAR_DENSITY*nelr];
			float_3 momentum_i;
			momentum_i.x = variables[i*NVAR + (VAR_MOMENTUM+0)]; //[i + (VAR_MOMENTUM+0)*nelr];
			momentum_i.y = variables[i*NVAR + (VAR_MOMENTUM+1)]; //[i + (VAR_MOMENTUM+1)*nelr];
			momentum_i.z = variables[i*NVAR + (VAR_MOMENTUM+2)]; //[i + (VAR_MOMENTUM+2)*nelr];

			float density_energy_i = variables[i*NVAR + VAR_DENSITY_ENERGY]; //[i + VAR_DENSITY_ENERGY*nelr];

			float_3 velocity_i; 
			compute_velocity(density_i, momentum_i, &velocity_i);
			
			float speed_sqd_i      = compute_speed_sqd(velocity_i);
			float speed_i          = std::sqrt(speed_sqd_i);
			float pressure_i       = compute_pressure(density_i, density_energy_i, speed_sqd_i);
			float speed_of_sound_i = compute_speed_of_sound(density_i, pressure_i);
			
			float_3 flux_contribution_i_momentum_x, flux_contribution_i_momentum_y, flux_contribution_i_momentum_z;
			float_3 flux_contribution_i_density_energy;
			compute_flux_contribution(density_i, momentum_i, density_energy_i, 
									pressure_i, velocity_i, 
									&flux_contribution_i_momentum_x, &flux_contribution_i_momentum_y, &flux_contribution_i_momentum_z, 
									&flux_contribution_i_density_energy);

			float flux_i_density = float(0.0f);
			float_3 flux_i_momentum;
			flux_i_momentum.x = float(0.0f);
			flux_i_momentum.y = float(0.0f);
			flux_i_momentum.z = float(0.0f);
			float flux_i_density_energy = float(0.0f);

			float_3 velocity_nb;
			float density_nb, density_energy_nb;
			float_3 momentum_nb;
			float_3 flux_contribution_nb_momentum_x, flux_contribution_nb_momentum_y, flux_contribution_nb_momentum_z;
			float_3 flux_contribution_nb_density_energy;
			float speed_sqd_nb, speed_of_sound_nb, pressure_nb;

			//#pragma unroll
			for(int j = 0; j < NNB; j++) {
				float_3 normal; 
				float normal_len;
				float factor;

				int nb = elements_surrounding_elements[i*NNB + j]; //[i + j*nelr];
				normal.x = normals[i*NNB*NDIM + j*NDIM + 0]; //[i + (j + 0*NNB)*nelr]; 
				normal.y = normals[i*NNB*NDIM + j*NDIM + 1]; //[i + (j + 1*NNB)*nelr];
				normal.z = normals[i*NNB*NDIM + j*NDIM + 2]; //[i + (j + 2*NNB)*nelr];
				normal_len = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

				if(nb >= 0) {// a legitimate neighbor
					density_nb        = variables[nb*NVAR + VAR_DENSITY];        //[nb + VAR_DENSITY*nelr];
					momentum_nb.x     = variables[nb*NVAR + (VAR_MOMENTUM+0)];   //[nb + (VAR_MOMENTUM+0)*nelr];
					momentum_nb.y     = variables[nb*NVAR + (VAR_MOMENTUM+1)];   //[nb + (VAR_MOMENTUM+1)*nelr];
					momentum_nb.z     = variables[nb*NVAR + (VAR_MOMENTUM+2)];   //[nb + (VAR_MOMENTUM+2)*nelr];
					density_energy_nb = variables[nb*NVAR + VAR_DENSITY_ENERGY]; //[nb + VAR_DENSITY_ENERGY*nelr];
					compute_velocity(density_nb, momentum_nb, &velocity_nb);
				
					speed_sqd_nb      = compute_speed_sqd(velocity_nb);
					pressure_nb       = compute_pressure(density_nb, density_energy_nb, speed_sqd_nb);
					speed_of_sound_nb = compute_speed_of_sound(density_nb, pressure_nb);
					compute_flux_contribution(density_nb, momentum_nb, density_energy_nb, 
											pressure_nb, velocity_nb, 
											&flux_contribution_nb_momentum_x, &flux_contribution_nb_momentum_y, &flux_contribution_nb_momentum_z, 
											&flux_contribution_nb_density_energy);

					// artificial viscosity
					factor = -normal_len * smoothing_coefficient * float(0.5f) * 
								(speed_i + std::sqrt(speed_sqd_nb) + speed_of_sound_i + speed_of_sound_nb);
					flux_i_density        += factor*(density_i-density_nb);
					flux_i_density_energy += factor*(density_energy_i-density_energy_nb);
					flux_i_momentum.x     += factor*(momentum_i.x-momentum_nb.x);
					flux_i_momentum.y     += factor*(momentum_i.y-momentum_nb.y);
					flux_i_momentum.z     += factor*(momentum_i.z-momentum_nb.z);

					// accumulate cell-centered fluxes
					factor                 = float(0.5f) * normal.x;
					flux_i_density        += factor * (momentum_nb.x + momentum_i.x);
					flux_i_density_energy += factor * (flux_contribution_nb_density_energy.x + flux_contribution_i_density_energy.x);
					flux_i_momentum.x     += factor * (flux_contribution_nb_momentum_x.x + flux_contribution_i_momentum_x.x);
					flux_i_momentum.y     += factor * (flux_contribution_nb_momentum_y.x + flux_contribution_i_momentum_y.x);
					flux_i_momentum.z     += factor * (flux_contribution_nb_momentum_z.x + flux_contribution_i_momentum_z.x);

					factor                 = float(0.5f) * normal.y;
					flux_i_density        += factor * (momentum_nb.y + momentum_i.y);
					flux_i_density_energy += factor * (flux_contribution_nb_density_energy.y + flux_contribution_i_density_energy.y);
					flux_i_momentum.x     += factor * (flux_contribution_nb_momentum_x.y + flux_contribution_i_momentum_x.y);
					flux_i_momentum.y     += factor * (flux_contribution_nb_momentum_y.y + flux_contribution_i_momentum_y.y);
					flux_i_momentum.z     += factor * (flux_contribution_nb_momentum_z.y + flux_contribution_i_momentum_z.y);

					factor                 = float(0.5f) * normal.z;
					flux_i_density        += factor * (momentum_nb.z + momentum_i.z);
					flux_i_density_energy += factor * (flux_contribution_nb_density_energy.z + flux_contribution_i_density_energy.z);
					flux_i_momentum.x     += factor * (flux_contribution_nb_momentum_x.z + flux_contribution_i_momentum_x.z);
					flux_i_momentum.y     += factor * (flux_contribution_nb_momentum_y.z + flux_contribution_i_momentum_y.z);
					flux_i_momentum.z     += factor * (flux_contribution_nb_momentum_z.z + flux_contribution_i_momentum_z.z);
					
				} else if(nb == -1) { // a wing boundary
					flux_i_momentum.x += normal.x * pressure_i;
					flux_i_momentum.y += normal.y * pressure_i;
					flux_i_momentum.z += normal.z * pressure_i;
					
				} else if(nb == -2) { // a far field boundary
					factor                 = float(0.5f) * normal.x;
					flux_i_density        += factor * (ff_variable[VAR_MOMENTUM+0] + momentum_i.x);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.x + flux_contribution_i_density_energy.x);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.x + flux_contribution_i_momentum_x.x);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.x + flux_contribution_i_momentum_y.x);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.x + flux_contribution_i_momentum_z.x);

					factor                 = float(0.5f) * normal.y;
					flux_i_density        += factor * (ff_variable[VAR_MOMENTUM+1] + momentum_i.y);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.y + flux_contribution_i_density_energy.y);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.y + flux_contribution_i_momentum_x.y);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.y + flux_contribution_i_momentum_y.y);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.y + flux_contribution_i_momentum_z.y);

					factor                 = float(0.5f) * normal.z;
					flux_i_density        += factor * (ff_variable[VAR_MOMENTUM+2] + momentum_i.z);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.z + flux_contribution_i_density_energy.z);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.z + flux_contribution_i_momentum_x.z);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.z + flux_contribution_i_momentum_y.z);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.z + flux_contribution_i_momentum_z.z);
				}
			}
			
			fluxes[i*NVAR + VAR_DENSITY]        = flux_i_density;
			fluxes[i*NVAR + (VAR_MOMENTUM+0)]   = flux_i_momentum.x;
			fluxes[i*NVAR + (VAR_MOMENTUM+1)]   = flux_i_momentum.y;
			fluxes[i*NVAR + (VAR_MOMENTUM+2)]   = flux_i_momentum.z;
			fluxes[i*NVAR + VAR_DENSITY_ENERGY] = flux_i_density_energy;
			/*
			fluxes[i + VAR_DENSITY*nelr]        = flux_i_density;
			fluxes[i + (VAR_MOMENTUM+0)*nelr]   = flux_i_momentum.x;
			fluxes[i + (VAR_MOMENTUM+1)*nelr]   = flux_i_momentum.y;
			fluxes[i + (VAR_MOMENTUM+2)*nelr]   = flux_i_momentum.z;
			fluxes[i + VAR_DENSITY_ENERGY*nelr] = flux_i_density_energy;
			*/
		}
	}	
}

void time_step_ser(int j, int nelr, float* old_variables, float* variables, float* step_factors, float* fluxes) {
	for(int i = 0; i < nelr; ++i) {
		for(int x=0; x < VOLUME_REP; x++) {
			float factor = step_factors[i]/float(RK+1-j);

			variables[i*NVAR + VAR_DENSITY]        = old_variables[i*NVAR + VAR_DENSITY]        + factor * fluxes[i*NVAR + VAR_DENSITY];
			variables[i*NVAR + (VAR_MOMENTUM+0)]   = old_variables[i*NVAR + (VAR_MOMENTUM+0)]   + factor * fluxes[i*NVAR + (VAR_MOMENTUM+0)];
			variables[i*NVAR + (VAR_MOMENTUM+1)]   = old_variables[i*NVAR + (VAR_MOMENTUM+1)]   + factor * fluxes[i*NVAR + (VAR_MOMENTUM+1)];
			variables[i*NVAR + (VAR_MOMENTUM+2)]   = old_variables[i*NVAR + (VAR_MOMENTUM+2)]   + factor * fluxes[i*NVAR + (VAR_MOMENTUM+2)];
			variables[i*NVAR + VAR_DENSITY_ENERGY] = old_variables[i*NVAR + VAR_DENSITY_ENERGY] + factor * fluxes[i*NVAR + VAR_DENSITY_ENERGY];
			
			/*
			variables[i + VAR_DENSITY*nelr]        = old_variables[i + VAR_DENSITY*nelr]        + factor * fluxes[i + VAR_DENSITY*nelr];
			variables[i + (VAR_MOMENTUM+0)*nelr]   = old_variables[i + (VAR_MOMENTUM+0)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+0)*nelr];
			variables[i + (VAR_MOMENTUM+1)*nelr]   = old_variables[i + (VAR_MOMENTUM+1)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+1)*nelr];
			variables[i + (VAR_MOMENTUM+2)*nelr]   = old_variables[i + (VAR_MOMENTUM+2)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+2)*nelr];
			variables[i + VAR_DENSITY_ENERGY*nelr] = old_variables[i + VAR_DENSITY_ENERGY*nelr] + factor * fluxes[i + VAR_DENSITY_ENERGY*nelr];
			*/
		}
	}
}

void run_serial_and_verify(const char* data_file_name, int nelr, int iterations, 
						   float* variables_dev, double time_dev) {
	float* variables = alloc<float>(nelr * NVAR);
	
	std::cout << "Calculating CFD serial...\n";
	double time_ser = run_serial(data_file_name, nelr, iterations, variables);
	
	printf("Speedup: %g \n", time_ser/time_dev);
	
	//Compare results
	std::cout << "Verifying...\n";
	bool ok = true;
	for(int i=0; i<nelr; i++) {
		if(abs(variables[i*NVAR + VAR_DENSITY] - variables_dev[i*NVAR + VAR_DENSITY]) > 0.00001f) {
			printf("density %6d Expected: %.7f | Calculated: %.7f\n", 
				   i, variables[i*NVAR + VAR_DENSITY], variables_dev[i*NVAR + VAR_DENSITY]);
			ok = false;
		}
			
		if( abs(variables[i*NVAR + (VAR_MOMENTUM+0)] - variables_dev[i*NVAR + (VAR_MOMENTUM+0)]) > 0.00001f ||
			abs(variables[i*NVAR + (VAR_MOMENTUM+1)] - variables_dev[i*NVAR + (VAR_MOMENTUM+1)]) > 0.00001f ||
			abs(variables[i*NVAR + (VAR_MOMENTUM+2)] - variables_dev[i*NVAR + (VAR_MOMENTUM+2)]) > 0.00001f) {
			printf("momentun %6d Expected: %.7f %.7f %.7f | Calculated: %.7f %.7f %.7f\n", 
					i, variables[i*NVAR + (VAR_MOMENTUM+0)], variables[i*NVAR + (VAR_MOMENTUM+1)], variables[i*NVAR + (VAR_MOMENTUM+2)],
					   variables_dev[i*NVAR + (VAR_MOMENTUM+0)], variables_dev[i*NVAR + (VAR_MOMENTUM+1)], variables_dev[i*NVAR + (VAR_MOMENTUM+2)]);
			ok = false;
		}
			
		if(abs(variables[i*NVAR + VAR_DENSITY_ENERGY] - variables_dev[i*NVAR + VAR_DENSITY_ENERGY]) > 0.00001f) {
			printf("density_energy %6d Expected: %.7f | Calculated: %.7f\n", 
				   i, variables[i*NVAR + VAR_DENSITY_ENERGY], variables_dev[i*NVAR + VAR_DENSITY_ENERGY]);
			ok = false;
		}
	}
	
	if(ok)
		std::cout << "Verification OK\n";
	
	dealloc<float>(variables);
}

double run_serial(const char* data_file_name, int nelr, int iterations, 
				  float* variables) {
#ifdef VERBOSE
	printf("nelr: %d\n", nelr);
#endif
	float ff_variable[NVAR];
	float_3 ff_flux_contribution_momentum_x, ff_flux_contribution_momentum_y, ff_flux_contribution_momentum_z;
	float_3 ff_flux_contribution_density_energy;

	// set far field conditions
	const float angle_of_attack = float(3.1415926535897931 / 180.0f) * float(DEG_ANGLE_OF_ATTACK);

	ff_variable[VAR_DENSITY] = float(1.4);

	float ff_pressure = float(1.0f);
	float ff_speed_of_sound = std::sqrt(GAMMA*ff_pressure / ff_variable[VAR_DENSITY]);
	float ff_speed = float(FF_MACH)*ff_speed_of_sound;

	float_3 ff_velocity;
	ff_velocity.x = ff_speed*float(cos((float)angle_of_attack));
	ff_velocity.y = ff_speed*float(sin((float)angle_of_attack));
	ff_velocity.z = 0.0f;

	ff_variable[VAR_MOMENTUM+0] = ff_variable[VAR_DENSITY] * ff_velocity.x;
	ff_variable[VAR_MOMENTUM+1] = ff_variable[VAR_DENSITY] * ff_velocity.y;
	ff_variable[VAR_MOMENTUM+2] = ff_variable[VAR_DENSITY] * ff_velocity.z;

	ff_variable[VAR_DENSITY_ENERGY] = ff_variable[VAR_DENSITY]*(float(0.5f)*(ff_speed*ff_speed)) + (ff_pressure / float(GAMMA-1.0f));

	float_3 ff_momentum;
	ff_momentum.x = *(ff_variable+VAR_MOMENTUM+0);
	ff_momentum.y = *(ff_variable+VAR_MOMENTUM+1);
	ff_momentum.z = *(ff_variable+VAR_MOMENTUM+2);
	compute_flux_contribution(ff_variable[VAR_DENSITY], ff_momentum, ff_variable[VAR_DENSITY_ENERGY], 
							  ff_pressure, ff_velocity, 
							  &ff_flux_contribution_momentum_x, &ff_flux_contribution_momentum_y, &ff_flux_contribution_momentum_z, 
							  &ff_flux_contribution_density_energy);
	
	float* areas                       = alloc<float>(nelr);         //new float[nelr];
	int* elements_surrounding_elements = alloc<int>(nelr * NNB);     // new int[nelr*NNB];
	float* normals                     = alloc<float>(nelr * NNB * NDIM); // new float[NDIM*NNB*nelr];

#ifdef VERBOSE
	std::cout << "SERIAL - Reading input file..." << std::endl;
#endif
	
	read_input_file(data_file_name, nelr, 
					areas, elements_surrounding_elements, normals);

#ifdef VERBOSE
	std::cout << "SERIAL - Initializing..." << std::endl;
#endif

	// Create arrays and set initial conditions
	//float* variables = alloc<float>(nelr * NVAR);
	initialize_variables(nelr, variables, ff_variable);

	float* old_variables = alloc<float>(nelr * NVAR);
	float* fluxes        = alloc<float>(nelr * NVAR);
	float* step_factors  = alloc<float>(nelr);

	long start = get_time();
	
#ifdef VERBOSE
	// these need to be computed the first time in order to compute time step
	std::cout << "SERIAL - Starting iterations..." << std::endl;
#endif
	
	// Begin iterations
	for(int i = 0; i < iterations; i++) {
		copy_ser(old_variables, variables, nelr * NVAR);

		// for the first iteration we compute the time step
		compute_step_factor_ser(nelr, variables, areas, step_factors);

		for(int j = 0; j < RK; j++) {
			compute_flux_ser(nelr, 
						 elements_surrounding_elements, normals, variables, fluxes, 
						 ff_variable, 
						 ff_flux_contribution_momentum_x, ff_flux_contribution_momentum_y, ff_flux_contribution_momentum_z, 
						 ff_flux_contribution_density_energy);
			time_step_ser(j, nelr, old_variables, variables, step_factors, fluxes);
		}
	}

	long end = get_time();
	double time = (end - start) / 1000000.0;

#ifdef VERBOSE
	std::cout  << "SERIAL - Compute time: " << time << std::endl;

	std::cout << "SERIAL - Cleaning up..." << std::endl;
#endif
	
	dealloc<float>(areas);
	dealloc<int>(elements_surrounding_elements);
	dealloc<float>(normals);

	dealloc<float>(old_variables);
	dealloc<float>(fluxes);
	dealloc<float>(step_factors);

	return time;
}