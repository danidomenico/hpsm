#pragma once

#include "starpu_functor.hpp"

namespace starpu {
	
/*
 * Codelets Functions CPU
 */
template<typename TFunc>
void cpu_impl_1D(void *buffers[], void *cl_arg) {
	//printf("CPU 1D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	unsigned int n         = obj_func.view_container.getBlockView().block_size;
	unsigned int block_qtd = obj_func.view_container.getBlockView().block_number;
	
	for(int i=0; i<obj_func.view_container.parallel_views.size(); i++) {
		parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]), STARPU_VECTOR_GET_NX(buffers[i]));
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	
#if defined(_OPENMP)
	//std::cout << "Threads: " << starpu_combined_worker_get_size() << "\n";
	#pragma omp parallel for schedule(static) num_threads(starpu_combined_worker_get_size())
#endif
	for(int i = 0; i < n; i++) {
		parallel_utils::index<1> idx(i, args.idx_block, block_qtd, n);
		func(idx);
	}
}

template<typename TFunc>
void cpu_impl_2D(void *buffers[], void *cl_arg) {
	//printf("CPU 2D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	/* length of the matrix block */
	unsigned idx_handle = obj_func.view_container.getBlockViewIdx();
	unsigned nx = STARPU_MATRIX_GET_NX(buffers[idx_handle]);
	unsigned ny = STARPU_MATRIX_GET_NY(buffers[idx_handle]);
	unsigned block_qtd, block_qtd_i, block_qtd_j;
	obj_func.view_container.getBlockNumberBlockView_Mat_Exec(block_qtd, block_qtd_i, block_qtd_j);
	
	//printf("idx_handle=%u nx=%u, ny=%u\n", idx_handle, nx, ny);
	//DEBUG - Print matrices block info
	//obj_func.view_container.printBlockInfo();
	for(int i=0; i<obj_func.view_container.parallel_views.size(); i++) {
		StarPU_View& view = *(obj_func.view_container.parallel_views[i]);
		
		if(view.isMatrix()) {
			parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]),
											 STARPU_MATRIX_GET_NY(buffers[i]),
											 STARPU_MATRIX_GET_NX(buffers[i]),
										     STARPU_MATRIX_GET_LD(buffers[i]));
			view.data_block = data_block;
		} else {
			parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]),
											 STARPU_MATRIX_GET_NX(buffers[i]));
			view.data_block = data_block;
		}
	}

#if defined(_OPENMP)
	//std::cout << "2D Threads: " << starpu_combined_worker_get_size() << "\n";
	#pragma omp parallel for schedule(static) num_threads(starpu_combined_worker_get_size())
#endif
	for(int i = 0; i < ny; i++) {
		for(int j = 0; j < nx; j++) {
			parallel_utils::index<2> idx(i, j, 
										 args.idx_block, args.idx_block_i, args.idx_block_j, 
										 block_qtd, block_qtd_i, block_qtd_j,
										 ny, nx);
			func(idx);
		}
	}
}

template<typename TFunc>
void cpu_impl_reduction_1D(void *buffers[], void *cl_arg) {
	//printf("CPU reduction 1D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	unsigned int n         = obj_func.view_container.getBlockView().block_size;
	unsigned int block_qtd = obj_func.view_container.getBlockView().block_number;
	int size               = obj_func.view_container.parallel_views.size();
	
	for(int i=0; i<size; i++) {
		parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]), STARPU_VECTOR_GET_NX(buffers[i]));
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	obj_func.reduction_var_ptr = STARPU_VARIABLE_GET_PTR(buffers[size]);
	
	for(int i = 0; i < n; i++) {
		parallel_utils::index<1> idx(i, args.idx_block, block_qtd, n);
		func(idx);
	}
}

template<typename TFunc>
void cpu_impl_reduction_2D(void *buffers[], void *cl_arg) {
	//printf("CPU reduction 2D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	/* length of the matrix block */
	unsigned idx_handle = obj_func.view_container.getBlockViewIdx();
	unsigned int nx = STARPU_MATRIX_GET_NX(buffers[idx_handle]);
	unsigned int ny = STARPU_MATRIX_GET_NY(buffers[idx_handle]);
	int size        = obj_func.view_container.parallel_views.size();
	unsigned block_qtd, block_qtd_i, block_qtd_j;
	obj_func.view_container.getBlockNumberBlockView_Mat_Exec(block_qtd, block_qtd_i, block_qtd_j);
	
	//printf("idx_handle=%u nx=%u, ny=%u\n", idx_handle, nx, ny);
	
	for(int i=0; i<size; i++) {
		StarPU_View& view = *(obj_func.view_container.parallel_views[i]);
		
		if(view.isMatrix()) {
			parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]),
											 STARPU_MATRIX_GET_NY(buffers[i]),
											 STARPU_MATRIX_GET_NX(buffers[i]),
										     STARPU_MATRIX_GET_LD(buffers[i]));
			view.data_block = data_block;
		} else {
			parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]),
											 STARPU_MATRIX_GET_NX(buffers[i]));
			view.data_block = data_block;
		}
	}
	obj_func.reduction_var_ptr = STARPU_VARIABLE_GET_PTR(buffers[size]);
	
	for(int i = 0; i < ny; i++) {
		for(int j = 0; j < nx; j++) {
			parallel_utils::index<2> idx(i, j, 
										 args.idx_block, args.idx_block_i, args.idx_block_j,
										 block_qtd, block_qtd_i, block_qtd_j,
										 ny, nx);
			func(idx);
		}
	}
}

/*
 * Reduction codelet functions
 */
template<typename TData>
void cpu_impl_reduction_init(void *buffers[], void *cl_arg) {
	//printf("CPU redux init\n");
	
	TData* var = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[0]));
	
	switch(gb_redux_op) {
		case parallel_utils::red_Mult:
			*var = static_cast<TData>(1);
			break;
			
		case parallel_utils::red_Max:
			*var =  std::numeric_limits<TData>::min();
			break;
			
		case parallel_utils::red_Min:
			*var =  std::numeric_limits<TData>::max();
			break;
			
		default: //parallel_utils::red_Sum
			*var = static_cast<TData>(0);
	}
}

template<typename TData>
void cpu_impl_reduction_redux(void *buffers[], void *cl_arg) {
	//printf("CPU redux end\n");

	TData* var_rw = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[0]));
	TData* var_r  = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[1]));

	switch(gb_redux_op) {
		case parallel_utils::red_Mult:
			*var_rw = (*var_rw) * (*var_r);
			break;
			
		case parallel_utils::red_Max:
			*var_rw = STARPU_MAX(*var_rw, *var_r);
			break;
			
		case parallel_utils::red_Min:
			*var_rw = STARPU_MIN(*var_rw, *var_r);
			break;
			
		default: //parallel_utils::red_Sum
			*var_rw = *var_rw + *var_r;
	}
}
	
} //namespace starpu
