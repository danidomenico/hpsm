#include "openmp_functor.hpp"

namespace openmp {

/*
 * Functions OpenMP_View
 */
unsigned OpenMP_View::getMatrixLineDepth() {
	unsigned line_depth = 1;
	
	if(isMatrix())
		line_depth = column_number;
		
	return line_depth;
}

void OpenMP_View::getMatrixSize(unsigned& size_i, unsigned& size_j) {
	size_i = 0;
	size_j = 0;
	
	switch(map) {
		case parallel_view::map_Vector:
			size_i = size;
			break;
			
		case parallel_view::map_Mat_Horiz:
		case parallel_view::map_Mat_Vert:
		case parallel_view::map_Mat_Vert_Horiz:
			size_i = line_number;
			size_j = column_number;
	}
}


/*
 * Functions OpenMP_Functor
 */
void OpenMP_Functor::clear_data() {
	view_container.parallel_views.clear();
}

void OpenMP_Functor::remove_data() {
	view_container.parallel_views.clear();
}

} //namespace openmp