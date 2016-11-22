#pragma once

#include "parallel_const.hpp"
#include "parallel_utils.hpp"
#include "parallel_macros.hpp"

#if defined(BACKEND_OPENMP)
#include "openmp/openmp_backend.hpp"
#else
namespace openmp{
struct OpenMP {};
struct OpenMP_Functor {};
struct OpenMP_View {};
};
#endif

#include "serial/serial_backend.hpp"

#if defined(BACKEND_STARPU)
#include "starpu/starpu_backend.hpp"
#else
namespace starpu{
struct StarPU {};
struct StarPU_Functor {};
struct StarPU_View {};
};
#endif

namespace parallel_backend {

/*
 * Types 
 */
template<Parallel_Backend backend_option>
using Backend = metaprog::Select<backend_option, openmp::OpenMP, serial::Serial, starpu::StarPU>;

template<Parallel_Backend backend_option>
using Backend_Functor = metaprog::Select<backend_option, openmp::OpenMP_Functor, serial::Serial_Functor, starpu::StarPU_Functor>;

template<Parallel_Backend backend_option>
using Backend_View = metaprog::Select<backend_option, openmp::OpenMP_View, serial::Serial_View, starpu::StarPU_View>;

struct Parallel_Functor : Backend_Functor<BACKEND_OPT> {
	template<typename TData>
	PARALLEL_FUNCTION
	TData* reduction_var() {
		#if defined(__CUDA_ARCH__)
			return reinterpret_cast<TData*>(Backend_Functor<BACKEND_OPT>::reduction_var_ptr);
		#else
			#if defined BACKEND_OPENMP
				return reinterpret_cast<TData*>(openmp::OpenMP_Functor::reduction_var_ptr[omp_get_thread_num()]);
			#else
				return reinterpret_cast<TData*>(Backend_Functor<BACKEND_OPT>::reduction_var_ptr);
			#endif
		#endif
	}
};

template<typename TData>
struct Parallel_View : Backend_View<BACKEND_OPT> {
	
	Parallel_View() {};
	
	Parallel_View(TData* data, unsigned size, unsigned block_size, 
				  hpsm::AccessMode::Parallel_AccessMode mode = hpsm::AccessMode::InOut) {
		Backend_View<BACKEND_OPT>::set_vector(data, size, block_size, mode);
	}
	
	Parallel_View(TData* data, unsigned line_number, unsigned column_number,
				  unsigned block_size, hpsm::PartitionMode::Parallel_PartitionMode map,
				  hpsm::AccessMode::Parallel_AccessMode mode = hpsm::AccessMode::InOut) {
		Backend_View<BACKEND_OPT>::set_matrix(data, line_number, column_number, block_size, map, mode);
	}
	
	PARALLEL_FUNCTION
	TData& operator()(unsigned i) {
		return Backend_View<BACKEND_OPT>::data_block.get<TData>(i);
	}
	
	PARALLEL_FUNCTION
	TData& operator()(unsigned i, unsigned j) {
		return Backend_View<BACKEND_OPT>::data_block.get<TData>(i, j);
	}
	
	PARALLEL_FUNCTION
	TData& operator()(parallel_utils::index<1> idx) {
		return Backend_View<BACKEND_OPT>::data_block.get<TData>(idx);
	}
	
	PARALLEL_FUNCTION
	TData& operator()(parallel_utils::index<2> idx) {
		return Backend_View<BACKEND_OPT>::data_block.get<TData>(idx);
	}
	
	PARALLEL_FUNCTION
	unsigned size(unsigned dimension = 0) {
		return Backend_View<BACKEND_OPT>::data_block.size(dimension);
	}
};

/*
 * Global variables
 */
extern Backend<BACKEND_OPT> gb_backend;

} //namespace parallel_backend
