#pragma once

#include "starpu.h"

#include "../parallel_utils.hpp"
#include "../parallel_view.hpp"

#include <limits>

namespace starpu {

/*
 * Global variables
 */
extern parallel_utils::Parallel_Redux gb_redux_op;

/*
 * Types
 */
struct StarPU_View : parallel_view::view_t {
	starpu_data_handle_t handle;
	starpu_data_access_mode mode;
	
protected:
	template<typename TData>
	void set_vector(TData* a_data, unsigned a_size, unsigned a_block_size, 
					hpsm::AccessMode::Parallel_AccessMode a_mode);
	
	template<typename TData>
	void set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
					unsigned a_block_size, parallel_view::Parallel_Map a_map,
					hpsm::AccessMode::Parallel_AccessMode a_mode);
	
private:
	void set_StarPU_mode(hpsm::AccessMode::Parallel_AccessMode a_mode);
	void partition_data();
};

struct StarPU_Red_Var {
	starpu_data_handle_t handle;
	
	template<typename TData>
	void register_variable(TData* data, parallel_utils::Parallel_Redux a_redux_op);
	
	void unregister_variable();
};

struct StarPU_Functor {
	parallel_view::view_container_t<StarPU_View> view_container;
	StarPU_Red_Var reduction_var;
	
	uintptr_t reduction_var_ptr;
	parallel_utils::Parallel_BlockTile block_tile;
	
	unsigned gpu_max_blocks = 0;
	
	StarPU_Functor() {};
	
	StarPU_Functor(const StarPU_Functor& other) : 
		view_container(other.view_container), reduction_var_ptr(other.reduction_var_ptr),
		gpu_max_blocks(other.gpu_max_blocks) {};
		
	template<typename TData, typename... Args>
	void register_data(TData& value, Args&... args);
    void register_data(void) { }
	
	void clear_data();
	void remove_data();
};

template<typename TFunc>
struct StarPU_Args {
	TFunc func;
	
	unsigned idx_block;
	
	unsigned idx_block_i;
	unsigned idx_block_j;
	
	StarPU_Args(TFunc& _func, unsigned _idx_block, unsigned _idx_block_i = -1, unsigned _idx_block_j = -1) : 
		func(_func), idx_block(_idx_block), idx_block_i(_idx_block_i), idx_block_j(_idx_block_j) {}
};

/*
 * Templated functions StarPU_View
 */
template<typename TData>
void StarPU_View::set_vector(TData* a_data, unsigned a_size, unsigned a_block_size, 
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	size = a_size;
	block_size = a_block_size;
	map  = parallel_view::map_Vector;
	
	setBlockNumber();
	
	set_StarPU_mode(a_mode);
	
	starpu_vector_data_register(&handle, STARPU_MAIN_RAM, reinterpret_cast<uintptr_t>(a_data), 
								size, sizeof(TData));
	partition_data();
}

template<typename TData>
void StarPU_View::set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
							 unsigned a_block_size, parallel_view::Parallel_Map a_map,
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	line_number   = a_line_number;
	column_number = a_column_number;
	block_size    = a_block_size;
	map           = a_map;
	
	setBlockNumber();
	set_StarPU_mode(a_mode);
	
	starpu_matrix_data_register(&handle, STARPU_MAIN_RAM, reinterpret_cast<uintptr_t>(a_data), 
								column_number, column_number, line_number, sizeof(TData));
	
	partition_data();
}

/*
 * Templated functions StarPU_Red_Var
 */
template<typename TData>
void StarPU_Red_Var::register_variable(TData* data, parallel_utils::Parallel_Redux a_redux_op) {
	starpu_variable_data_register(&handle, STARPU_MAIN_RAM, reinterpret_cast<uintptr_t>(data), 
								  sizeof(TData));
	gb_redux_op = a_redux_op;
	
	switch(gb_redux_op) {
		case parallel_utils::red_Mult:
			*data = static_cast<TData>(1);
			break;
			
		case parallel_utils::red_Max:
			*data =  std::numeric_limits<TData>::min();
			break;
			
		case parallel_utils::red_Min:
			*data =  std::numeric_limits<TData>::max();
			break;
			
		default:
			*data = static_cast<TData>(0);
	}
}

/*
 * Templated functions StarPU_Functor
 */
template<typename TData, typename... Args>
void StarPU_Functor::register_data(TData& value, Args&... args) {
	view_container.parallel_views.push_back(&value);
	register_data(args...);
}

} //namespace starpu