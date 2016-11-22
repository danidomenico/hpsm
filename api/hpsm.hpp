#pragma once

#include "parallel_const.hpp"
#include "parallel_utils.hpp"
#include "parallel_backend.hpp"

namespace hpsm {

/*
 * Type aliases
 */
using Functor = parallel_backend::Parallel_Functor;

template<typename TData>
using View = parallel_backend::Parallel_View<TData>;

using interval = parallel_utils::interval;

template<int dim>
using range = parallel_utils::range<dim>;

using block_range = parallel_utils::block_range;

template<int dim>
using index = parallel_utils::index<dim>;

/*
 * Templated functions
 */
template<int aux = 0>
void initialize() {
	parallel_backend::gb_backend.initialize();
}

template<int aux = 0>
void finalize() {
	parallel_backend::gb_backend.finalize();
}

template<int aux = 0>
unsigned num_workers_cpu() {
	return parallel_backend::gb_backend.num_workers(parallel_utils::worker_Cpu);
}

template<int aux = 0>
unsigned num_workers_gpu() {
	return parallel_backend::gb_backend.num_workers(parallel_utils::worker_Gpu);
}

template<typename TFunc>
void parallel_for(range<1> loop_rg, block_range block_rg, TFunc& func) {
	parallel_backend::gb_backend.parallel_for(loop_rg, block_rg, func);
}

template<typename TFunc>
void parallel_for(range<2> loop_rg, block_range block_rg, TFunc& func) {
	parallel_backend::gb_backend.parallel_for(loop_rg, block_rg, func);
}

template<typename TFunc, typename TData>
void parallel_reduce(range<1> loop_rg, block_range block_rg,
					 TFunc& func, TData& value, 
					 parallel_utils::Parallel_Redux redux_op) {
	parallel_backend::gb_backend.parallel_reduce(loop_rg, block_rg, func, value, redux_op);
}

template<typename TFunc, typename TData>
void parallel_reduce(range<2> loop_rg, block_range block_rg,
					 TFunc& func, TData& value, 
					 parallel_utils::Parallel_Redux redux_op) {
	parallel_backend::gb_backend.parallel_reduce(loop_rg, block_rg, func, value, redux_op);
}

} //namespace hpsm