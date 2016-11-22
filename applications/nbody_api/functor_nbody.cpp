#include "functor_nbody.hpp"

/*
 * Functions
 */
PARALLEL_FUNCTION
void calculate_force(Particle* this_particle1, Particle* this_particle2,
					 float* force_x, float* force_y, float* force_z) {
	/* Particle_calculate_force */
	float difference_x, difference_y, difference_z;
	float distance_squared, distance;
	float force_magnitude;

	difference_x = this_particle2->position_x - this_particle1->position_x;
	difference_y = this_particle2->position_y - this_particle1->position_y;
	difference_z = this_particle2->position_z - this_particle1->position_z;

	distance_squared = difference_x * difference_x +
					   difference_y * difference_y +
					   difference_z * difference_z;

	distance = std::sqrt(distance_squared);//sqrtf(distance_squared);

	force_magnitude = GRAVITATIONAL_CONSTANT * (this_particle1->mass) * (this_particle2->mass) / distance_squared;

	*force_x = (force_magnitude / distance) * difference_x;
	*force_y = (force_magnitude / distance) * difference_y;
	*force_z = (force_magnitude / distance) * difference_z;
}

void serial_nbody(Particle* d_particles, Particle *output) {
	for(int id=0; id<number_of_particles; id++) {
		Particle* this_particle = &output[id];
		
		float force_x = 0.0f, force_y = 0.0f, force_z = 0.0f;
		float total_force_x = 0.0f, total_force_y = 0.0f, total_force_z = 0.0f;
		
		int i;
		for(i = 0; i < number_of_particles; i++) {
			if(i != id) {
				calculate_force(d_particles + id, d_particles + i, &force_x, &force_y, &force_z);
				
				total_force_x += force_x;
				total_force_y += force_y;
				total_force_z += force_z;
			}
		}
	
		float velocity_change_x, velocity_change_y, velocity_change_z;
		float position_change_x, position_change_y, position_change_z;

		this_particle->mass = d_particles[id].mass;
        
		velocity_change_x = total_force_x * (time_interval / this_particle->mass);
		velocity_change_y = total_force_y * (time_interval / this_particle->mass);
		velocity_change_z = total_force_z * (time_interval / this_particle->mass);

		position_change_x = d_particles[id].velocity_x + velocity_change_x * (0.5 * time_interval);
		position_change_y = d_particles[id].velocity_y + velocity_change_y * (0.5 * time_interval);
		position_change_z = d_particles[id].velocity_z + velocity_change_z * (0.5 * time_interval);

		this_particle->velocity_x = d_particles[id].velocity_x + velocity_change_x;
		this_particle->velocity_y = d_particles[id].velocity_y + velocity_change_y;
		this_particle->velocity_z = d_particles[id].velocity_z + velocity_change_z;

		this_particle->position_x = d_particles[id].position_x + position_change_x;
		this_particle->position_y = d_particles[id].position_y + position_change_y;
		this_particle->position_z = d_particles[id].position_z + position_change_z;
	}
}

void compute_serial(Particle* serial_particles, Particle* result_particles, double time_parallel) {
	
	Particle* serial_particles2 = Particle_array_construct(number_of_particles);
	
	//Calculate nbody
	std::cout << "Calculating NBody serial...\n";
	
	long start = wtime();
	
	for(int timestep = 1; timestep <= number_of_timesteps; timestep++) {
		if((timestep % timesteps_between_outputs) == 0) 
			std::cout << "SERIAL - Starting timestep #" << timestep << std::endl;
		
		serial_nbody(serial_particles, serial_particles2);
		
		/* swap arrays */
		Particle * tmp = serial_particles;
		serial_particles = serial_particles2;
		serial_particles2 = tmp;
	}
	
	long end = wtime();
	
	double time = (end - start) / 1000000.0;
	printf("SERIAL - Time in seconds: %g s.\n", time);
	printf("SERIAL - Particles per second: %g \n", (number_of_particles*number_of_timesteps)/time);
	printf("Speedup: %g \n", time/time_parallel);
	
	//Compare results
	std::cout << "Verifying...\n";
	bool ok = true;
	for(int i=0; i<number_of_particles; i++) {
		if(serial_particles[i].position_x != result_particles[i].position_x ||
		   serial_particles[i].position_y != result_particles[i].position_y ||
		   serial_particles[i].position_z != result_particles[i].position_z) {
			//printf("%5d Expected: %g %g %g | Calculated: %g %g %g\n", i,
			printf("%5d Expected: %.2f %.2f %.2f | Calculated: %.2f %.2f %.2f\n", i,
				serial_particles[i].position_x, serial_particles[i].position_y, serial_particles[i].position_z,
				result_particles[i].position_x, result_particles[i].position_y, result_particles[i].position_z);
			ok = false;
		}
	}
	
	if(ok)
		std::cout << "Verification OK\n";
}