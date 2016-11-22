#pragma once

#include "starpu.h"

#include "../parallel_macros.hpp"
#include "../parallel_utils.hpp"
#include "../parallel_runtime.hpp"
#include "starpu_functor.hpp"
#include "starpu_runtime.hpp"
#include <typeinfo>

namespace starpu {
	
struct StarPU_Objects : parallel_utils::functor_id {
	starpu_perfmodel perf_model;
	starpu_codelet   codelet;
	starpu_codelet   codelet_init;
	starpu_codelet   codelet_redux;
};

struct StarPU {
	template<int aux = 0>
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
	
protected:
	int array_idx;
	int array_size;
	StarPU_Objects* array_objects;
	
	template<typename TFunc>
	void configure_performance_model(TFunc& func);
	
	template<typename TFunc>
	void configure_codelet_1D(TFunc& func, bool gpu = true);
	
	template<typename TFunc>
	void configure_codelet_2D(TFunc& func, bool gpu = true);
	
	template<typename TFunc, typename TData>
	void configure_codelet_reduction_init_redux(TFunc& func, bool gpu = true);
	
	template<typename TFunc, typename TData>
	void configure_codelet_reduction_1D(TFunc& func, bool gpu = true);
	
	template<typename TFunc, typename TData>
	void configure_codelet_reduction_2D(TFunc& func, bool gpu = true);
	
	int index_key(std::string key);
	int insert_key(std::string key);
	
private:
	unsigned get_gpu_max_blocks();
};

/*
 * Templated functions StarPU
 */
template<int aux>
void StarPU::initialize() { 
	int err_ret = starpu_init(NULL);
	if(err_ret == -ENODEV) {
		char msg[100] = "";
		std::sprintf(msg, "Error initializing StarPU runtime. Error code: %d", err_ret);
		parallel_utils::print_message(msg);
		std::abort();
	};
	
	//Test disable implicit dependencies
	//starpu_data_set_default_sequential_consistency_flag(0);
	
	array_objects = new StarPU_Objects[NUMBER_FUNCTORS];
	array_size = 0;
	
	get_gpu_max_blocks();
}

template<typename TFunc>
void StarPU::parallel_for(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	obj_func.gpu_max_blocks = get_gpu_max_blocks();
	
	array_idx = index_key(typeid(func).name());
	if(array_idx < 0) {
		//Check views and range
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i()))
			std::abort();
		
		//Create and configure codelet
		configure_codelet_1D(func);
	}
	
	unsigned block_start = loop_rg.getInterval_i().start / block_rg.block_size;
	unsigned block_end   = loop_rg.getInterval_i().end   / block_rg.block_size;
	
	//Create and submit tasks
	std::vector<StarPU_Args<TFunc>*> list_args;
	list_args.reserve((obj_func.view_container.getBlockView().block_number));
	obj_func.block_tile = loop_rg.getBlockTile();
	
	for(unsigned i=block_start; i<block_end; i++)
		submit_task(func, array_objects[array_idx].codelet, false, list_args, i);
	
	starpu_task_wait_for_all();
	
	for(StarPU_Args<TFunc>* args : list_args)
		delete args;
}

template<typename TFunc>
void StarPU::parallel_for(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg, TFunc& func) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	obj_func.gpu_max_blocks = get_gpu_max_blocks();

	array_idx = index_key(typeid(func).name());
	if(array_idx < 0) {
		//Check views and range
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, 
												   loop_rg.getInterval_i(), loop_rg.getInterval_j()))
			std::abort();
		
		//Create and configure codelet
		configure_codelet_2D(func);
	}
	
	unsigned block_start, block_end, block_start_j, block_end_j;
	parallel_runtime::getMatrixStartEndBlock(obj_func, block_rg,
											 loop_rg.getInterval_i(), loop_rg.getInterval_j(),
											 block_start, block_end, block_start_j, block_end_j);
	
	
	//Create and submit tasks
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	std::vector<StarPU_Args<TFunc>*> list_args;
	list_args.reserve((obj_func.view_container.getBlockView().block_number));
	obj_func.block_tile = loop_rg.getBlockTile();
	
	for(unsigned i=block_start; i<block_end; i++) {
		if(map == parallel_view::map_Mat_Vert_Horiz) {
			for(unsigned j=block_start_j; j<block_end_j; j++)
				submit_task(func, array_objects[array_idx].codelet, false, list_args, i, j);
		} else
			submit_task(func, array_objects[array_idx].codelet, false, list_args, i);
	}
	
	starpu_task_wait_for_all();
	
	for(StarPU_Args<TFunc>* args : list_args)
		delete args;
}

