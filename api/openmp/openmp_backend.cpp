#include "openmp_backend.hpp"

namespace openmp {

/*
 * Functions OpenMP
 */
void OpenMP::finalize() { 
	delete_keys();
}

unsigned OpenMP::num_workers(parallel_utils::Parallel_Workers worker_type) {
	if(worker_type == parallel_utils::worker_Gpu) {
		return 0;
	
	} else { //parallel_utils::worker_Cpu
		unsigned num_threads;
		#pragma omp parallel
		#pragma omp master
			num_threads = omp_get_num_threads();
		
		return num_threads;
	};
}

} //namespace openmp