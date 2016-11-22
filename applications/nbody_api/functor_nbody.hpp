#pragma once

#include "nbody.hpp"
#include "hpsm.hpp"

#include <cmath>

/*
 * Functions prototypes
 */
PARALLEL_FUNCTION
void calculate_force(Particle* this_particle1, Particle* this_particle2,
					 float* force_x, float* force_y, float* force_z);

void serial_nbody(Particle* d_particles, Particle *output);

void compute_serial(Particle* serial_particles, Particle* result_particles, double time_parallel);

struct funcNBody : hpsm::Functor {
	hpsm::View<Particle> part_in;
	hpsm::View<Particle> part_out;
	
	int   local_number_of_particles;
	float local_time_interval;
	
	
	funcNBody(hpsm::View<Particle> _part_in, hpsm::View<Particle> _part_out,
			  int _local_number_of_particles, float _local_time_interval) : 
		part_in(_part_in), part_out(_part_out), local_number_of_particles(_local_number_of_particles), 
		local_time_interval(_local_time_interval) {
		register_data(part_in, part_out);
    }
    
	funcNBody(const funcNBody& other) : hpsm::Functor(other), part_in(other.part_in), part_out(other.part_out),
		local_number_of_particles(other.local_number_of_particles), local_time_interval(other.local_time_interval) {
		clear_data();
		register_data(part_in, part_out);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		unsigned idx_real = (idx.block() * part_out.size()) + idx(0); 
		
		float force_x = 0.0f, force_y = 0.0f, force_z = 0.0f;
		float total_force_x = 0.0f, total_force_y = 0.0f, total_force_z = 0.0f;
		
		for(unsigned i = 0; i < local_number_of_particles; i++) {
			if(i != idx_real) {
				
				calculate_force(&(part_in(idx_real)), &(part_in(i)), &force_x, &force_y, &force_z);
				
				total_force_x += force_x;
				total_force_y += force_y;
				total_force_z += force_z;
			}
		}
	
		float velocity_change_x, velocity_change_y, velocity_change_z;
		float position_change_x, position_change_y, position_change_z;

		part_out(idx).mass = part_in(idx_real).mass;
        
		velocity_change_x = total_force_x * (local_time_interval / part_out(idx).mass);
		velocity_change_y = total_force_y * (local_time_interval / part_out(idx).mass);
		velocity_change_z = total_force_z * (local_time_interval / part_out(idx).mass);

		position_change_x = part_in(idx_real).velocity_x + velocity_change_x * (0.5 * local_time_interval);
		position_change_y = part_in(idx_real).velocity_y + velocity_change_y * (0.5 * local_time_interval);
		position_change_z = part_in(idx_real).velocity_z + velocity_change_z * (0.5 * local_time_interval);

		part_out(idx).velocity_x = part_in(idx_real).velocity_x + velocity_change_x;
		part_out(idx).velocity_y = part_in(idx_real).velocity_y + velocity_change_y;
		part_out(idx).velocity_z = part_in(idx_real).velocity_z + velocity_change_z;

		part_out(idx).position_x = part_in(idx_real).position_x + position_change_x;
		part_out(idx).position_y = part_in(idx_real).position_y + position_change_y;
		part_out(idx).position_z = part_in(idx_real).position_z + position_change_z;
	};
};