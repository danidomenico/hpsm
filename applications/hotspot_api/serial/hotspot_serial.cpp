#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>

using FLOAT = float;

// Returns the current system time in microseconds 
long long get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000) + tv.tv_usec;
}

/* maximum power density possible (say 300W for a 10mm x 10mm chip) */
const FLOAT MAX_PD = 3.0e6;

/* required precision in degrees */
const FLOAT PRECISION    = 0.001;
const FLOAT SPEC_HEAT_SI = 1.75e6;
const int   K_SI         = 100;

/* capacitance fitting factor */
const FLOAT FACTOR_CHIP = 0.5;

/* chip parameters */
const FLOAT T_CHIP      = 0.0005;
const FLOAT CHIP_HEIGHT = 0.016;
const FLOAT CHIP_WIDTH  = 0.016;

/* ambient temperature, assuming no package at all	*/
const FLOAT AMB_TEMP = 80.0;

/* volume repetitions */
const unsigned VOLUME_REP = 60;

/* Single iteration of the transient solver in the grid model.
 * advances the solution of the discretized difference equations 
 * by one time step
 */
void single_iteration(FLOAT *result, FLOAT *temp, FLOAT *power, int row, int col,
					  FLOAT Cap_1, FLOAT Rx_1, FLOAT Ry_1, FLOAT Rz_1) {
	
	for(unsigned r=0; r<row; r++) {
		for(unsigned c=0; c<col; c++) {
			
			for(unsigned i=0; i<VOLUME_REP; i++) {
				
				FLOAT delta;
				unsigned idx = r*col+c;
				
				//Corner 1
				if(r == 0 && c == 0) {
					delta = Cap_1 * 
							( power[idx] +
							(temp[1] - temp[idx]) * Rx_1 +
							(temp[col] - temp[idx]) * Ry_1 +
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Corner 2
				} else if(r == 0 && c == col-1) {
					delta = Cap_1 * 
							( power[idx] +
							(temp[c-1] - temp[idx]) * Rx_1 +
							(temp[c+col] - temp[idx]) * Ry_1 +
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Corner 3
				} else if(r == row-1 && c == col-1) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[idx-1] - temp[idx]) * Rx_1 + 
							(temp[(r-1)*col+c] - temp[idx]) * Ry_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Corner 4
				} else if(r == row-1 && c == 0) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[idx+1] - temp[idx]) * Rx_1 + 
							(temp[(r-1)*col] - temp[idx]) * Ry_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Edge 1
				} else if (r == 0) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[idx+1] + temp[idx-1] - 2.0*temp[idx]) * Rx_1 + 
							(temp[col+c] - temp[idx]) * Ry_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Edge 2
				} else if (c == col-1) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[idx]) * Ry_1 + 
							(temp[idx-1] - temp[idx]) * Rx_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Edge 3 
				} else if (r == row-1) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[idx+1] + temp[idx-1] - 2.0*temp[idx]) * Rx_1 + 
							(temp[(r-1)*col+c] - temp[idx]) * Ry_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				// Edge 4
				} else if (c == 0) {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[(r+1)*col] + temp[(r-1)*col] - 2.0*temp[idx]) * Ry_1 + 
							(temp[idx+1] - temp[idx]) * Rx_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				//Demais
				} else {
					delta = Cap_1 * 
							( power[idx] + 
							(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[idx]) * Ry_1 + 
							(temp[idx+1] + temp[idx-1] - 2.0*temp[idx]) * Rx_1 + 
							(AMB_TEMP - temp[idx]) * Rz_1 );
				}
				
				result[idx] = temp[idx] + delta;
			}
		}
	}
}

/* Transient solver driver routine: simply converts the heat 
 * transfer differential equations to difference equations 
 * and solves the difference equations by iterating
 */
