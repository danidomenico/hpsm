#pragma once

#include "../parallel_utils.hpp"
#include "../parallel_runtime.hpp"
#include "serial_functor.hpp"
#include "serial_runtime.hpp"

#include <typeinfo>

namespace serial {

struct Serial : parallel_utils::Parallel_Backend_Key {
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
 * Templated functions Serial
 */
template<int aux>
void Serial::initialize() {
	new_keys(NUMBER_FUNCTORS);
}

template<typename TFunc>
void Serial::parallel_for(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	
	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i()))
			std::abort();
	
		insert_key(typeid(func).name());
	}
	
	unsigned block_start = loop_rg.getInterval_i().start / block_rg.block_size;
	unsigned block_end   = loop_rg.getInterval_i().end   / block_rg.block_size;
	
	//Submit tasks
	obj_func.block_tile = loop_rg.getBlockTile();
	for(unsigned i=block_start; i<block_end; i++) 
		submit_task_1D(func, 0, i);
}

template<typename TFunc>
void Serial::parallel_for(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);

	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, 
												loop_rg.getInterval_i(), loop_rg.getInterval_j()))
			std::abort();
		
		insert_key(typeid(func).name());
	}
	
	unsigned block_start, block_end, block_start_j, block_end_j;
	parallel_runtime::getMatrixStartEndBlock(obj_func, block_rg,
											 loop_rg.getInterval_i(), loop_rg.getInterval_j(),
											 block_start, block_end, block_start_j, block_end_j);
	
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	
	//Submit tasks
	obj_func.block_tile = loop_rg.getBlockTile();
	for(unsigned i=block_start; i<block_end; i++) {
		if(map == parallel_view::map_Mat_Vert_Horiz) {
			for(unsigned j=block_start_j; j<block_end_j; j++)
				submit_task_2D(func, 0, i, j);
		} else
			submit_task_2D(func, 0, i);
	}	
}

template<typename TFunc, typename TData>
void Serial::parallel_reduce(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	
	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i()))
			std::abort();
	
		insert_key(typeid(func).name());
	}
	
	obj_func.initialize_reduction_var(value, redux_op);
	uintptr_t reduction_var = reinterpret_cast<uintptr_t>(&value);
	
	unsigned block_start = loop_rg.getInterval_i().start / block_rg.block_size;
	unsigned block_end   = loop_rg.getInterval_i().end   / block_rg.block_size;
	
	//Submit tasks
	obj_func.block_tile = loop_rg.getBlockTile();
	for(unsigned i=block_start; i<block_end; i++) 
		submit_task_1D(func, reduction_var, i);
}

template<typename TFunc, typename TData>
void Serial::parallel_reduce(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);

	//Check views and range
	if(! has_key(typeid(func).name())) {
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, loop_rg.getInterval_i(), loop_rg.getInterval_j()))
			std::abort();
	
		insert_key(typeid(func).name());
	}
	
	obj_func.initialize_reduction_var(value, redux_op);
	uintptr_t reduction_var = reinterpret_cast<uintptr_t>(&value);
	
	unsigned block_start, block_end, block_start_j, block_end_j;
	parallel_runtime::getMatrixStartEndBlock(obj_func, block_rg, 
											 loop_rg.getInterval_i(), loop_rg.getInterval_j(),
											 block_start, block_end, block_start_j, block_end_j);
	
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	
	//Submit tasks
	obj_func.block_tile = loop_rg.getBlockTile();
	for(unsigned i=block_start; i<block_end; i++) {
		if(map == parallel_view::map_Mat_Vert_Horiz) {
			for(unsigned j=block_start_j; j<block_end_j; j++)
				submit_task_2D(func, reduction_var, i, j);
		} else
			submit_task_2D(func, reduction_var, i);
	}
}

} //namespace serial