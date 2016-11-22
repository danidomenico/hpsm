#include "functors.hpp"
#include "serial.hpp"

void copy(float* dst, float* src, int N, int block_size) {
	hpsm::View<float> v_dst(dst, N, block_size, hpsm::AccessMode::Out); 
	hpsm::View<float> v_src(src, N, block_size, hpsm::AccessMode::In);
	hpsm::range<1> rg(N);
	
	funcCopy func(v_dst, v_src);
	hpsm::parallel_for(rg, v_dst.block_range(), func);
	
	func.remove_data();
}

void dump(float* variables, int nelr) {
	std::ofstream file1("density");
	file1 << nelr << std::endl;
	for(int i = 0; i < nelr; i++) 
		file1 << variables[i*NVAR + VAR_DENSITY] << std::endl;
		//file1 << variables[i + VAR_DENSITY*nelr] << std::endl;

	std::ofstream file2("momentum");
	file2 << nelr << " " << nelr << std::endl;
	for(int i = 0; i < nelr; i++) {
		for(int j = 0; j != NDIM; j++) 
			file2 << variables[i*NVAR + (VAR_MOMENTUM+j)] << " ";
			//file2 << variables[i + (VAR_MOMENTUM+j)*nelr] << " ";
		file2 << std::endl;
	}

	std::ofstream file3("density_energy");
	file3 << nelr << " " << nelr << std::endl;
	for(int i = 0; i < nelr; i++) 
		file3 << variables[i*NVAR + VAR_DENSITY_ENERGY] << std::endl;
		//file3 << variables[i + VAR_DENSITY_ENERGY*nelr] << std::endl;
}

void compute_step_factor(int nelr, int block_size, float* variables, float* areas, float* step_factors) {
	hpsm::View<float> v_variables(variables, nelr*NVAR, block_size*NVAR, hpsm::AccessMode::In); 
	hpsm::View<float> v_areas(areas, nelr, block_size, hpsm::AccessMode::In);
	hpsm::View<float> v_step_factors(step_factors, nelr, block_size, hpsm::AccessMode::Out);
	hpsm::range<1> rg(nelr);
	
	funcStepFactor func(v_variables, v_areas, v_step_factors);
	hpsm::parallel_for(rg, v_step_factors.block_range(), func);
	
	func.remove_data();
}

void compute_flux(int nelr, int block_size, 
				  int* elements_surrounding_elements, float* normals, float* variables, float* fluxes, float* step_factors, 
				  variable ff_variable, 
				  float_3 ff_flux_contribution_momentum_x, float_3 ff_flux_contribution_momentum_y, float_3 ff_flux_contribution_momentum_z, 
				  float_3 ff_flux_contribution_density_energy) {
	//const float smoothing_coefficient = float(0.2f);
	
	hpsm::View<int> v_ele_sur_ele(elements_surrounding_elements, nelr*NNB, block_size*NNB, hpsm::AccessMode::In);
	hpsm::View<float> v_normals(normals, nelr*NNB*NDIM, block_size*NNB*NDIM, hpsm::AccessMode::In);
	hpsm::View<float> v_variables(variables, nelr*NVAR, nelr*NVAR, hpsm::AccessMode::In); //Do not particionate
	hpsm::View<float> v_fluxes(fluxes, nelr*NVAR, block_size*NVAR, hpsm::AccessMode::Out);
	hpsm::View<float> v_step_factors(step_factors, nelr, block_size, hpsm::AccessMode::In);
	hpsm::range<1> rg(nelr);
	
	funcComputeFlux func(v_ele_sur_ele, v_normals, v_variables,
						 v_fluxes, v_step_factors,
						 ff_variable,
						 ff_flux_contribution_momentum_x, ff_flux_contribution_momentum_y, ff_flux_contribution_momentum_z,
						 ff_flux_contribution_density_energy);
	hpsm::parallel_for(rg, v_step_factors.block_range(), func);
	
	func.remove_data();
}

void time_step(int j, int nelr, int block_size, float* old_variables, float* variables, float* step_factors, float* fluxes) {
	hpsm::View<float> v_old_variables(old_variables, nelr*NVAR, block_size*NVAR, hpsm::AccessMode::In); 
	hpsm::View<float> v_variables(variables, nelr*NVAR, block_size*NVAR, hpsm::AccessMode::Out); 
	hpsm::View<float> v_step_factors(step_factors, nelr, block_size, hpsm::AccessMode::In);
	hpsm::View<float> v_fluxes(fluxes, nelr*NVAR, block_size*NVAR, hpsm::AccessMode::In);
	hpsm::range<1> rg(nelr);
	
	funcTimeStep func(v_old_variables, v_variables, v_step_factors, v_fluxes, j);
	hpsm::parallel_for(rg, v_step_factors.block_range(), func);
	
	func.remove_data();
}

/*
 * Main function
 */
