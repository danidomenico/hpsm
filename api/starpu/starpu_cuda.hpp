#pragma once

#include "starpu_functor.hpp"

namespace starpu {
	
/*
 * Codelets Functions CUDA
 */
const unsigned GPU_WARP_SIZE      = 32;
const unsigned GPU_MAX_WARP_COUNT = 8;  //All compute capabilities support at least 16 warps (512 threads).
                                        //However, we have found that 8 warps typically gives better performance.
                                        //GPU_MAX_WARP_COUNT = cudaProp.maxThreadsPerBlock / GPU_WARP_SIZE;

/*
 * Kernels
 */
template<typename TFunc> 
static __global__ void kernel_cuda_1D(int size, int idx_block, int block_qtd, TFunc func) {
	const unsigned stride = blockDim.y * gridDim.x;
	const unsigned start  = threadIdx.y + blockDim.y * blockIdx.x;
	
	for(unsigned i=start; i<size; i+=stride) {
		parallel_utils::index<1> idx(i, idx_block, block_qtd, size);
		func(idx);
	}
}

template<typename TFunc> 
static __global__ void kernel_cuda_2D(int sizeX, int sizeY, int line_depth,
									  int idx_block, int idx_block_i, int idx_block_j,
									  int block_qtd, int block_qtd_i, int block_qtd_j,
									  TFunc func) {
	const unsigned stride     = blockDim.y * gridDim.x;
	const unsigned start      = threadIdx.y + blockDim.y * blockIdx.x;
	const unsigned total_size = sizeX * sizeY;
	
	for(unsigned k=start; k<total_size; k+=stride) {
		unsigned i = k / line_depth;
		unsigned j = k % line_depth;
		//printf("ld=%d k=%d i=%d j=%d start=%d stride=%d total_size=%d\n", 
		//	line_depth, k, i, j, start, stride, total_size);
		
		parallel_utils::index<2> idx(i, j, 
									 idx_block, idx_block_i, idx_block_j, 
									 block_qtd, block_qtd_i, block_qtd_j,
									 sizeY, sizeX);
		func(idx);
	}
}

template<typename TData>
static __global__ void kernel_cuda_reduction_redux(TData* var_rw, TData* var_r, parallel_utils::Parallel_Redux redux_op) {
	switch(redux_op) {
		case parallel_utils::red_Mult:
			*var_rw = (*var_rw) * (*var_r);
			break;
			
		case parallel_utils::red_Max:
			if(*var_rw < *var_r)
				*var_rw = *var_r;
			break;
			
		case parallel_utils::red_Min:
			if(*var_rw > *var_r)
				*var_rw = *var_r;
			break;
			
		default: //parallel_utils::red_Sum
			*var_rw = *var_rw + *var_r;
	}
}

/*
 * Codelets Functions CUDA
 */
template<typename TFunc>
void cuda_impl_1D(void *buffers[], void *cl_arg) {
	//printf("CUDA 1D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	unsigned n         = obj_func.view_container.getBlockView().block_size;
	unsigned block_qtd = obj_func.view_container.getBlockView().block_number;
	
	for(int i=0; i<obj_func.view_container.parallel_views.size(); i++) {
		parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]), STARPU_VECTOR_GET_NX(buffers[i]));
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	
	const dim3 threads(1, GPU_WARP_SIZE * GPU_MAX_WARP_COUNT, 1);
    const dim3 blocks (std::min((n + threads.y - 1 ) / threads.y, obj_func.gpu_max_blocks), 1, 1);
	
	kernel_cuda_1D<<<blocks, threads, 0, starpu_cuda_get_local_stream()>>>(n, args.idx_block, block_qtd, func);
	cudaStreamSynchronize(starpu_cuda_get_local_stream());
}

template<typename TFunc>
void cuda_impl_2D(void *buffers[], void *cl_arg) {
	//printf("CUDA 2D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	/* length of the matrix block */
	unsigned idx_handle = obj_func.view_container.getBlockViewIdx();
	unsigned nx         = STARPU_MATRIX_GET_NX(buffers[idx_handle]);
	unsigned ny         = STARPU_MATRIX_GET_NY(buffers[idx_handle]);
	unsigned ld         = STARPU_MATRIX_GET_LD(buffers[idx_handle]);
	unsigned total_n    = nx * ny;
	unsigned block_qtd, block_qtd_i, block_qtd_j;
	obj_func.view_container.getBlockNumberBlockView_Mat_Exec(block_qtd, block_qtd_i, block_qtd_j);
	
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
	
	const dim3 threads(1, GPU_WARP_SIZE * GPU_MAX_WARP_COUNT, 1);
    const dim3 blocks (std::min((total_n + threads.y - 1 ) / threads.y, obj_func.gpu_max_blocks), 1, 1);
	
	kernel_cuda_2D<<<blocks, threads, 0, starpu_cuda_get_local_stream()>>>(nx, ny, ld,
																		   args.idx_block, args.idx_block_i, args.idx_block_j,
																		   block_qtd, block_qtd_i, block_qtd_j,
																		   func);
	cudaStreamSynchronize(starpu_cuda_get_local_stream());
}

