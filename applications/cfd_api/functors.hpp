#pragma once

#include "euler3d.hpp"

struct funcCopy : hpsm::Functor {
	hpsm::View<float> dest;
	hpsm::View<float> src;
	
	funcCopy(hpsm::View<float> _dest, hpsm::View<float> _src) : 
		dest(_dest), src(_src) {
		register_data(dest, src);
    }
    
	funcCopy(const funcCopy& other) : hpsm::Functor(other), dest(other.dest), src(other.src) {
		clear_data();
		register_data(dest, src);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		dest(idx) = src(idx);
	};
};

struct funcStepFactor : hpsm::Functor {
	hpsm::View<float> variables;
	hpsm::View<float> areas;
	hpsm::View<float> step_factors;
	
	funcStepFactor(hpsm::View<float> _variables, hpsm::View<float> _areas, hpsm::View<float> _step_factors) : 
		variables(_variables), areas(_areas), step_factors(_step_factors) {
		register_data(variables, areas, step_factors);
    }
    
	funcStepFactor(const funcStepFactor& other) : hpsm::Functor(other), 
		variables(other.variables), areas(other.areas), step_factors(other.step_factors) {
		clear_data();
		register_data(variables, areas, step_factors);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		for(int x=0; x < VOLUME_REP; x++) {
			float density = variables(idx(0)*NVAR + VAR_DENSITY); //[i + VAR_DENSITY*nelr];

			float_3 momentum;
			momentum.x = variables(idx(0)*NVAR + (VAR_MOMENTUM+0)); //[i + (VAR_MOMENTUM+0)*nelr];
			momentum.y = variables(idx(0)*NVAR + (VAR_MOMENTUM+1)); //[i + (VAR_MOMENTUM+1)*nelr];
			momentum.z = variables(idx(0)*NVAR + (VAR_MOMENTUM+2)); //[i + (VAR_MOMENTUM+2)*nelr];

			float density_energy = variables(idx(0)*NVAR + VAR_DENSITY_ENERGY); //[i + VAR_DENSITY_ENERGY*nelr];
			float_3 velocity;
			compute_velocity(density, momentum, &velocity);
			float speed_sqd      = compute_speed_sqd(velocity);
			float pressure       = compute_pressure(density, density_energy, speed_sqd);
			float speed_of_sound = compute_speed_of_sound(density, pressure);

			// dt = float(0.5f) * std::sqrt(areas[i]) /  (||v|| + c).... but when we do time stepping, this later would need to be divided by the area, so we just do it all at once
			step_factors(idx) = float(0.5f) / (std::sqrt(areas(idx)) * (std::sqrt(speed_sqd) + speed_of_sound));
		}
	};
};

struct funcComputeFlux : hpsm::Functor {
	hpsm::View<int> elements_surrounding_elements;
	hpsm::View<float> normals;
	hpsm::View<float> variables;
	hpsm::View<float> fluxes;
	hpsm::View<float> step_factors; //Apenas para controlar o laço
	
	variable ff_variable;
	float_3 ff_flux_contribution_momentum_x; 
	float_3 ff_flux_contribution_momentum_y;
	float_3 ff_flux_contribution_momentum_z;
	float_3 ff_flux_contribution_density_energy;
	
	const float smoothing_coefficient = float(0.2f);
	
	funcComputeFlux(hpsm::View<int> _elements_surrounding_elements, hpsm::View<float> _normals,
				hpsm::View<float> _variables, hpsm::View<float> _fluxes, 
				hpsm::View<float> _step_factors, 
				variable _ff_variable, 
				float_3 _ff_flux_contribution_momentum_x,  float_3 _ff_flux_contribution_momentum_y, float_3 _ff_flux_contribution_momentum_z,
				float_3 _ff_flux_contribution_density_energy) : 
		elements_surrounding_elements(_elements_surrounding_elements), normals(_normals),
		variables(_variables), fluxes(_fluxes), step_factors(_step_factors),
		ff_variable(_ff_variable),
		ff_flux_contribution_momentum_x(_ff_flux_contribution_momentum_x), 
		ff_flux_contribution_momentum_y(_ff_flux_contribution_momentum_y),
		ff_flux_contribution_momentum_z(_ff_flux_contribution_momentum_z),
		ff_flux_contribution_density_energy(_ff_flux_contribution_density_energy) {
		register_data(elements_surrounding_elements, normals, variables, fluxes, step_factors);
    }
    