int main(int argc, char** argv) {
	
	int size, block_size, iterations;
	bool serial = false;
	
	/* check validity of inputs */
	if ( argc != 6 ||
		(size = atoi(argv[2])) <= 0 ||
		(block_size = atoi(argv[3])) <= 0 ||
		(iterations = atoi(argv[4])) <= 0 ) {
		std::cout << "Usage: ./euler3d <input_file> <size> <block_size> <iterations> <serial>\n";
		return 0;
	}
	
	const char* data_file_name = argv[1];

#ifdef VERBOSE
	int aux = atoi(argv[5]);
	serial = aux != 0 ? true : false;
#endif
	
	variable ff_variable;
	float_3 ff_flux_contribution_momentum_x, ff_flux_contribution_momentum_y, ff_flux_contribution_momentum_z;
	float_3 ff_flux_contribution_density_energy;

	// set far field conditions
	const float angle_of_attack = float(3.1415926535897931 / 180.0f) * float(DEG_ANGLE_OF_ATTACK);

	ff_variable.density = float(1.4);

	float ff_pressure = float(1.0f);
	float ff_speed_of_sound = sqrt(GAMMA*ff_pressure / ff_variable.density);
	float ff_speed = float(FF_MACH)*ff_speed_of_sound;

	float_3 ff_velocity;
	ff_velocity.x = ff_speed*float(cos((float)angle_of_attack));
	ff_velocity.y = ff_speed*float(sin((float)angle_of_attack));
	ff_velocity.z = 0.0f;

	ff_variable.momentum_0 = ff_variable.density * ff_velocity.x;
	ff_variable.momentum_1 = ff_variable.density * ff_velocity.y;
	ff_variable.momentum_2 = ff_variable.density * ff_velocity.z;

	ff_variable.density_energy = ff_variable.density*(float(0.5f)*(ff_speed*ff_speed)) + (ff_pressure / float(GAMMA-1.0f));

	float_3 ff_momentum;
	ff_momentum.x = ff_variable.momentum_0; //*(ff_variable+VAR_MOMENTUM+0);
	ff_momentum.y = ff_variable.momentum_1; //*(ff_variable+VAR_MOMENTUM+1);
	ff_momentum.z = ff_variable.momentum_2; //*(ff_variable+VAR_MOMENTUM+2);
	compute_flux_contribution(ff_variable.density, ff_momentum, ff_variable.density_energy, 
							  ff_pressure, ff_velocity, 
							  &ff_flux_contribution_momentum_x, &ff_flux_contribution_momentum_y, &ff_flux_contribution_momentum_z, 
							  &ff_flux_contribution_density_energy);
	
	int nelr = size;

	float* areas                       = alloc<float>(nelr);         //new float[nelr];
	int* elements_surrounding_elements = alloc<int>(nelr * NNB);     // new int[nelr*NNB];
	float* normals                     = alloc<float>(nelr * NNB * NDIM); // new float[NDIM*NNB*nelr];

#ifdef VERBOSE
	std::cout << "Reading input file..." << std::endl;
#endif
	
	read_input_file(data_file_name, size, 
					areas, elements_surrounding_elements, normals);

#ifdef VERBOSE
	std::cout << "Initializing..." << std::endl;
#endif

	// Create arrays and set initial conditions
	float* variables = alloc<float>(nelr * NVAR);
	initialize_variables(nelr, variables, ff_variable);

	float* old_variables = alloc<float>(nelr * NVAR);
	float* fluxes        = alloc<float>(nelr * NVAR);
	float* step_factors  = alloc<float>(nelr);

	hpsm::initialize();
	long start = get_time();
	
	// these need to be computed the first time in order to compute time step
#ifdef VERBOSE
	std::cout << "Starting iterations..." << std::endl;
#endif
	
	// Begin iterations
	for(int i = 0; i < iterations; i++) {
		copy(old_variables, variables, nelr * NVAR, block_size * NVAR);

		// for the first iteration we compute the time step
		compute_step_factor(nelr, block_size, variables, areas, step_factors);

		for(int j = 0; j < RK; j++) {
			compute_flux(nelr, block_size,
						 elements_surrounding_elements, normals, variables, fluxes, step_factors, 
						 ff_variable, 
						 ff_flux_contribution_momentum_x, ff_flux_contribution_momentum_y, ff_flux_contribution_momentum_z, 
						 ff_flux_contribution_density_energy);
			time_step(j, nelr, block_size, old_variables, variables, step_factors, fluxes);
		}
	}

	long end = get_time();
	hpsm::finalize();
	
	double time = (end - start) / 1000000.0;
	
#ifdef VERBOSE
	std::cout  << "Compute time: " << time << std::endl;

	std::cout << "Saving solution..." << std::endl;
	dump(variables, nelr);
	std::cout << "Saved solution..." << std::endl;
#else
	printf("cdf_solver;%s;%d;%d;%d;%d;%d;%.8f\n", 
		parallel_Backend_Str[BACKEND_OPT], nelr, block_size, iterations, hpsm::num_workers_cpu(), hpsm::num_workers_gpu(), time);
#endif
	
	if(serial)
		run_serial_and_verify(data_file_name, nelr, iterations, variables, time);

#ifdef VERBOSE
	std::cout << "Cleaning up..." << std::endl;
#endif
	
	dealloc<float>(areas);
	dealloc<int>(elements_surrounding_elements);
	dealloc<float>(normals);

	dealloc<float>(variables);
	dealloc<float>(old_variables);
	dealloc<float>(fluxes);
	dealloc<float>(step_factors);

#ifdef VERBOSE
	std::cout << "Done..." << std::endl;
#endif

	return 0;
}