#include "serial_backend.hpp"

namespace serial {

/*
 * Functions Serial
 */
void Serial::finalize() { 
	delete_keys();
}

unsigned Serial::num_workers(parallel_utils::Parallel_Workers worker_type) {
	switch(worker_type) {
		case parallel_utils::worker_Gpu:
			return 0;
			
		default: //parallel_utils::worker_Cpu:
			return 1;
	};
}

} //namespace serial