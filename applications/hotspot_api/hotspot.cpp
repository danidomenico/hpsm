#include "hpsm.hpp"

#include <cstdio>
//#include <cstdlib>
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

struct funcHotspot : hpsm::Functor {
	hpsm::View<FLOAT> result;
	hpsm::View<FLOAT> temp;
	hpsm::View<FLOAT> power;
	
	FLOAT Cap_1;
	FLOAT Rx_1;
	FLOAT Ry_1;
	FLOAT Rz_1;
	
	funcHotspot(hpsm::View<FLOAT> _result, hpsm::View<FLOAT> _temp, hpsm::View<FLOAT> _power,
		FLOAT _Cap_1, FLOAT _Rx_1, FLOAT _Ry_1, FLOAT _Rz_1) : 
		result(_result), temp(_temp), power(_power), Cap_1(_Cap_1), Rx_1(_Rx_1), Ry_1(_Ry_1), Rz_1(_Rz_1) {
		register_data(result, temp, power);
    }
    
	funcHotspot(const funcHotspot& other) : hpsm::Functor(other), result(other.result), 
		temp(other.temp), power(other.power), Cap_1(other.Cap_1), Rx_1(other.Rx_1), Ry_1(other.Ry_1), Rz_1(other.Rz_1) {
		clear_data();
		register_data(result, temp, power);
	}
	
	PARALLEL_FUNCTION 
	void operator()(hpsm::index<2> idx) {
		for(unsigned i=0; i<VOLUME_REP; i++) {
			FLOAT delta;
			
			unsigned qtd_block_row = idx.block_qtd_dim(0);
			unsigned qtd_block_col = idx.block_qtd_dim(1);
			
			unsigned r = idx(0);
			unsigned c = idx(1);
			unsigned r_real = (idx.block_dim(0) * idx.size(0)) + r;
			unsigned c_real = (idx.block_dim(1) * idx.size(1)) + c;
			
			/* Corner 1 - Canto superior esquerdo */
			if(idx.block() == 0 && r == 0 && c == 0) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real+1) - temp(r_real, c_real)) * Rx_1 +
						(temp(r_real+1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1 );
			
			/* Corner 2 - Canto superior direito */
			} else if(idx.block_dim(0) == 0 && idx.block_dim(1) == (qtd_block_col-1) &&
					r == 0 && c == (idx.size(1)-1)) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real-1) - temp(r_real, c_real)) * Rx_1 +
						(temp(r_real+1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(c)) * Rz_1 );
			
			/* Corner 3 - Canto inferior direito */  
			} else if(idx.block_dim(0) == (qtd_block_row-1) && idx.block_dim(1) == (qtd_block_col-1) &&
					r == (idx.size(0)-1) && c == (idx.size(1)-1)) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real-1) - temp(r_real, c_real)) * Rx_1 +
						(temp(r_real-1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1 );
			
			/* Corner 4 - Canto inferior esquerdo */  
			} else if(idx.block_dim(0) == (qtd_block_row-1) && idx.block_dim(1) == 0 &&
					r == (idx.size(0)-1) && c == 0) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real+1) - temp(r_real, c_real)) * Rx_1 +
						(temp(r_real-1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1 );
			/* Edge 1 - Linha superior */
			} else if(idx.block_dim(0) == 0 && r == 0) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real+1) + temp(r_real, c_real-1) - 2.0*temp(r_real, c_real)) * Rx_1 +
						(temp(r_real+1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1 );
			/* Edge 2 - Linha direita */
			} else if(idx.block_dim(1) == (qtd_block_col-1) && c == (idx.size(1)-1)) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real+1, c_real) + temp(r_real-1, c_real) - 2.0*temp(r_real, c_real)) * Rx_1 +
						(temp(r_real, c_real-1) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1 );
			/* Edge 3 - Linha inferior */
			} else if(idx.block_dim(0) == (qtd_block_row-1) && r == (idx.size(0)-1)) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real, c_real+1) + temp(r_real, c_real-1) - 2.0*temp(r_real, c_real)) * Rx_1 +
						(temp(r_real-1, c_real) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1);
			/* Edge 4 - Linha esquerda*/
			} else if(idx.block_dim(1) == 0 && c == 0) {
				delta = Cap_1 * 
						( power(idx) +
						(temp(r_real+1, c_real) + temp(r_real-1, c_real) - 2.0*temp(r_real, c_real)) * Rx_1 +
						(temp(r_real, c_real+1) - temp(r_real, c_real)) * Ry_1 +
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1);
			/* Demais */
			} else {
				delta = Cap_1 * 
						( power(idx) + 
						(temp(r_real+1, c_real) + temp(r_real-1, c_real) - 2.0*temp(r_real, c_real)) * Ry_1 + 
						(temp(r_real, c_real+1) + temp(r_real, c_real-1) - 2.0*temp(r_real, c_real)) * Rx_1 + 
						(AMB_TEMP - temp(r_real, c_real)) * Rz_1);
			}
			
			result(idx) = temp(r_real, c_real) + delta;
		}
	}
};

/* Transient solver driver routine: simply converts the heat 
 * transfer differential equations to difference equations 
 * and solves the difference equations by iterating
 */
