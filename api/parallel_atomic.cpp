#include "parallel_atomic.hpp"

namespace parallel_atomic {

#if defined(__CUDA_ARCH__)
/*
 * ADD
 */
__device__
void atomic_add(int* dest, int value) {
	atomicAdd(dest, value);
}

__device__
void atomic_add(float* dest, float value) {
	atomicAdd(dest, value);
}

__device__
void atomic_add(unsigned int* dest, unsigned int value) {
	atomicAdd(dest, value);
}

__device__
void atomic_add(unsigned long long int* dest, unsigned long long int value) {
	atomicAdd(dest, value);
}

/*
 * MIN
 */
__device__
void atomic_min(int* dest, int value) {
	atomicMin(dest, value);
}

__device__
void atomic_min(unsigned int* dest, unsigned int value) {
	atomicMin(dest, value);
}

//__device__
//void atomic_min(unsigned long long int* dest, unsigned long long int value) {
//	atomicMin(dest, value);
//}

/*
 * MIN
 */
__device__
void atomic_max(int* dest, int value) {
	atomicMax(dest, value);
}

__device__
void atomic_max(unsigned int* dest, unsigned int value) {
	atomicMax(dest, value);
}

//__device__
//void atomic_max(unsigned long long int* dest, unsigned long long int value) {
//	atomicMax(dest, value);
//}
#endif

} //namespace parallel_atomic