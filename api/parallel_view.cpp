#include "parallel_view.hpp"

namespace parallel_view {

/*
 * Functions Parallel_View
 */
parallel_utils::block_range Parallel_View::block_range() {
	if(isVector())
		return parallel_utils::block_range(view_id, size, block_size);
	else //isMatrix
		return parallel_utils::block_range(view_id, line_number, column_number, block_size);
}

unsigned Parallel_View::getIdxBlockPartition(unsigned idx_block, unsigned block_number_exec,
											 parallel_utils::Parallel_BlockTile block_tile, bool is_idx_j) {
	int proportion, b_number;
	if(map == map_Mat_Vert_Horiz) {
		if(is_idx_j) {
			if(block_number_column < block_number_exec) {
				proportion = block_number_exec/block_number_column;
				b_number   = block_number_column;
			} else //block_number_column >= block_number_exec
				return idx_block;
				
		} else {
			if(block_number_line < block_number_exec) {
				proportion = block_number_exec/block_number_line;
				b_number   = block_number_line; 
			} else //block_number_line >= block_number_exec
				return idx_block;
		}
	} else { //map_Vector, map_Mat_Vert, map_Mat_Horiz
		
		if(block_number < block_number_exec) {
			proportion = block_number_exec/block_number;
			b_number   = block_number;
		} else //block_number >= block_number_exec
			return idx_block;
	}
	
	switch(block_tile) {
		case parallel_utils::bt_Intercalary :
			return idx_block/proportion;
			
		case parallel_utils::bt_Sequentially :
			if(proportion == 1)
				return idx_block;
			else
				return idx_block % b_number;
	}
	
	return idx_block;
}

unsigned Parallel_View::getMatrixOppositeSize() {
	switch(map) {
		case map_Mat_Horiz:
			return line_number;
			
		case map_Mat_Vert:
			return column_number;
		
		case map_Mat_Vert_Horiz: //See if block_size is the correct field
			return block_size;
	}
	
	//Case it isn't a matrix, it is a map_Vector. Thus, return 0
	return 0;
}

bool Parallel_View::isVector() {
	if(map == map_Vector)
		return true;
	
	return false;
}

bool Parallel_View::isMatrix() {
	if(map == map_Mat_Horiz ||
	   map == map_Mat_Vert ||
	   map == map_Mat_Vert_Horiz)
		return true;
	
	return false;
}

void Parallel_View::printBlockInfo() {
	std::cout << "view_id:" << view_id
		<< " | map:" << parallel_Map_Str[map]
		<< " | block_size:" << block_size
		<< " | block_number:" << block_number
		<< " | block_number_line:" << block_number_line
		<< " | block_number_column:" << block_number_column
		<< " | matrix_opposite_size:" << getMatrixOppositeSize()
		<< std::endl;
}

void Parallel_View::reset() {
	size                = 0u;
	line_number         = 0u;
	column_number       = 0u;
	block_size          = 0u;
	block_number        = 0u;
	block_number_line   = 0u;
	block_number_column = 0u;
}

void Parallel_View::setBlockNumber() {
	
	//Validate whether block_size is greater than 0
	if(block_size == 0) {
		parallel_utils::print_message("The view block_size must be greater than 0!");
		std::abort();
	}
	
	//Calculate number of blocks
	auto validate = [] (unsigned number, unsigned blc_size, const char label[]) {
		if(number % blc_size != 0) {
			char msg[100] = "";
			std::strcat(msg, label);
			std::strcat(msg, "/block_size must be an exact division!");
			parallel_utils::print_message(msg);
			std::abort();
		}
	};
	
	switch(map) {
		case map_Vector:
			validate(size, block_size, "size");
			block_number = size/block_size;
			break;
		
		case map_Mat_Horiz:
			validate(column_number, block_size, "column_number");
			block_number = column_number/block_size;
			break;
			
		case map_Mat_Vert:
			validate(line_number, block_size, "line_number");
			block_number = line_number/block_size;
			break;
			
		case map_Mat_Vert_Horiz:
			validate(line_number, block_size, "line_number");
			validate(column_number, block_size, "column_number");
			block_number_line   = line_number/block_size;
			block_number_column = column_number/block_size;
			block_number        = block_number_line * block_number_column;
			break;
	}
}

} //namespace parallel_view
