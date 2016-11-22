#pragma once

#include "starpu.h"

#include "starpu_cpu.hpp"
#include "starpu_cuda.hpp"
#include "starpu_functor.hpp"
#include "../parallel_utils.hpp"

namespace starpu {

template<typename TFunc>
void submit_task(TFunc& func, starpu_codelet& cl, bool reduce, std::vector<StarPU_Args<TFunc>*>& lst_args,
				 int i, int j=-1) {
	StarPU_Functor& obj_func = static_cast<StarPU_Functor&>(func);
	
	//Defining task
	starpu_task* task = starpu_task_create();
	task->cl = &cl;
	
	//Test disable implicit dependencies
	//task->sequential_consistency = 0;
	//task->use_tag = 1;
	//task->tag_id = i;
	
	unsigned bl_line, bl_colunmn;
	if(j > -1) //Indicates parallel_view::map_Mat_Vert_Horiz
		obj_func.view_container.getBlockNumberBlockView_MatVerHoriz(bl_line, bl_colunmn);
	
	for(int k=0; k<obj_func.view_container.parallel_views.size(); k++) {
		starpu_data_handle_t sub_handle;
		StarPU_View& view = *(obj_func.view_container.parallel_views[k]);
		
		if(view.map == parallel_view::map_Mat_Vert_Horiz) {
			unsigned idx_i = 
				view.getIdxBlockPartition(i, bl_line, obj_func.block_tile);
			unsigned idx_j = 
				view.getIdxBlockPartition(j, bl_colunmn, obj_func.block_tile, true);
			
			sub_handle = starpu_data_get_sub_data(view.handle, 2, idx_i, idx_j);
			
		} else { //parallel_view::map_Vector
			int idx_block = i;
			if(j > -1) //Indicates parallel_view::map_Mat_Vert_Horiz
				idx_block = i * bl_colunmn + j;
			
			unsigned block_number = obj_func.view_container.getBlockNumberBlockView();
			sub_handle = starpu_data_get_sub_data(view.handle, 1, 
												  view.getIdxBlockPartition(idx_block, block_number, obj_func.block_tile));
		}
		
		task->handles[k] = sub_handle;
	}
	
	if(reduce)
		task->handles[obj_func.view_container.parallel_views.size()] = obj_func.reduction_var.handle;

	//Generate args
	int idx_block = i;
	if(j > -1) //Indicates parallel_view::map_Mat_Vert_Horiz
		idx_block = i * bl_colunmn + j;
	
	parallel_view::view_t& view = obj_func.view_container.getBlockView();
	int i_block = view.map == parallel_view::map_Mat_Horiz ? 0 : i;
	int j_block = j;
	if(view.map == parallel_view::map_Mat_Horiz)
		j_block = i;
	else if(view.map == parallel_view::map_Mat_Vert)
		j_block = 0;
	
	StarPU_Args<TFunc>* args = new StarPU_Args<TFunc>(func, idx_block, i_block, j_block);
	void* args_addr   = reinterpret_cast<void*>(args);
	task->cl_arg      = args_addr;
	task->cl_arg_size = sizeof(StarPU_Args<TFunc>);
	
	lst_args.push_back(args);
	
	//Submit task
	int err_ret = starpu_task_submit(task);
	if(err_ret == -ENODEV) {
		char msg[100] = "";
		std::sprintf(msg, "Error submitting a StarPU task. Error code: %d", err_ret);
		parallel_utils::print_message(msg);
		std::abort();
	}
}

	
} //namespace starpu