template<typename TFunc, typename TData>
void StarPU::parallel_reduce(parallel_utils::range<1> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	obj_func.gpu_max_blocks = get_gpu_max_blocks();

	array_idx = index_key(typeid(func).name());
	if(array_idx < 0) {
		//Check views and range
		if(! parallel_runtime::checkParallelFor_1D(obj_func, block_rg, loop_rg.getInterval_i()))
			std::abort();
		
		//Create and configure codelet
		configure_codelet_reduction_1D<TFunc, TData>(func);
	}
	
	obj_func.reduction_var.register_variable(&value, redux_op);
	starpu_data_set_reduction_methods(obj_func.reduction_var.handle, 
									  &array_objects[array_idx].codelet_redux, &array_objects[array_idx].codelet_init);
	
	unsigned block_start = loop_rg.getInterval_i().start / block_rg.block_size;
	unsigned block_end   = loop_rg.getInterval_i().end   / block_rg.block_size;
	
	//Create and submit tasks
	std::vector<StarPU_Args<TFunc>*> list_args;
	list_args.reserve((obj_func.view_container.getBlockView().block_number));
	obj_func.block_tile = loop_rg.getBlockTile();
	
	for(unsigned i=block_start; i<block_end; i++)
		submit_task(func, array_objects[array_idx].codelet, true, list_args, i);
	
	starpu_task_wait_for_all();
	
	for(StarPU_Args<TFunc>* args : list_args)
		delete args;
	
	//Executes reduction codelet (cl_redux)
	obj_func.reduction_var.unregister_variable();
}

template<typename TFunc, typename TData>
void StarPU::parallel_reduce(parallel_utils::range<2> loop_rg, parallel_utils::block_range block_rg,
							 TFunc& func, TData& value, 
							 parallel_utils::Parallel_Redux redux_op) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	obj_func.view_container.setBlockView(block_rg.view_id);
	obj_func.gpu_max_blocks = get_gpu_max_blocks();

	array_idx = index_key(typeid(func).name());
	if(array_idx < 0) {
		//Check views and range
		if(! parallel_runtime::checkParallelFor_2D(obj_func, block_rg, 
												   loop_rg.getInterval_i(), loop_rg.getInterval_j()))
			std::abort();
		
		//Create and configure codelet
		configure_codelet_reduction_2D<TFunc, TData>(func);
	}
	
	obj_func.reduction_var.register_variable(&value, redux_op);
	starpu_data_set_reduction_methods(obj_func.reduction_var.handle, 
									  &array_objects[array_idx].codelet_redux, &array_objects[array_idx].codelet_init);
	
	unsigned block_start, block_end, block_start_j, block_end_j;
	parallel_runtime::getMatrixStartEndBlock(obj_func, block_rg,
											 loop_rg.getInterval_i(), loop_rg.getInterval_j(),
											 block_start, block_end, block_start_j, block_end_j);
	
	//Create and submit tasks
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	std::vector<StarPU_Args<TFunc>*> list_args;
	list_args.reserve((obj_func.view_container.getBlockView().block_number));
	obj_func.block_tile = loop_rg.getBlockTile();
	
	for(unsigned i=block_start; i<block_end; i++) {
		if(map == parallel_view::map_Mat_Vert_Horiz) {
			for(unsigned j=block_start_j; j<block_end_j; j++)
				submit_task(func, array_objects[array_idx].codelet, true, list_args, i, j);
		} else
			submit_task(func, array_objects[array_idx].codelet, true, list_args, i);
	}
	
	starpu_task_wait_for_all();
	
	for(StarPU_Args<TFunc>* args : list_args)
		delete args;
	
	//Executes reduction codelet (cl_redux)
	obj_func.reduction_var.unregister_variable();
}

template<typename TFunc>
void StarPU::configure_performance_model(TFunc& func) {
	starpu_perfmodel& perf = array_objects[array_idx].perf_model;
	
	std::memset(&perf, 0, sizeof(perf));
	const char* model = typeid(func).name();
	perf.symbol = model;
	perf.type = STARPU_HISTORY_BASED;
	array_objects[array_idx].codelet.model = &perf;
	
	//printf("Model configurado - idx: %d key: %s\n", array_idx, model);
}
	
template<typename TFunc>
void StarPU::configure_codelet_1D(TFunc& func, bool gpu) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	
	array_idx = insert_key(typeid(func).name());
	starpu_codelet& cl = array_objects[array_idx].codelet;
		
	//Defining codelet
	starpu_codelet_init(&cl);
	cl.cpu_funcs[0] = cpu_impl_1D<TFunc>;
	const char* cpu_funcs_name = typeid(func).name();
	cl.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_name);
#if defined(_OPENMP)
	cl.type = STARPU_FORKJOIN;
	cl.max_parallelism = INT_MAX;
#endif
	
	if(gpu)
		cl.cuda_funcs[0] = cuda_impl_1D<TFunc>;

	cl.nbuffers = obj_func.view_container.parallel_views.size();
	for(int i=0; i<cl.nbuffers; i++)
		cl.modes[i] = obj_func.view_container.parallel_views[i]->mode;
	
	cl.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	
	configure_performance_model(func);
}