	funcComputeFlux(const funcComputeFlux& o) : hpsm::Functor(o), 
		elements_surrounding_elements(o.elements_surrounding_elements), normals(o.normals),
		variables(o.variables), fluxes(o.fluxes), step_factors(o.step_factors),
		ff_variable(o.ff_variable),
		ff_flux_contribution_momentum_x(o.ff_flux_contribution_momentum_x), 
		ff_flux_contribution_momentum_y(o.ff_flux_contribution_momentum_y),
		ff_flux_contribution_momentum_z(o.ff_flux_contribution_momentum_z),
		ff_flux_contribution_density_energy(o.ff_flux_contribution_density_energy) {
		clear_data();
		register_data(elements_surrounding_elements, normals, variables, fluxes, step_factors);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		unsigned idx_real = (idx.block() * step_factors.size()) + idx(0); 
		unsigned idx_var = idx_real * NVAR;
		
		for(int x=0; x < VOLUME_REP; x++) {
		
			float density_i = variables(idx_var + VAR_DENSITY); //[i + VAR_DENSITY*nelr];
			float_3 momentum_i;
			momentum_i.x = variables(idx_var + (VAR_MOMENTUM+0)); //[i + (VAR_MOMENTUM+0)*nelr];
			momentum_i.y = variables(idx_var + (VAR_MOMENTUM+1)); //[i + (VAR_MOMENTUM+1)*nelr];
			momentum_i.z = variables(idx_var + (VAR_MOMENTUM+2)); //[i + (VAR_MOMENTUM+2)*nelr];

			float density_energy_i = variables(idx_var + VAR_DENSITY_ENERGY); //[i + VAR_DENSITY_ENERGY*nelr];

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

				int nb = elements_surrounding_elements(idx(0)*NNB + j); //[i + j*nelr];
				
				normal.x = normals(idx(0)*NNB*NDIM + j*NDIM + 0); //[i + (j + 0*NNB)*nelr]; 
				normal.y = normals(idx(0)*NNB*NDIM + j*NDIM + 1); //[i + (j + 1*NNB)*nelr];
				normal.z = normals(idx(0)*NNB*NDIM + j*NDIM + 2); //[i + (j + 2*NNB)*nelr];
				normal_len = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);

				if(nb >= 0) {// a legitimate neighbor
					density_nb        = variables(nb*NVAR + VAR_DENSITY);        //[nb + VAR_DENSITY*nelr];
					momentum_nb.x     = variables(nb*NVAR + (VAR_MOMENTUM+0));   //[nb + (VAR_MOMENTUM+0)*nelr];
					momentum_nb.y     = variables(nb*NVAR + (VAR_MOMENTUM+1));   //[nb + (VAR_MOMENTUM+1)*nelr];
					momentum_nb.z     = variables(nb*NVAR + (VAR_MOMENTUM+2));   //[nb + (VAR_MOMENTUM+2)*nelr];
					density_energy_nb = variables(nb*NVAR + VAR_DENSITY_ENERGY); //[nb + VAR_DENSITY_ENERGY*nelr];
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
					flux_i_density        += factor * (ff_variable.momentum_0 + momentum_i.x);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.x + flux_contribution_i_density_energy.x);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.x + flux_contribution_i_momentum_x.x);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.x + flux_contribution_i_momentum_y.x);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.x + flux_contribution_i_momentum_z.x);

					factor                 = float(0.5f) * normal.y;
					flux_i_density        += factor * (ff_variable.momentum_1 + momentum_i.y);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.y + flux_contribution_i_density_energy.y);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.y + flux_contribution_i_momentum_x.y);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.y + flux_contribution_i_momentum_y.y);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.y + flux_contribution_i_momentum_z.y);

					factor                 = float(0.5f) * normal.z;
					flux_i_density        += factor * (ff_variable.momentum_2 + momentum_i.z);
					flux_i_density_energy += factor * (ff_flux_contribution_density_energy.z + flux_contribution_i_density_energy.z);
					flux_i_momentum.x     += factor * (ff_flux_contribution_momentum_x.z + flux_contribution_i_momentum_x.z);
					flux_i_momentum.y     += factor * (ff_flux_contribution_momentum_y.z + flux_contribution_i_momentum_y.z);
					flux_i_momentum.z     += factor * (ff_flux_contribution_momentum_z.z + flux_contribution_i_momentum_z.z);
				}
			}
			
			fluxes(idx(0)*NVAR + VAR_DENSITY)        = flux_i_density;
			fluxes(idx(0)*NVAR + (VAR_MOMENTUM+0))   = flux_i_momentum.x;
			fluxes(idx(0)*NVAR + (VAR_MOMENTUM+1))   = flux_i_momentum.y;
			fluxes(idx(0)*NVAR + (VAR_MOMENTUM+2))   = flux_i_momentum.z;
			fluxes(idx(0)*NVAR + VAR_DENSITY_ENERGY) = flux_i_density_energy;
			/*
			fluxes[i + VAR_DENSITY*nelr]        = flux_i_density;
			fluxes[i + (VAR_MOMENTUM+0)*nelr]   = flux_i_momentum.x;
			fluxes[i + (VAR_MOMENTUM+1)*nelr]   = flux_i_momentum.y;
			fluxes[i + (VAR_MOMENTUM+2)*nelr]   = flux_i_momentum.z;
			fluxes[i + VAR_DENSITY_ENERGY*nelr] = flux_i_density_energy;
			*/
		} //VOLUME_REP
	};
};