void compute_tran_temp(FLOAT *result, FLOAT *temp, FLOAT *power, 
					   int num_iterations, int order_size) {
	int row = order_size;
	int col = order_size;
	
	FLOAT grid_height = CHIP_HEIGHT / row;
	FLOAT grid_width  = CHIP_WIDTH  / col;

	FLOAT Cap = FACTOR_CHIP * SPEC_HEAT_SI * T_CHIP * grid_width * grid_height;
	FLOAT Rx = grid_width  / (2.0 * K_SI * T_CHIP * grid_height);
	FLOAT Ry = grid_height / (2.0 * K_SI * T_CHIP * grid_width);
	FLOAT Rz = T_CHIP / (K_SI * grid_height * grid_width);

	FLOAT max_slope = MAX_PD / (FACTOR_CHIP * T_CHIP * SPEC_HEAT_SI);
	FLOAT step      = PRECISION / max_slope / 1000.0;

	FLOAT Rx_1 = 1.f/Rx;
	FLOAT Ry_1 = 1.f/Ry;
	FLOAT Rz_1 = 1.f/Rz;
	FLOAT Cap_1 = step/Cap;
	
	/* Single iteration of the transient solver in the grid model.
	 * advances the solution of the discretized difference equations 
     * by one time step
    */
	for(int i = 0; i < num_iterations ; i++) {
		single_iteration(result, temp, power, row, col, Cap_1, Rx_1, Ry_1, Rz_1);
	
		/* Swap arrays */
		FLOAT* tmp = temp;
		temp       = result;
		result     = tmp;
	}
	
	result = temp;
}

void fatal(std::string s) {
	std::cerr << "Hotspot error: " << s << std::endl;
	std::abort();
}

void generate_input(FLOAT* temp, FLOAT* power, FLOAT* result, int order_size) {
	const FLOAT MIN = 323.0;
	const FLOAT MAX = 341.0;
	FLOAT value = MIN;
	bool  asc = true;
	
	for(unsigned i=0; i<order_size * order_size; i++) {
		//POWER
		FLOAT x = (rand() % 1000000) / 1000000.0;
		power[i] = x;
		
		//TEMP
		x = (rand() % 1000000) / 1000000.0;
		if(asc) {
			if((value + x) > MAX) {
				value -= x;
				asc   = false;
			} else
				value += x;
		} else {
			if((value - x) < MIN) {
				value += x;
				asc   = true;
			} else
				value -= x;
		}
		temp[i] = value;
		
		//RESULT
		result[i] = 0.0;
	}
}

void usage(int argc, char **argv) {
	std::cerr << "Usage:" << argv[0] << "<order> <iterations>\n";
	std::cerr << "\t<order>      - order for the grid - size= <order>X<order> (positive integer)\n";
	std::cerr << "\t<iterations> - number of iterations\n";
	std::abort();
}

int main(int argc, char **argv) {
	int order_size, iterations;
	FLOAT *temp, *power, *result;
	
	/* check validity of inputs*/
	if(argc != 3)
		usage(argc, argv);
	if( (order_size = atoi(argv[1])) <= 0 ||
		(iterations = atoi(argv[2])) <= 0)  
		usage(argc, argv);

	/* allocate memory for the temperature and power arrays	*/
	temp   = new FLOAT[order_size * order_size];
	power  = new FLOAT[order_size * order_size];
	result = new FLOAT[order_size * order_size];
	if(!temp || !power || ! result)
		fatal("unable to allocate memory");
	
	/* generate input */
	generate_input(temp, power, result, order_size);
	
#ifdef VERBOSE
	printf("Start computing the transient temperature\n");
#endif
	
	long long start_time = get_time();
	compute_tran_temp(result, temp, power, iterations, order_size);
	long long end_time = get_time();
	
	float total_time = ((float) (end_time - start_time)) / (1000*1000);
	
	//printf("BENCH=hotspot;backend=%s;size=%d;block_size=%d;iterations=%d;threads=%d;gpus=%d;time=%.8f\n", 
	printf("hotspot;%s;%d;%d;%d;%d;%d;%.8f\n",
		"SERIAL", order_size, order_size, iterations, 1, 0, total_time);
	
	delete[] temp;
	delete[] power;
	delete[] result;
	
	return 0;
}
