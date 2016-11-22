#pragma once

#include "../parallel_utils.hpp"
#include "../parallel_view.hpp"

#include <limits>

namespace openmp {

//Foward declaration
struct OpenMP_Functor;

struct OpenMP_View : parallel_view::view_t {
	uintptr_t data;
	
	unsigned getMatrixLineDepth();
	void getMatrixSize(unsigned& size_i, unsigned& size_j);
	
protected:
	template<typename TData>
	void set_vector(TData* a_data, unsigned a_size, unsigned a_block_size, 
					hpsm::AccessMode::Parallel_AccessMode a_mode);
	
	template<typename TData>
	void set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
					unsigned a_block_size, parallel_view::Parallel_Map a_map, 
					hpsm::AccessMode::Parallel_AccessMode a_mode);
};

struct OpenMP_Functor {
	parallel_view::view_container_t<OpenMP_View> view_container;
	
	uintptr_t* reduction_var_ptr;
	
	template<typename TData>
	void initialize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op);
	
	template<typename TData>
	void totalize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op, int num_threads);
	
    template<typename TData, typename... Args>
    void register_data(TData& value, Args&... args);
    void register_data(void) { }
	
	void clear_data();
	void remove_data();
};

/*
 * Templated functions OpenMP_View
 */
template<typename TData>
void OpenMP_View::set_vector(TData* a_data, unsigned a_size, unsigned a_block_size,
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	size = a_size;
	block_size = size;
	map  = parallel_view::map_Vector;
	
	data = reinterpret_cast<uintptr_t>(a_data);
}

template<typename TData>
void OpenMP_View::set_matrix(TData* a_data, unsigned a_line_number, unsigned a_column_number,
							 unsigned a_block_size, parallel_view::Parallel_Map a_map, 
							 hpsm::AccessMode::Parallel_AccessMode a_mode) {
	reset();
	
	line_number   = a_line_number;
	column_number = a_column_number;
	block_size    = a_map == parallel_view::map_Mat_Horiz ? column_number : line_number;
	map           = a_map;
	
	data = reinterpret_cast<uintptr_t>(a_data);
}

/*
 * Templated functions OpenMP_Functor
 */
template<typename TData>
void OpenMP_Functor::initialize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op) {
	switch(redux_op) {
		case parallel_utils::red_Mult:
			data = static_cast<TData>(1);
			break;
			
		case parallel_utils::red_Max:
			data = std::numeric_limits<TData>::min();
			break;
			
		case parallel_utils::red_Min:
			data = std::numeric_limits<TData>::max();
			break;
			
		default: //parallel_utils::red_Sum
			data = static_cast<TData>(0);
	}
}

template<typename TData>
void OpenMP_Functor::totalize_reduction_var(TData& data, parallel_utils::Parallel_Redux redux_op, int num_threads) {
	for(int i=0; i<num_threads; i++) {
		TData aux = *(reinterpret_cast<TData*>(reduction_var_ptr[i]));
		switch(redux_op) {
			case parallel_utils::red_Mult:
				data *= aux;
				break;
				
			case parallel_utils::red_Max:
				if(aux > data)
					data = aux;
				break;
				
			case parallel_utils::red_Min:
				if(aux < data)
					data = aux;
				break;
				
			default: //parallel_utils::red_Sum
				data += aux;
		}
	}
}

template<typename TData, typename... Args>
void OpenMP_Functor::register_data(TData& value, Args&... args) {
	view_container.parallel_views.push_back(&value);
	register_data(args...);
}

} //namespace openmp