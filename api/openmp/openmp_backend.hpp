#pragma once

#include "../parallel_utils.hpp"
#include "../parallel_runtime.hpp"
#include "openmp_functor.hpp"

#include <omp.h>
#include <typeinfo>

namespace openmp {

struct OpenMP : parallel_utils::Parallel_Backend_Key {
	template<int aux=0>
	void initialize();
	
	void finalize();
	
	unsigned num_workers(parallel_utils::Parallel_Workers worker_type);
	
	template<typename TFunc>
	void parallel_for(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg, TFunc& func);
	
	template<typename TFunc>
	void parallel_for(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg, TFunc& func);
	
	template<typename TFunc, typename TData>
	void parallel_reduce(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg,
						 TFunc& func, TData& value, 
						 parallel_utils::Parallel_Redux redux_op);
	
	template<typename TFunc, typename TData>
	void parallel_reduce(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg,
						 TFunc& func, TData& value, 
						 parallel_utils::Parallel_Redux redux_op);
};

/*
 * Templated functions OpenMP
 */
template<int aux>
void OpenMP::initialize() { 
	new_keys(NUMBER_FUNCTORS);
}

template<typename TFunc>
void OpenMP::parallel_for(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	OpenMP_Functor& obj_func = static_cast<OpenMP_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	
	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i(), false))
			std::abort();
		
		insert_key(typeid(func).name());
	}
	
	//Set views Data
	for(unsigned i=0; i<obj_func.view_container.parallel_views.size(); i++) {
		OpenMP_View& view = *(obj_func.view_container.parallel_views[i]);
		parallel_view::data_t data_block(view.data, view.size);
		
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	
	unsigned start = loop_rg.getInterval_i().start;
	unsigned end   = loop_rg.getInterval_i().end;
	unsigned n     = obj_func.view_container.getBlockView().size;
	
	//std::cout << "OpenMP 1D\n";
	#pragma omp parallel for schedule(runtime)
	for(int i=start; i<end; i++) {
		//printf("i=%d, num=%d, thread=%d\n", i, omp_get_num_threads(), omp_get_thread_num());
		parallel_utils::index<1> idx(i, 0, 1, n);
		func(idx);
	}
}

template<typename TFunc>
void OpenMP::parallel_for(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	OpenMP_Functor& obj_func = static_cast<OpenMP_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);

	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, 
												loop_rg.getInterval_i(), loop_rg.getInterval_j(),
												false))
			std::abort();
		
		insert_key(typeid(func).name());
	}
	
	for(unsigned k=0; k<obj_func.view_container.parallel_views.size(); k++) {
		OpenMP_View& view = *(obj_func.view_container.parallel_views[k]);
		
		unsigned size_block_i, size_block_j;
		view.getMatrixSize(size_block_i, size_block_j);
		
		parallel_view::data_t data_block(view.data,
										 size_block_i,
										 size_block_j,
										 view.getMatrixLineDepth());
		obj_func.view_container.parallel_views[k]->data_block = data_block;
	}
	
	OpenMP_View& view = static_cast<OpenMP_View&>(obj_func.view_container.getBlockView());
	unsigned nx, ny;
	view.getMatrixSize(ny, nx);
	
	unsigned start_i = loop_rg.getInterval_i().start;
	unsigned end_i   = loop_rg.getInterval_i().end;
	unsigned start_j = loop_rg.getInterval_j().start;
	unsigned end_j   = loop_rg.getInterval_j().end;
	
	//std::cout << "OpenMP 2D\n";
	
	if(view.map == parallel_view::map_Mat_Horiz) {
		for(int i=start_i; i<end_i; i++) {
			#pragma omp parallel for schedule(runtime)
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	} else if(view.map == parallel_view::map_Mat_Vert) {
		#pragma omp parallel for schedule(runtime)
		for(int i=start_i; i<end_i; i++) {
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	} else { //parallel_view::map_Mat_Vert_Horiz
		#pragma omp parallel for schedule(runtime) collapse(2)
		for(int i=start_i; i<end_i; i++) {
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	}
}

template<typename TFunc, typename TData>
void OpenMP::parallel_reduce(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	OpenMP_Functor& obj_func = static_cast<OpenMP_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	
	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i(), false))
			std::abort();
		
		insert_key(typeid(func).name());
	}
	
	obj_func.initialize_reduction_var(value, redux_op);
	
	//Set views Data
	for(unsigned i=0; i<obj_func.view_container.parallel_views.size(); i++) {
		OpenMP_View& view = *(obj_func.view_container.parallel_views[i]);
		parallel_view::data_t data_block(view.data, view.size);
		
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	
	unsigned start = loop_rg.getInterval_i().start;
	unsigned end   = loop_rg.getInterval_i().end;
	unsigned n     = obj_func.view_container.getBlockView().size;
	
	//Create private arrays for each thread
	unsigned num_threads;
	#pragma omp parallel
	#pragma omp master
		num_threads = omp_get_num_threads();
	
	TData* values = new TData[num_threads];
	obj_func.reduction_var_ptr = new uintptr_t[num_threads];
	for(int i=0; i<num_threads; i++) {
		values[i] = value;
		obj_func.reduction_var_ptr[i] = reinterpret_cast<uintptr_t>(&values[i]);
	}
	
	//std::cout << "OpenMP Redux 1D\n";
	#pragma omp parallel for schedule(runtime)
	for(int i=start; i<end; i++) {
		//printf("i=%d, num=%d, thread=%d\n", i, omp_get_num_threads(), omp_get_thread_num());
		parallel_utils::index<1> idx(i, 0, 1, n);
		func(idx);
	}
	
	obj_func.totalize_reduction_var(value, redux_op, num_threads);
	
	delete[] obj_func.reduction_var_ptr;
	delete[] values;
}

