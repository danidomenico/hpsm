#pragma once

#include "parallel_const.hpp"
#include "parallel_utils.hpp"

#include <iostream>
#include <vector>

namespace parallel_view {

/* 
 * Type Alias
 */
using Parallel_Map = hpsm::PartitionMode::Parallel_PartitionMode;

static const Parallel_Map map_Vector         = hpsm::PartitionMode::Vector;
static const Parallel_Map map_Mat_Horiz      = hpsm::PartitionMode::Matrix_Horiz;
static const Parallel_Map map_Mat_Vert       = hpsm::PartitionMode::Matrix_Vert;
static const Parallel_Map map_Mat_Vert_Horiz = hpsm::PartitionMode::Matrix_Vert_Horiz;

/*
 * Types
 */
struct Parallel_Data {
	Parallel_Data() {};
	
	PARALLEL_FUNCTION
	Parallel_Data(uintptr_t data_, unsigned size_i_) :
		data(data_), size_i(size_i_) {};
		
	PARALLEL_FUNCTION
	Parallel_Data(uintptr_t data_, unsigned size_i_, unsigned size_j_, unsigned line_depth_) :
		data(data_), size_i(size_i_), size_j(size_j_), line_depth(line_depth_) {};
	
	template<typename TData>
	PARALLEL_FUNCTION
	TData& get(int i) {
		return (reinterpret_cast<TData*>(data))[i];
	}
	
	template<typename TData>
	PARALLEL_FUNCTION
	TData& get(int i, int j) {
		return (reinterpret_cast<TData*>(data))[j+i*line_depth];
	}
	
	template<typename TData>
	PARALLEL_FUNCTION
	TData& get(parallel_utils::index<1> idx) {
		return get<TData>(idx(0));
	}
	
	template<typename TData>
	PARALLEL_FUNCTION
	TData& get(parallel_utils::index<2> idx) {
		return get<TData>(idx(0), idx(1));
	}
	
	PARALLEL_FUNCTION
	unsigned size(int dimension) {
		if(dimension == 0)
			return size_i;
		else if(dimension == 1)
			return size_j;
		
		return -1u;
	}

private:
	uintptr_t data;
	unsigned line_depth = 1u;
	unsigned size_i     = 0u;
	unsigned size_j     = 0u;
};
using data_t = Parallel_Data;

struct Parallel_View : parallel_utils::unique_id {
	
	
	const unsigned view_id = ID;
	
	Parallel_Map map;
	unsigned size;
	unsigned line_number;
	unsigned column_number;
	unsigned block_size;
	unsigned block_number;
	unsigned block_number_line;
	unsigned block_number_column;
	
	Parallel_Data data_block;
	
	parallel_utils::block_range block_range();
	unsigned getIdxBlockPartition(unsigned idx_block, unsigned block_number_exec,
								  parallel_utils::Parallel_BlockTile block_tile, bool is_idx_j = false);
	unsigned getMatrixOppositeSize();
	bool isVector();
	bool isMatrix();
	void printBlockInfo();
	
protected:
	void reset();
	void setBlockNumber();
};
using view_t = Parallel_View;

template<typename TBackend>
struct Parallel_View_Container {
	std::vector<TBackend*> parallel_views;
	
	Parallel_Map getMap(bool is_matrix);
	unsigned getBlockNumberBlockView();
	void getBlockNumberBlockView_MatVerHoriz(unsigned& bn_line, unsigned& bn_column);
	void getBlockNumberBlockView_Mat_Exec(unsigned& bn_total, unsigned& bn_line, unsigned& bn_column);
	void getTotalSize(unsigned& size_line, unsigned& size_column);
	
	bool hasMatrix();
	bool hasValidBlockNumber();
	bool hasValidMatrixMap();
	
	void printBlockInfo();
	
