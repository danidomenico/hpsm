#include "../serial.hpp"
#include "../utils.hpp"

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

/*
 * Main function
 */
int main(int argc, char** argv) {
	
	int nelr, iterations;
	
	/* check validity of inputs */
	if ( argc != 4 ||
		(nelr = atoi(argv[2])) <= 0 ||
		(iterations = atoi(argv[3])) <= 0 ) {
		std::cout << "Usage: ./euler3d <input_file> <size> <iterations>\n";
		return 0;
	}
	
	const char* data_file_name = argv[1];

	// Create arrays and set initial conditions
	float* variables = alloc<float>(nelr * NVAR);
	
	double time = run_serial(data_file_name, nelr, iterations, variables);
	
#ifdef VERBOSE
	std::cout << "Saving solution..." << std::endl;
	dump(variables, nelr);
	std::cout << "Saved solution..." << std::endl;
#else
	printf("cdf_solver;%s;%d;%d;%d;%d;%d;%.8f\n", 
		"SERIAL", nelr, nelr, iterations, 1, 0, time);
#endif

	return 0;
}