template<typename TFunc>
void cuda_impl_reduction_1D(void *buffers[], void *cl_arg) {
	//printf("CUDA reduction\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	unsigned n         = obj_func.view_container.getBlockView().block_size;
	unsigned block_qtd = obj_func.view_container.getBlockView().block_number;
	unsigned size      = obj_func.view_container.parallel_views.size();
	
	for(int i=0; i<size; i++) {
		parallel_view::data_t data_block(STARPU_VECTOR_GET_PTR(buffers[i]), STARPU_VECTOR_GET_NX(buffers[i]));
		obj_func.view_container.parallel_views[i]->data_block = data_block;
	}
	obj_func.reduction_var_ptr = STARPU_VARIABLE_GET_PTR(buffers[size]);
	
	const dim3 threads(1, GPU_WARP_SIZE * GPU_MAX_WARP_COUNT, 1);
    const dim3 blocks (std::min((n + threads.y - 1 ) / threads.y, obj_func.gpu_max_blocks), 1, 1);
	
	kernel_cuda_1D<<<blocks, threads, 0, starpu_cuda_get_local_stream()>>>(n, args.idx_block, block_qtd, func);
	
	cudaStreamSynchronize(starpu_cuda_get_local_stream());
}

template<typename TFunc>
void cuda_impl_reduction_2D(void *buffers[], void *cl_arg) {
	//printf("CUDA reduction 2D\n");
	StarPU_Args<TFunc>& args = *(reinterpret_cast<StarPU_Args<TFunc>*>(cl_arg)); 
	
	TFunc func = args.func;
	StarPU_Functor& obj_func = reinterpret_cast<StarPU_Functor&>(func);
	
	/* length of the matrix block */
	unsigned idx_handle = obj_func.view_container.getBlockViewIdx();
	unsigned nx         = STARPU_MATRIX_GET_NX(buffers[idx_handle]);
	unsigned ny         = STARPU_MATRIX_GET_NY(buffers[idx_handle]);
	unsigned ld         = STARPU_MATRIX_GET_LD(buffers[idx_handle]);
	unsigned total_n    = nx * ny;
	unsigned size = obj_func.view_container.parallel_views.size();
	unsigned block_qtd, block_qtd_i, block_qtd_j;
	obj_func.view_container.getBlockNumberBlockView_Mat_Exec(block_qtd, block_qtd_i, block_qtd_j);
	
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
	
	const dim3 threads(1, GPU_WARP_SIZE * GPU_MAX_WARP_COUNT, 1);
    const dim3 blocks (std::min((total_n + threads.y - 1 ) / threads.y, obj_func.gpu_max_blocks), 1, 1);
	
	kernel_cuda_2D<<<blocks, threads, 0, starpu_cuda_get_local_stream()>>>(nx, ny, ld,
																		   args.idx_block, args.idx_block_i, args.idx_block_j,
																		   block_qtd, block_qtd_i, block_qtd_j,
																		   func);
	cudaStreamSynchronize(starpu_cuda_get_local_stream());
}

//REDUCTIONS
template<typename TData>
void cuda_impl_reduction_init(void *buffers[], void *cl_arg) {
	//printf("CUDA redux init\n");
	TData* var = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[0]));
	
	TData value;
	switch(gb_redux_op) {
		case parallel_utils::red_Mult:
			value = static_cast<TData>(1);
			break;
			
		case parallel_utils::red_Max:
			value =  std::numeric_limits<TData>::min();
			break;
			
		case parallel_utils::red_Min:
			value =  std::numeric_limits<TData>::max();
			break;
			
		default: //parallel_utils::red_Sum
			value = static_cast<TData>(0); 
	}
	
	cudaMemcpyAsync(var, &value, sizeof(TData), cudaMemcpyHostToDevice, starpu_cuda_get_local_stream());
}

template<typename TData>
void cuda_impl_reduction_redux(void *buffers[], void *cl_arg) {
	//printf("CUDA redux end\n");

	TData* var_rw = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[0]));
	TData* var_r  = reinterpret_cast<TData*>(STARPU_VARIABLE_GET_PTR(buffers[1]));
	
	kernel_cuda_reduction_redux<<<1, 1, 0, starpu_cuda_get_local_stream()>>>
		(var_rw, var_r, gb_redux_op);
}

}
