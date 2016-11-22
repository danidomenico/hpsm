#include "functor_nbody.hpp"

/* main */
int main (int argc, char** argv) { 
	if(argc < 2) {
		std::cout << "Usage: ./nbody <input_file>\n";
		std::abort();
	}
	
	Particle* particle_array  = nullptr;
	Particle* particle_array2 = nullptr;
	
	Particle* particle_array_serial = nullptr;
	
	FILE *input_data = fopen(argv[1], "r");
	Particle_input_arguments(input_data);

	particle_array  = Particle_array_construct(number_of_particles);
	particle_array2 = Particle_array_construct(number_of_particles);
	Particle_array_initialize(particle_array, number_of_particles);
	
	if(execute_serial) {
		particle_array_serial = Particle_array_construct(number_of_particles);
		for(int i=0; i<number_of_particles; i++)
			particle_array_serial[i] = particle_array[i];
	}

#ifdef VERBOSE
	//FILE * fileptr = fopen("nbody_out.xyz", "w");
	//Particle_array_output_xyz(fileptr, particle_array, number_of_particles);

	std::cout << "Backend: " << parallel_Backend_Str[BACKEND_OPT] << std::endl;
#endif
	
	hpsm::initialize();
	hpsm::range<1> rg(number_of_particles);
	
	long start = wtime();
	
	for(int timestep = 1; timestep <= number_of_timesteps; timestep++) {
#ifdef VERBOSE
		if((timestep % timesteps_between_outputs) == 0) 
			std::cout << "Starting timestep #" << timestep << std::endl;
#endif
		
		hpsm::View<Particle> part_in(particle_array, number_of_particles, number_of_particles, hpsm::AccessMode::In); 
		hpsm::View<Particle> part_out(particle_array2, number_of_particles, block_size, hpsm::AccessMode::Out);
		
		funcNBody func(part_in, part_out, number_of_particles, time_interval);
		hpsm::parallel_for(rg, part_out.block_range(), func);
		
		func.remove_data();
		
		/* Swap arrays */
		Particle * tmp  = particle_array;
		particle_array  = particle_array2; //Results must be in particle_array
		particle_array2 = tmp;
	}

	long end = wtime();
	
	hpsm::finalize();
	
	double time = (end - start) / 1000000.0;

#ifdef VERBOSE
	printf("Time in seconds: %g s.\n", time);
	printf("Particles per second: %g \n", (number_of_particles*number_of_timesteps)/time);
#else
	//printf("BENCH=NBody;backend=%s;size=%d;block_size=%d;iterations=%d;threads=%d;gpus=%d;time=%.8f\n",
	printf("nbody;%s;%d;%d;%d;%d;%d;%.8f\n", 
		parallel_Backend_Str[BACKEND_OPT], number_of_particles, block_size, number_of_timesteps, hpsm::num_workers_cpu(), hpsm::num_workers_gpu(), time);
#endif
	
	if(execute_serial)
		compute_serial(particle_array_serial, particle_array, time);

#ifdef VERBOSE
	//Particle_array_output_xyz(fileptr, particle_array, number_of_particles);
#endif

	particle_array  = Particle_array_destruct(particle_array, number_of_particles);
	particle_array2 = Particle_array_destruct(particle_array2, number_of_particles);
	if(execute_serial)
		particle_array_serial = Particle_array_destruct(particle_array_serial, number_of_particles);

#ifdef VERBOSE
	/*
	if(fclose(fileptr) != 0) {
		std::cout << "ERROR: can't close the output file.\n";
		std::abort();
	}*/
#endif

	return PROGRAM_SUCCESS_CODE;
}