template<typename TFunc, typename TData>
void OpenMP::parallel_reduce(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	OpenMP_Functor& obj_func = static_cast<OpenMP_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);

	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, 
												loop_rg.getInterval_i(), loop_rg.getInterval_j(),
												false))
			std::abort();
	
		insert_key(typeid(func).name());
	}
	
	for(unsigned k=0; k<obj_func.view_container.parallel_views.size(); k++) {
		OpenMP_View& view = *(obj_func.view_container.parallel_views[k]);
		
		unsigned size_block_i, size_block_j;
		view.getMatrixSize(size_block_i, size_block_j);
		
		parallel_view::data_t data_block(view.data,
										 size_block_i,
										 size_block_j,
										 view.getMatrixLineDepth());
		obj_func.view_container.parallel_views[k]->data_block = data_block;
	}
	
	OpenMP_View& view = static_cast<OpenMP_View&>(obj_func.view_container.getBlockView());
	unsigned nx, ny;
	view.getMatrixSize(ny, nx);
	
	unsigned start_i = loop_rg.getInterval_i().start;
	unsigned end_i   = loop_rg.getInterval_i().end;
	unsigned start_j = loop_rg.getInterval_j().start;
	unsigned end_j   = loop_rg.getInterval_j().end;
	
	//Create private arrays for each thread
	unsigned num_threads;
	#pragma omp parallel
	#pragma omp master
		num_threads = omp_get_num_threads();
	
	TData* values = new TData[num_threads];
	obj_func.reduction_var_ptr = new uintptr_t[num_threads];
	for(int i=0; i<num_threads; i++) {
		values[i] = value;
		obj_func.reduction_var_ptr[i] = reinterpret_cast<uintptr_t>(&values[i]);
	}
	
	//std::cout << "OpenMP Redux 2D\n";
	
	if(view.map == parallel_view::map_Mat_Horiz) {
		for(int i=start_i; i<end_i; i++) {
			#pragma omp parallel for schedule(runtime)
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	} else if(view.map == parallel_view::map_Mat_Vert) {
		#pragma omp parallel for schedule(runtime)
		for(int i=start_i; i<end_i; i++) {
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	} else { //parallel_view::map_Mat_Vert_Horiz
		#pragma omp parallel for schedule(runtime) collapse(2)
		for(int i=start_i; i<end_i; i++) {
			for(int j= start_j; j<end_j; j++) {
				//printf("i=%d, j=%d, thread=%d\n", i, j, omp_get_thread_num());
				parallel_utils::index<2> idx(i, j, 0, 0, 0, 1, 1, 1, ny, nx);
				func(idx);
			}
		}
	}
	
	obj_func.totalize_reduction_var(value, redux_op, num_threads);
	
	delete[] obj_func.reduction_var_ptr;
	delete[] values;
}

} //namespace openmp