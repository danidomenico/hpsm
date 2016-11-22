#include "starpu_functor.hpp"

namespace starpu {

/*
 * Global variables
 */
parallel_utils::Parallel_Redux gb_redux_op;

/*
 * Function StarPU_View
 */
void StarPU_View::set_StarPU_mode(hpsm::AccessMode::Parallel_AccessMode a_mode) {
	switch(a_mode) {
		case hpsm::AccessMode::In :
			mode = STARPU_R; //STARPU_R, STARPU_W, STARPU_RW from "starpu.h"
			break;
			
		case hpsm::AccessMode::Out :
			mode = STARPU_W;
			break;
			
		default:
			mode = STARPU_RW;
	}
}

void StarPU_View::partition_data() {
	switch(map) {
		case parallel_view::map_Vector:
			starpu_data_filter f;
			std::memset(&f, 0, sizeof(f));
			f.filter_func = starpu_vector_filter_block;
			f.nchildren   = block_number;
			starpu_data_partition(handle, &f);
			break;
		
		case parallel_view::map_Mat_Horiz:
		case parallel_view::map_Mat_Vert:
		case parallel_view::map_Mat_Vert_Horiz:
			starpu_data_filter vert, horiz;
			if(map == parallel_view::map_Mat_Vert || map == parallel_view::map_Mat_Vert_Horiz) {
				std::memset(&vert, 0, sizeof(vert));
				vert.filter_func = starpu_matrix_filter_vertical_block;
				vert.nchildren   = block_number;
				
				if(map == parallel_view::map_Mat_Vert)
					starpu_data_partition(handle, &vert);
			} 
			if(map == parallel_view::map_Mat_Horiz || map == parallel_view::map_Mat_Vert_Horiz) {
				std::memset(&horiz, 0, sizeof(horiz));
				horiz.filter_func = starpu_matrix_filter_block;
				horiz.nchildren   = block_number;
				
				if(map == parallel_view::map_Mat_Horiz)
					starpu_data_partition(handle, &horiz);
			}
			if(map == parallel_view::map_Mat_Vert_Horiz) {
				vert.nchildren = block_number_line;
				horiz.nchildren = block_number_column;
				starpu_data_map_filters(handle, 2, &vert, &horiz);
			}
			break;
	}
}

/*
 * Functions StarPU_Red_Var
 */
void StarPU_Red_Var::unregister_variable() {
	starpu_data_unregister(handle);
}

/*
 * Functions StarPU_Functor
 */
void StarPU_Functor::clear_data() {
	view_container.parallel_views.clear();
}

void StarPU_Functor::remove_data() {
	for(StarPU_View* starPU_view : view_container.parallel_views) {
		starpu_data_unpartition(starPU_view->handle, STARPU_MAIN_RAM);
		starpu_data_unregister(starPU_view->handle);
	}
	
	view_container.parallel_views.clear();
}

} //namespace starpu