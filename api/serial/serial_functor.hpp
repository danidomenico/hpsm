#pragma once

#include "../parallel_utils.hpp"
#include "../parallel_view.hpp"

#include <limits>

namespace serial {

//Foward declaration
struct Serial_Functor;

struct Serial_View : parallel_view::view_t {
	uintptr_t data;
	size_t data_size;
	 
	uintptr_t getDataBlock(unsigned idx_block, unsigned block_number_exec,
						   parallel_utils::Parallel_BlockTile block_tile);
	uintptr_t getDataBlock_MatVerHoriz(unsigned idx_block_line, unsigned idx_block_column,
						               Serial_Functor& obj_func);
	unsigned getMatrixLineDepth();
	void getMatrixBlockSize(unsigned& size_i, unsigned& size_j);

protected:
	template<typename TData>
	void set_vector(TData* a_data, unsigned a_size, unsigned a_block_size, 
					hpsm::AccessMode::Parallel_AccessMode a_mode);
	
	template<typename TData>
	void set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
					unsigned a_block_size, parallel_view::Parallel_Map a_map, 
					hpsm::AccessMode::Parallel_AccessMode a_mode);
};

struct Serial_Functor {
	parallel_view::view_container_t<Serial_View> view_container;
	
	uintptr_t reduction_var_ptr;
	parallel_utils::Parallel_BlockTile block_tile;
	
	template<typename TData>
	void initialize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op);
	
    template<typename TData, typename... Args>
    void register_data(TData& value, Args&... args);
    void register_data(void) { }
	
	void clear_data();
	void remove_data();
};

/*
 * Templated functions Serial_View
 */
template<typename TData>
void Serial_View::set_vector(TData* a_data, unsigned a_size, unsigned a_block_size,
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	size = a_size;
	block_size = a_block_size;
	map  = parallel_view::map_Vector;
	
	setBlockNumber();
	
	data = reinterpret_cast<uintptr_t>(a_data);
	data_size = sizeof(TData);
}

template<typename TData>
void Serial_View::set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
							 unsigned a_block_size, parallel_view::Parallel_Map a_map, 
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	line_number   = a_line_number;
	column_number = a_column_number;
	block_size    = a_block_size;
	map           = a_map;
	
	setBlockNumber();
	
	data = reinterpret_cast<uintptr_t>(a_data);
	data_size = sizeof(TData);
}

/*
 * Templated functions Serial_Functor
 */
template<typename TData>
void Serial_Functor::initialize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op) {
	switch(redux_op) {
		case parallel_utils::red_Mult:
			data = static_cast<TData>(1);
			break;
			
		case parallel_utils::red_Max:
			data =  std::numeric_limits<TData>::min();
			break;
			
		case parallel_utils::red_Min:
			data =  std::numeric_limits<TData>::max();
			break;
			
		default: //parallel_utils::red_Sum
			data = static_cast<TData>(0);
	}
}

template<typename TData, typename... Args>
void Serial_Functor::register_data(TData& value, Args&... args) {
	view_container.parallel_views.push_back(&value);
	register_data(args...);
}

} //namespace serial