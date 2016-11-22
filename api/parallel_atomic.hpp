#pragma once

namespace parallel_atomic {

#if defined(__CUDA_ARCH__)

/*
 * Functions prototypes
 */

/*
 * ADD
 */
template<typename TData>
__device__ void atomic_add(TData* dest, TData value) { }

__device__ void atomic_add(int* dest, int value);
__device__ void atomic_add(float* dest, float value);
__device__ void atomic_add(unsigned int* dest, unsigned int value);
__device__ void atomic_add(unsigned long long int* dest, unsigned long long int value);

/*
 * MULTI
 */
template<typename TData>
__device__ void atomic_multi(TData* dest, TData value) { }

/*
 * MIN
 */
template<typename TData>
__device__ void atomic_min(TData* dest, TData value) { }
__device__ void atomic_min(int* dest, int value);
__device__ void atomic_min(unsigned int* dest, unsigned int value);
//__device__ void atomic_min(unsigned long long int* dest, unsigned long long int value);

/*
 * MIN
 */
template<typename TData>
__device__ void atomic_max(TData* dest, TData value) { }
__device__ void atomic_max(int* dest, int value);
__device__ void atomic_max(unsigned int* dest, unsigned int value);
//__device__ //void atomic_max(unsigned long long int* dest, unsigned long long int value);

#endif

} //namespace parallel_atomic