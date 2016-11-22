#pragma once

#include "../parallel_utils.hpp"
#include "serial_functor.hpp"
#include <cstdint>

namespace serial {

template<typename TFunc>
void submit_task_1D(TFunc& func, uintptr_t reduction_var, int i) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	
	for(unsigned k=0; k<obj_func.view_container.parallel_views.size(); k++) {
		Serial_View& view = *(obj_func.view_container.parallel_views[k]);
		
		unsigned block_number = obj_func.view_container.getBlockNumberBlockView();
		parallel_view::data_t data_block(view.getDataBlock(i, block_number, obj_func.block_tile),
										 view.block_size);
		
		obj_func.view_container.parallel_views[k]->data_block = data_block;
	}
	if(reduction_var > 0)
		obj_func.reduction_var_ptr = reduction_var;
	
	unsigned int n = obj_func.view_container.getBlockView().block_size;
	unsigned int qtd_blocks = obj_func.view_container.getBlockView().block_number;
	
	//printf("Bloco: %d\n", i);
	for(unsigned k=0; k<n; k++) {
		parallel_utils::index<1> idx(k, i, qtd_blocks, n);
		func(idx);
	}
}

template<typename TFunc>
void submit_task_2D(TFunc& func, uintptr_t reduction_var, int i, int j=-1) {
	Serial_Functor& obj_func = static_cast<Serial_Functor&>(func);
	
	for(unsigned k=0; k<obj_func.view_container.parallel_views.size(); k++) {
		Serial_View& view = *(obj_func.view_container.parallel_views[k]);
		
		unsigned size_block_i, size_block_j;
		view.getMatrixBlockSize(size_block_i, size_block_j);
		
		if(view.map == parallel_view::map_Mat_Vert_Horiz) {
			parallel_view::data_t data_block(view.getDataBlock_MatVerHoriz(i, j, obj_func),
											 size_block_i,
											 size_block_j,
											 view.getMatrixLineDepth());
			obj_func.view_container.parallel_views[k]->data_block = data_block;
			
		}else {
			int idx_block = i;
			if(j > -1) { //Indicates parallel_view::map_Mat_Vert_Horiz
				unsigned bl_line, bl_colunmn;
				obj_func.view_container.getBlockNumberBlockView_MatVerHoriz(bl_line, bl_colunmn);
				
				idx_block = i * bl_colunmn + j;
			}
			
			unsigned block_number = obj_func.view_container.getBlockNumberBlockView();
			
 			parallel_view::data_t data_block(view.getDataBlock(idx_block, block_number, obj_func.block_tile),
											 size_block_i,
											 size_block_j,
											 view.getMatrixLineDepth());
			obj_func.view_container.parallel_views[k]->data_block = data_block;
		}
	}
	if(reduction_var > 0)
		obj_func.reduction_var_ptr = reduction_var;
	
	unsigned nx, ny, idx_block, i_block, j_block;
	parallel_view::view_t& view = obj_func.view_container.getBlockView();
	unsigned block_qtd, block_qtd_i, block_qtd_j;
	obj_func.view_container.getBlockNumberBlockView_Mat_Exec(block_qtd, block_qtd_i, block_qtd_j);
	switch(view.map) {
		case parallel_view::map_Mat_Horiz:
			nx = view.block_size;
			ny = view.getMatrixOppositeSize();
			idx_block = i;
			i_block   = 0;
			j_block   = i;
			break;
		
		case parallel_view::map_Mat_Vert:
			nx = view.getMatrixOppositeSize();
			ny = view.block_size;
			idx_block = i;
			i_block   = i;
			j_block   = 0;
			break;
			
		case parallel_view::map_Mat_Vert_Horiz:
			nx = ny = view.block_size;
			
			idx_block = i * block_qtd_j + j;
			i_block   = i;
			j_block   = j;
			break;
	}
	
	//printf("Bloco: %d, %d %d\n", idx_block, i, j);
	for(int k = 0; k < ny; k++) {
		for(int l = 0; l < nx; l++) {
			parallel_utils::index<2> idx(k, l, 
										 idx_block, i_block, j_block, 
										 block_qtd, block_qtd_i, block_qtd_j, 
										 ny, nx);
			func(idx);
		}
	}
}
	
} //namespace serial