	void setBlockView(unsigned view_id);
	view_t& getBlockView();
	unsigned getBlockViewIdx();

private:
	int block_view_idx = -1;
};
template<typename TBackend>
using view_container_t = Parallel_View_Container<TBackend>;

/*
 * Templated functions Parallel_View_Container 
 */
template<typename TBackend>
Parallel_Map Parallel_View_Container<TBackend>::getMap(bool is_matrix) {
	if(is_matrix) {
		for(view_t* view : parallel_views) {
			if(view->isMatrix())
				//It's possible to get map from the first view because all matrices have the same map
				return view->map;
		}
	}
	
	return map_Vector;
}

template<typename TBackend>
unsigned Parallel_View_Container<TBackend>::getBlockNumberBlockView() {
	return getBlockView().block_number;
}

template<typename TBackend>
void Parallel_View_Container<TBackend>::getBlockNumberBlockView_MatVerHoriz(unsigned& bn_line, unsigned& bn_column) {
	bn_line   = 0;
	bn_column = 0;
	
	if(getMap(true) != map_Mat_Vert_Horiz)
		return;
	
	bn_line   = getBlockView().block_number_line;
	bn_column = getBlockView().block_number_column;
}

template<typename TBackend>
void Parallel_View_Container<TBackend>::getBlockNumberBlockView_Mat_Exec(unsigned& bn_total, 
																		 unsigned& bn_line, unsigned& bn_column) {
	view_t& view = getBlockView();
	
	bn_total = view.block_number;
	switch(view.map) {
		case map_Mat_Horiz:
			bn_line   = 1;
			bn_column = bn_total;
			break;
			
		case map_Mat_Vert:
			bn_line   = bn_total;
			bn_column = 1;
			break;
			
		case map_Mat_Vert_Horiz:
			bn_line   = view.block_number_line;
			bn_column = view.block_number_column;
			break;
	}
}

template<typename TBackend>
void Parallel_View_Container<TBackend>::getTotalSize(unsigned& size_line, unsigned& size_column) {
	size_line   = 0;
	size_column = 0;
	
	view_t& view = getBlockView();
	
	switch(view.map) {
		case map_Vector:
			size_line = view.size;
			break;
			
		case map_Mat_Horiz:
		case map_Mat_Vert:
		case map_Mat_Vert_Horiz: 
			size_line   = view.line_number;
			size_column = view.column_number;
			break;
	}
}

template<typename TBackend>
bool Parallel_View_Container<TBackend>::hasMatrix() {
	for(view_t* view : parallel_views) {
		if(view->isMatrix())
			return true;
	}
	
	return false;
}

template<typename TBackend>
bool Parallel_View_Container<TBackend>::hasValidBlockNumber() {
	unsigned block_number = getBlockNumberBlockView();
	
	if(getBlockView().map == map_Mat_Vert_Horiz) {
		unsigned bn_line, bn_colunmn;
		getBlockNumberBlockView_MatVerHoriz(bn_line, bn_colunmn);
		
		for(view_t* view : parallel_views) {
			if(view->map == map_Mat_Vert_Horiz) {
				if(view->block_number_line < bn_line &&
				   bn_line % view->block_number_line != 0)
					return false;
				
				if(view->block_number_column < bn_colunmn &&
				   bn_colunmn % view->block_number_column != 0)
					return false;
			
			} else { //map_Vector
				if(view->block_number >= block_number)
					continue;
				else if(block_number % view->block_number != 0)
					return false;
			}
		}
		
	} else {
		for(view_t* view : parallel_views) {
			if(view->block_number >= block_number)
				continue;
			else if(block_number % view->block_number != 0)
				return false;
		}
	}
	
	return true;
}

template<typename TBackend>
bool Parallel_View_Container<TBackend>::hasValidMatrixMap() {
	//Check if there are some matrix
	if(! hasMatrix())
		return false;
	
	Parallel_Map map = map_Vector;
	//Search for a matrix map
	for(view_t* view : parallel_views) {
		if(view->isMatrix()) {
			map = view->map;
			break;
		}
	}
	
	//Check if there are differents matrix maps
	for(view_t* view : parallel_views) {
		if(view->map != map_Vector && 
		   view->map != map)
			return false;
	}
	
	return true;
}

template<typename TBackend>
void Parallel_View_Container<TBackend>::printBlockInfo() {
	std::cout << "Registered Views:\n";
	for(view_t* view : parallel_views)
		view->printBlockInfo();
}

template<typename TBackend>
void Parallel_View_Container<TBackend>::setBlockView(unsigned view_id) {
	int i = 0;
	for(view_t* view : parallel_views) {
		if(view->view_id == view_id) {
			block_view_idx = i;
			return;
		}
		
		i++;
	}
	
	char msg[100] = "";
	std::sprintf(msg, "View id=%d was not registered for the functor!", view_id);
	parallel_utils::print_message(msg);
	std::abort();
}

template<typename TBackend>
view_t& Parallel_View_Container<TBackend>::getBlockView() {
	if(block_view_idx >= 0) {
		TBackend* view = parallel_views[block_view_idx];
		return *view;
	}
	
	parallel_utils::print_message("Block view was not registered for the functor!");
	std::abort();
}

template<typename TBackend>
unsigned Parallel_View_Container<TBackend>::getBlockViewIdx() {
	if(block_view_idx >= 0) 
		return block_view_idx;
	
	parallel_utils::print_message("Block view was not registered for the functor!");
	std::abort();
}

} //namespace parallel_view
