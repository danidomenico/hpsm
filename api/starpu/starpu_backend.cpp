#include "starpu_backend.hpp"

namespace starpu {

/*
 * Functions StarPU
 */
void StarPU::finalize() { 
	starpu_shutdown();
	
	delete[] array_objects;
}

unsigned StarPU::num_workers(parallel_utils::Parallel_Workers worker_type) {
	switch(worker_type) {
		case parallel_utils::worker_Gpu :
			return starpu_cuda_worker_get_count();
			
		default: //parallel_utils::worker_Cpu:
			return starpu_cpu_worker_get_count();
	};
}

int StarPU::index_key(std::string key) {
	for(int i=0; i<array_size; i++) {
		if(array_objects[i].key == key)
			return i;
	}
	
	return -1;
}

int StarPU::insert_key(std::string key) {
	array_objects[array_size].key = key;
	array_size++;
	
	return array_size-1;
}

unsigned StarPU::get_gpu_max_blocks() {
	if(num_workers(hpsm::Workers::Gpu) > 0) {
		cudaDeviceProp prop;
		cudaGetDeviceProperties(&prop, 0); //Get properties from GPU 0
		
		//printf("Name: %s \n", prop.name);
		unsigned arch = prop.major*100 + prop.minor*10;
		//printf("Arch: %d %d %d\n", prop.major, prop.minor, arch);
		//printf("Grids: %d %d %d \n",  prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
		
		return arch < 300 ? 65535 : prop.maxGridSize[0];
	}
	
	return 0;
}

} //namespace serial