template<typename TFunc>
void StarPU::configure_codelet_2D(TFunc& func, bool gpu) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	
	array_idx = insert_key(typeid(func).name());
	starpu_codelet& cl = array_objects[array_idx].codelet;
	
	//Defining codelet
	starpu_codelet_init(&cl);
	cl.cpu_funcs[0] = cpu_impl_2D<TFunc>;
	const char* cpu_funcs_name = typeid(func).name();
	cl.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_name);
#if defined(_OPENMP)
	cl.type = STARPU_FORKJOIN;
	cl.max_parallelism = INT_MAX;
#endif	
	
	if(gpu)
		cl.cuda_funcs[0] = cuda_impl_2D<TFunc>;

	cl.nbuffers = obj_func.view_container.parallel_views.size();
	for(int i=0; i<cl.nbuffers; i++)
		cl.modes[i] = obj_func.view_container.parallel_views[i]->mode;
	
	cl.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	
	configure_performance_model(func);
}

template<typename TFunc, typename TData>
void StarPU::configure_codelet_reduction_init_redux(TFunc& func, bool gpu) {
	//Defining INIT_CODELET
	starpu_codelet& cl_init = array_objects[array_idx].codelet_init;
	
	starpu_codelet_init(&cl_init);
	
	cl_init.cpu_funcs[0] = cpu_impl_reduction_init<TData>;
	const char* cpu_funcs_init_name = typeid(func).name();
	cl_init.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_init_name);
	if(gpu) {
		cl_init.cuda_funcs[0] = cuda_impl_reduction_init<TData>;
		cl_init.cuda_flags[0] = STARPU_CUDA_ASYNC;
	}
	cl_init.nbuffers = 1;
	cl_init.modes[0] = STARPU_W;
	cl_init.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	cl_init.name = "init";
	
	//Defining REDUX_CODELET
	starpu_codelet& cl_redux = array_objects[array_idx].codelet_redux;
	starpu_codelet_init(&cl_redux);
	cl_redux.cpu_funcs[0] = cpu_impl_reduction_redux<TData>;
	const char* cpu_funcs_redux_name = typeid(func).name();
	cl_redux.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_redux_name);
	if(gpu) {
		cl_redux.cuda_funcs[0] = cuda_impl_reduction_redux<TData>;
		cl_redux.cuda_flags[0] = STARPU_CUDA_ASYNC;
	}
	cl_redux.nbuffers = 2;
	cl_redux.modes[0] = STARPU_RW;
	cl_redux.modes[1] = STARPU_R;
	cl_redux.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	cl_redux.name = "redux";
}

template<typename TFunc, typename TData>
void StarPU::configure_codelet_reduction_1D(TFunc& func, bool gpu) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	
	array_idx = insert_key(typeid(func).name());
	starpu_codelet& cl_exec = array_objects[array_idx].codelet;
	
	//Defining EXEC_CODELET
	starpu_codelet_init(&cl_exec);
	cl_exec.cpu_funcs[0] = cpu_impl_reduction_1D<TFunc>; 
	const char* cpu_funcs_name = typeid(func).name();
	cl_exec.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_name);

	if(gpu)
		cl_exec.cuda_funcs[0] = cuda_impl_reduction_1D<TFunc>;

	int size = obj_func.view_container.parallel_views.size();
	cl_exec.nbuffers = size + 1;
	for(int i=0; i<size; i++)
		cl_exec.modes[i] = obj_func.view_container.parallel_views[i]->mode;
	cl_exec.modes[size] = STARPU_REDUX;
	cl_exec.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	cl_exec.name = "exec";
	
	configure_performance_model(func);
	configure_codelet_reduction_init_redux<TFunc, TData>(func, gpu);
}

template<typename TFunc, typename TData>
void StarPU::configure_codelet_reduction_2D(TFunc& func, bool gpu) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	
	array_idx = insert_key(typeid(func).name());
	starpu_codelet& cl_exec = array_objects[array_idx].codelet;
	
	//Defining EXEC_CODELET
	starpu_codelet_init(&cl_exec);
	cl_exec.cpu_funcs[0] = cpu_impl_reduction_2D<TFunc>; 
	const char* cpu_funcs_name = typeid(func).name();
	cl_exec.cpu_funcs_name[0] = const_cast<char*>(cpu_funcs_name);

	if(gpu)
		cl_exec.cuda_funcs[0] = cuda_impl_reduction_2D<TFunc>;

	int size = obj_func.view_container.parallel_views.size();
	cl_exec.nbuffers = size + 1;
	for(int i=0; i<size; i++)
		cl_exec.modes[i] = obj_func.view_container.parallel_views[i]->mode;
	cl_exec.modes[size] = STARPU_REDUX;
	cl_exec.where = (gpu ? STARPU_CPU | STARPU_CUDA : STARPU_CPU);
	cl_exec.name = "exec";
	
	configure_performance_model(func);
	configure_codelet_reduction_init_redux<TFunc, TData>(func, gpu);
}

} //namespace starpu
