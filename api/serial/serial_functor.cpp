#include "serial_functor.hpp"

namespace serial {

/*
 * Functions Serial_View
 */
uintptr_t Serial_View::getDataBlock(unsigned idx_block, unsigned block_number_exec,
									parallel_utils::Parallel_BlockTile block_tile) {
	unsigned idx_partition = getIdxBlockPartition(idx_block, block_number_exec, block_tile);
	
	uintptr_t data_ret;
	
	switch(map) {
		case parallel_view::map_Vector:
		case parallel_view::map_Mat_Horiz:
			data_ret = data + (data_size * (idx_partition * block_size));
			break;
		
		case parallel_view::map_Mat_Vert:
			data_ret = data + (data_size * (idx_partition * (block_size * column_number)));
			break;
	}
	
	return data_ret;
}

uintptr_t Serial_View::getDataBlock_MatVerHoriz(unsigned idx_block_line, unsigned idx_block_column,
												Serial_Functor& obj_func) {
	
	uintptr_t data_ret;
	
	if(map == parallel_view::map_Mat_Vert_Horiz) {
		unsigned bl_line, bl_colunmn;
		obj_func.view_container.getBlockNumberBlockView_MatVerHoriz(bl_line, bl_colunmn);
				
		unsigned idx_i = getIdxBlockPartition(idx_block_line, bl_line, obj_func.block_tile);
		unsigned idx_j = getIdxBlockPartition(idx_block_column, bl_colunmn, obj_func.block_tile, true);
		
		unsigned pos = (idx_i * (column_number * block_size)) + 
					   (idx_j * block_size);
			
		data_ret = data + (data_size * pos);
	}
	
	return data_ret;
}

unsigned Serial_View::getMatrixLineDepth() {
	unsigned line_depth = 1;
	
	if(isMatrix())
		line_depth = column_number;
		
	return line_depth;
}

void Serial_View::getMatrixBlockSize(unsigned& size_i, unsigned& size_j) {
	size_i = 0;
	size_j = 0;
	
	switch(map) {
		case parallel_view::map_Vector:
			size_i = block_size;
			break;
			
		case parallel_view::map_Mat_Horiz:
			size_i = line_number;
			size_j = block_size;
			break;
			
		case parallel_view::map_Mat_Vert:
			size_i = block_size;
			size_j = column_number;
			break;
			
		case parallel_view::map_Mat_Vert_Horiz:
			size_i = block_size;
			size_j = block_size;
			break;
	}
}

/*
 * Functions Serial_Functor
 */
void Serial_Functor::clear_data() {
	view_container.parallel_views.clear();
}

void Serial_Functor::remove_data() {
	view_container.parallel_views.clear();
}

} //namespace serial