#pragma once

#include "parallel_atomic.hpp"
#include "parallel_macros.hpp"
#include "parallel_utils.hpp"
#include "parallel_view.hpp"

namespace parallel_runtime {

/*
 * Templated Functions
 */
template<typename TFunctor>
bool checkParallelFor_1D(TFunctor& obj_func, parallel_utils::block_range block_rg, 
						 parallel_utils::interval interval_i, bool check_block = true) {
	//DEBUG - Print matrices block info
	//obj_func.view_container.printBlockInfo();
	
	//Verify registered handles
	if(obj_func.view_container.parallel_views.size() <= 0) {
		parallel_utils::print_message(
			"There aren't available data! It's required to create and register at least a View before call parallel_<...> function.");
		return false;
	}
	
	//Verify map - for 1D parallel_for, it is allowed just vector structures
	if(obj_func.view_container.getMap(true) != parallel_view::map_Vector) {
		parallel_utils::print_message(
			"For 1 dimension parallel_<...> function, it's not allowed to add matrices!");
		return false;
	}
	
	//Verify block number multiplicity - It's required a compatible number of blocks
	if(check_block)
		if(! obj_func.view_container.hasValidBlockNumber()) {
			parallel_utils::print_message(
				"The structures block number must be equal/greater or divisable by range block view!");
			return false;
		}
	
	//Verify range limits
	unsigned size_line, size_column; 
	obj_func.view_container.getTotalSize(size_line, size_column);
	
	if(interval_i.end > size_line) {
		parallel_utils::print_message(
			"The defined range is greater than the block range view size!");
		return false;
	}
	
	if(check_block) {
		if((interval_i.start % block_rg.block_size) > 0) {
			parallel_utils::print_message(
				"The defined start range is not a multiple of block range view size!");
			return false;
		}
		
		if((interval_i.end % block_rg.block_size) > 0) {
			parallel_utils::print_message(
				"The defined end range is not a multiple of block range view size!");
			return false;
		}
	}
	
	return true;
}

template<typename TFunctor>
bool checkParallelFor_2D(TFunctor& obj_func, parallel_utils::block_range block_rg, 
						 parallel_utils::interval interval_i, parallel_utils::interval interval_j,
						 bool check_block = true) {
	
	//DEBUG - Print matrices block info
	//obj_func.view_container.printBlockInfo();
	
	//Verify registered handles
	if(obj_func.view_container.parallel_views.size() <= 0) {
		parallel_utils::print_message(
			"There aren't available data! It's required to create and register at least a View before call parallel_<...> function.");
		return false;
	}
	
	//Verify if block_rg is a matrix
	parallel_view::view_t& view = obj_func.view_container.getBlockView();
	if(! view.isMatrix()) {
		parallel_utils::print_message(
			"For 2 dimensions parallel_<...> function, the block range view must be a matrix!");
		return false;
	}
	
	//Verify map - It's required the same matrix map 
	if(! obj_func.view_container.hasValidMatrixMap()) {
		parallel_utils::print_message(
			"It is required to add at least one matrix and the matrix map must be the same for all structures!");
		return false;
	}
	
	//Verify block number multiplicity - It's required a compatible number of blocks
	if(check_block)
		if(! obj_func.view_container.hasValidBlockNumber()) {
			parallel_utils::print_message(
				"The structures block number must be equal/greater or divisable by range block view!");
			return false;
		}
	
	//Verify range limits
	unsigned size_line, size_column; 
	obj_func.view_container.getTotalSize(size_line, size_column);
	
	auto check_max_size = [=] (unsigned value, unsigned value_max, 
							   const char* label, const char* label2) -> bool {
		if(value > value_max) {
			char msg[100] = "";
			std::sprintf(msg, "The defined %s dimension range is greater than the block range view %s number!", 
						 label, label2);
			parallel_utils::print_message(msg);
			return false;
		}
		
		return true;
	};
	
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	
	switch(map) {
		case parallel_view::map_Mat_Horiz:
			if((!check_block) &&
			   (! check_max_size(interval_i.end, size_line, "1st", "line")))
				return false;
			if(! check_max_size(interval_j.end, size_column, "2nd", "column"))
				return false;
			break;
			
		case parallel_view::map_Mat_Vert:
			if(! check_max_size(interval_i.end, size_line, "1st", "line"))
				return false;
			if((!check_block) &&
			   (! check_max_size(interval_j.end, size_column, "2nd", "column")))
				return false;
			break;
		
		case parallel_view::map_Mat_Vert_Horiz:
			if(! check_max_size(interval_i.end, size_line, "1st", "line"))
				return false;
			if(! check_max_size(interval_j.end, size_column, "2nd", "column"))
				return false;
			break;
	}
	
	if(check_block) {
		auto check_bs = [=] (parallel_utils::interval interv, const char* label) -> bool {
			char msg[100] = "";
			
			if((interv.start % block_rg.block_size) > 0) {
				std::sprintf(msg, "The defined %s dimension start range is not a multiple of structures block size!", label);
				parallel_utils::print_message(msg);
				return false;
			}
			
			if((interv.end % block_rg.block_size) > 0) {
				std::sprintf(msg, "The defined %s dimension end range is not a multiple of structures block size!", label);
				parallel_utils::print_message(msg);
				return false;
			}

			return true;
		};
		
		switch(map) {
			case parallel_view::map_Mat_Horiz:
				if(! check_bs(interval_j, "2nd"))
					return false;
				break;
				
			case parallel_view::map_Mat_Vert:
				if(! check_bs(interval_i, "1st"))
					return false;
				break;
				
			case parallel_view::map_Mat_Vert_Horiz:
				if(! check_bs(interval_i, "1st"))
					return false;
				if(! check_bs(interval_j, "2nd"))
					return false;
				break;
		}
	}
	
	return true;
}

template<typename TFunctor>
void getMatrixStartEndBlock(TFunctor& obj_func, parallel_utils::block_range block_rg,
							parallel_utils::interval interval_i, parallel_utils::interval interval_j, 
							unsigned& block_start, unsigned& block_end, 
							unsigned& block_start_j, unsigned& block_end_j) {
	
	block_start = block_end = block_start_j = block_end_j = 0;
	parallel_view::Parallel_Map map = obj_func.view_container.getMap(true);
	
	switch(map) {
		case parallel_view::map_Mat_Horiz:
			block_start = interval_j.start / block_rg.block_size;
			block_end   = interval_j.end   / block_rg.block_size;
			break;
			
		case parallel_view::map_Mat_Vert:
			block_start = interval_i.start / block_rg.block_size;
			block_end   = interval_i.end   / block_rg.block_size;
			break;
		
		case parallel_view::map_Mat_Vert_Horiz:
			block_start = interval_i.start / block_rg.block_size;
			block_end   = interval_i.end   / block_rg.block_size;
			
			block_start_j = interval_j.start / block_rg.block_size;
			block_end_j   = interval_j.end   / block_rg.block_size;
			break;
	}
}
	
} //namespace parallel_runtime

namespace hpsm {

template<typename TData>
PARALLEL_FUNCTION
void atomic_add(TData* dest, TData value) {
	#if defined(__CUDA_ARCH__)
		parallel_atomic::atomic_add(dest, value);
	#else
		*dest = *dest + value;
	#endif
}

template<typename TData>
PARALLEL_FUNCTION
void atomic_multi(TData* dest, TData value) {
	#if defined(__CUDA_ARCH__)
		parallel_atomic::atomic_multi(dest, value);
	#else
		*dest = (*dest) * value;
	#endif
}

template<typename TData>
PARALLEL_FUNCTION
void atomic_max(TData* dest, TData value) {
	#if defined(__CUDA_ARCH__)
		parallel_atomic::atomic_max(dest, value);
	#else
		if(*dest < value)
			*dest = value;
	#endif
}

template<typename TData>
PARALLEL_FUNCTION
void atomic_min(TData* dest, TData value) {
	#if defined(__CUDA_ARCH__)
		parallel_atomic::atomic_min(dest, value);
	#else
		if(*dest > value)
			*dest = value;
	#endif
}
	
} //namespace hpsm