void compute_tran_temp(FLOAT *result, FLOAT *temp, FLOAT *power, 
					   int num_iterations, int bl_size, int order_size,
					   bool serial = false) {
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
	if(serial) {
		for(int i = 0; i < num_iterations ; i++) {
			single_iteration(result, temp, power, row, col, Cap_1, Rx_1, Ry_1, Rz_1);
		
			/* Swap arrays */
			FLOAT* tmp = temp;
			temp       = result;
			result     = tmp;
		}
	} else {
		hpsm::range<2> rg(row, col);
		for(int i = 0; i < num_iterations ; i++) {
			hpsm::View<FLOAT> v_power(power, row, col, bl_size, hpsm::PartitionMode::Matrix_Vert_Horiz, 
										  hpsm::AccessMode::In);
			hpsm::View<FLOAT> v_result(result, row, col, bl_size, hpsm::PartitionMode::Matrix_Vert_Horiz, 
										   hpsm::AccessMode::Out); 
			hpsm::View<FLOAT> v_temp(temp, row, col, row, hpsm::PartitionMode::Matrix_Vert_Horiz, 
										 hpsm::AccessMode::In);
			
			funcHotspot func(v_result, v_temp, v_power, Cap_1, Rx_1, Ry_1, Rz_1);
			hpsm::parallel_for(rg, v_result.block_range(), func);
			
			func.remove_data();
			
			/* Swap arrays */
			FLOAT* tmp = temp;
			temp       = result;
			result     = tmp;
		}
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
	std::cerr << "Usage:" << argv[0] << "<order> <block_size> <iterations> <serial>\n";
	std::cerr << "\t<order>      - order for the grid - size= <order>X<order> (positive integer)\n";
	std::cerr << "\t<block_size> - block size (positive integer divisible by <order>)\n";
	std::cerr << "\t<iterations> - number of iterations\n";
	std::cerr << "\t<serial>     - noz-zero value to execute the serial version\n";
	std::abort();
}

int main(int argc, char **argv) {
	int order_size, block_size, iterations;
	bool serial = false;
	FLOAT *temp, *power, *result, *temp_serial, *result_serial;
	
	/* check validity of inputs*/
	if(argc != 5)
		usage(argc, argv);
	if( (order_size = atoi(argv[1])) <= 0 ||
		(block_size = atoi(argv[2])) <= 0 ||
		(iterations = atoi(argv[3])) <= 0)  
		usage(argc, argv);

#ifdef VERBOSE
	int aux = atoi(argv[4]);
	serial = aux != 0 ? true : false;
#endif

	/* allocate memory for the temperature and power arrays	*/
	temp   = new FLOAT[order_size * order_size];
	power  = new FLOAT[order_size * order_size];
	result = new FLOAT[order_size * order_size];
	if(!temp || !power || ! result)
		fatal("unable to allocate memory");
	
	/* generate input */
	generate_input(temp, power, result, order_size);
	
	if(serial) {
		temp_serial   = new FLOAT[order_size * order_size];
		result_serial = new FLOAT[order_size * order_size];
		if(!temp_serial || ! result_serial)
			fatal("unable to allocate memory");
		
		for(unsigned i=0; i<order_size*order_size; i++) {
			temp_serial[i]   = temp[i];
			result_serial[i] = result[i];
		}
	}
	
#ifdef VERBOSE
	std::cout << "Backend: " << parallel_Backend_Str[BACKEND_OPT] << std::endl;
#endif
	
	hpsm::initialize();
	
#ifdef VERBOSE
	printf("Start computing the transient temperature\n");
#endif
	
	long long start_time = get_time();
	compute_tran_temp(result, temp, power, iterations, block_size, order_size);
	long long end_time = get_time();
	
	float total_time = ((float) (end_time - start_time)) / (1000*1000);
	
	hpsm::finalize();

#ifdef VERBOSE
	printf("Ending simulation\n");
	printf("Total time: %.3f seconds\n", total_time);
#else
	//printf("BENCH=hotspot;backend=%s;size=%d;block_size=%d;iterations=%d;threads=%d;gpus=%d;time=%.8f\n",
	printf("hotspot;%s;%d;%d;%d;%d;%d;%.8f\n", 
		parallel_Backend_Str[BACKEND_OPT], order_size, block_size, iterations, hpsm::num_workers_cpu(), hpsm::num_workers_gpu(), total_time);
#endif
	
	if(serial) {
		printf("SERIAL - Start computing the transient temperature\n");
		
		start_time = get_time();
		compute_tran_temp(result_serial, temp_serial, power, iterations, block_size, order_size, true);
		end_time = get_time();
		
		float total_time_serial = ((float) (end_time - start_time)) / (1000*1000);
		
		printf("SERIAL - Ending simulation\n");
		printf("SERIAL - Total time: %.3f seconds\n", total_time_serial);
		printf("Speedup: %g \n", total_time_serial/total_time);
		
		//Compare results
		std::cout << "Verifying...\n";
		bool ok = true;
		for(unsigned r=0; r<order_size; r++) {
			for(unsigned c=0; c<order_size; c++) {
				unsigned idx = r*order_size+c;
				
				if(abs(static_cast<FLOAT>(result[idx] - result_serial[idx])) > 0.0001f) {
					printf("%4d %4d Expected: %.5f | Calculated: %.5f\n", r, c, result_serial[idx], result[idx]);
					ok = false;
				}
			}
		}
		
		if(ok)
			std::cout << "Verification OK\n";
	}

	delete[] temp;
	delete[] power;
	delete[] result;
	
	if(serial) {
		delete[] temp_serial;
		delete[] result_serial;
	}

	return 0;
}