struct funcTimeStep : hpsm::Functor {
	hpsm::View<float> old_variables;
	hpsm::View<float> variables;
	hpsm::View<float> step_factors;
	hpsm::View<float> fluxes;
	int j;
	
	funcTimeStep(hpsm::View<float> _old_variables, hpsm::View<float> _variables, 
				 hpsm::View<float> _step_factors, hpsm::View<float> _fluxes, int _j) : 
		old_variables(_old_variables), variables(_variables), step_factors(_step_factors), fluxes(_fluxes), j(_j) {
		register_data(old_variables, variables, step_factors, fluxes);
    }
    
	funcTimeStep(const funcTimeStep& other) : hpsm::Functor(other), 
		old_variables(other.old_variables), variables(other.variables), 
		step_factors(other.step_factors), fluxes(other.fluxes), j(other.j) {
		clear_data();
		register_data(old_variables, variables, step_factors, fluxes);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<1> idx) {
		for(int x=0; x < VOLUME_REP; x++) {
			float factor = step_factors(idx) / float(RK + 1 - j);
			unsigned idx_var = idx(0) * NVAR;

			variables(idx_var + VAR_DENSITY)        = old_variables(idx_var + VAR_DENSITY)        + factor * fluxes(idx_var + VAR_DENSITY);
			variables(idx_var + (VAR_MOMENTUM+0))   = old_variables(idx_var + (VAR_MOMENTUM+0))   + factor * fluxes(idx_var + (VAR_MOMENTUM+0));
			variables(idx_var + (VAR_MOMENTUM+1))   = old_variables(idx_var + (VAR_MOMENTUM+1))   + factor * fluxes(idx_var + (VAR_MOMENTUM+1));
			variables(idx_var + (VAR_MOMENTUM+2))   = old_variables(idx_var + (VAR_MOMENTUM+2))   + factor * fluxes(idx_var + (VAR_MOMENTUM+2));
			variables(idx_var + VAR_DENSITY_ENERGY) = old_variables(idx_var + VAR_DENSITY_ENERGY) + factor * fluxes(idx_var + VAR_DENSITY_ENERGY);
			
			/*
			variables[i + VAR_DENSITY*nelr]        = old_variables[i + VAR_DENSITY*nelr]        + factor * fluxes[i + VAR_DENSITY*nelr];
			variables[i + (VAR_MOMENTUM+0)*nelr]   = old_variables[i + (VAR_MOMENTUM+0)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+0)*nelr];
			variables[i + (VAR_MOMENTUM+1)*nelr]   = old_variables[i + (VAR_MOMENTUM+1)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+1)*nelr];
			variables[i + (VAR_MOMENTUM+2)*nelr]   = old_variables[i + (VAR_MOMENTUM+2)*nelr]   + factor * fluxes[i + (VAR_MOMENTUM+2)*nelr];
			variables[i + VAR_DENSITY_ENERGY*nelr] = old_variables[i + VAR_DENSITY_ENERGY*nelr] + factor * fluxes[i + VAR_DENSITY_ENERGY*nelr];
			*/
		}
	};
};