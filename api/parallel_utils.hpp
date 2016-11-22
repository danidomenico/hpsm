#pragma once

#include "parallel_const.hpp"
#include "parallel_macros.hpp"
#include "parallel_metaprog.hpp"

#if defined(BACKEND_STARPU)
#include <cuda_runtime.h> //Requerired by __host__ __device__ macros and g++
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace parallel_utils {

/* 
 * Type Alias
 */
using Parallel_Redux = hpsm::ReduxMode::Parallel_ReduxMode;
static const Parallel_Redux red_Sum  = hpsm::ReduxMode::Sum;
static const Parallel_Redux red_Mult = hpsm::ReduxMode::Mult;
static const Parallel_Redux red_Max  = hpsm::ReduxMode::Max;
static const Parallel_Redux red_Min  = hpsm::ReduxMode::Min;

using Parallel_BlockTile = hpsm::BlockTile::Parallel_BlockTile;
static const Parallel_BlockTile bt_Intercalary  = hpsm::BlockTile::Intercalary;
static const Parallel_BlockTile bt_Sequentially = hpsm::BlockTile::Sequentially;

using Parallel_Workers = hpsm::Workers::Parallel_Workers;
static const Parallel_Workers worker_Cpu = hpsm::Workers::Cpu;
static const Parallel_Workers worker_Gpu = hpsm::Workers::Gpu;

/*
 * Functions
 */
void print_message(const char msg[], Parallel_MessageType type = PARALLEL_MESSAGE_ERROR);

/*
 * Types
 */
struct Parallel_Interval {
	unsigned start = 0u;
	unsigned end   = 0u;
	
	Parallel_Interval() {};
	
	Parallel_Interval(unsigned size_) :
		start(0), end(size_) {}
	
	Parallel_Interval(unsigned start_, unsigned end_) :
		start(start_), end(end_) {}
		
	bool hasInterval() {
		return (start > 0) && (end > 0); 
	}
};
using interval = Parallel_Interval;

template<int dim>
struct Parallel_Range {
	//Constructor Range 1D
	template<int aux = dim>
	Parallel_Range(Parallel_Interval interval_i_, 
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==1>* = nullptr) :
		interval_i(interval_i_), block_tile(block_tile_) {}
		
	template<int aux = dim>
	Parallel_Range(unsigned size_, 
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==1>* = nullptr) :
		interval_i(size_), block_tile(block_tile_) {}
		
	template<int aux = dim>
	Parallel_Range(unsigned start_, unsigned end_,
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==1>* = nullptr) :
		interval_i(start_, end_), block_tile(block_tile_) {}

	//Constructor Range 2D
	template<int aux = dim>
	Parallel_Range(Parallel_Interval interval_i_, Parallel_Interval interval_j_,
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==2>* = nullptr) :
		interval_i(interval_i_), interval_j(interval_j_), block_tile(block_tile_) {}
		
	template<int aux = dim>
	Parallel_Range(unsigned size_i_, unsigned size_j_, 
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==2>* = nullptr) :
		interval_i(size_i_), interval_j(size_j_), block_tile(block_tile_) {}
		
	template<int aux = dim>
	Parallel_Range(unsigned start_i_, unsigned end_i_,
				   unsigned start_j_, unsigned end_j_,
				   Parallel_BlockTile block_tile_ = bt_Intercalary,
				   metaprog::Enable_if<aux==2>* = nullptr) :
		interval_i(start_i_, end_i_), interval_j(start_j_, end_j_), block_tile(block_tile_) {}	
	
	Parallel_Interval getInterval_i() {
		return interval_i;
	};
	
	Parallel_Interval getInterval_j() {
		return interval_j;
	};
	
	Parallel_BlockTile getBlockTile() {
		return block_tile;
	};
	
private:
	Parallel_Interval interval_i;
	Parallel_Interval interval_j;
	
	Parallel_BlockTile block_tile;
};
template<int dim>
using range = Parallel_Range<dim>;

template<int dim>
struct Parallel_Index {
	Parallel_Index() {};
	
	//Constructor Index 1D
	template<int aux = dim>
	PARALLEL_FUNCTION
	Parallel_Index(unsigned i_, unsigned block_tot_, unsigned block_qtd_tot_, unsigned size_i_,
				   metaprog::Enable_if<aux==1>* = nullptr) : 
				   i(i_), 
				   block_tot(block_tot_), block_i(block_tot_), 
				   block_qtd_tot(block_qtd_tot_), block_qtd_i(block_qtd_tot_),
				   size_i(size_i_) {}
	
	//Constructor Index 2D
	template<int aux = dim>
	PARALLEL_FUNCTION
	Parallel_Index(unsigned i_, unsigned j_,
				   unsigned block_tot_, unsigned block_i_, unsigned block_j_,
				   unsigned block_qtd_tot_, unsigned block_qtd_i_, unsigned block_qtd_j_,
				   unsigned size_i_, unsigned size_j_,
				   metaprog::Enable_if<aux==2>* = nullptr) : 
				   i(i_), j(j_),
				   block_tot(block_tot_), block_i(block_i_), block_j(block_j_),
				   block_qtd_tot(block_tot_), block_qtd_i(block_qtd_i_), block_qtd_j(block_qtd_j_),
				   size_i(size_i_), size_j(size_j_) {}

	PARALLEL_FUNCTION
	unsigned operator()(unsigned dimen) {
		if(dimen == 0)
			return i;
		else if(dimen == 1)
			return j;
		
		return -1u;
	};
	
	PARALLEL_FUNCTION
	unsigned block() {
		return block_tot;
	}
	
	PARALLEL_FUNCTION
	unsigned block_dim(unsigned dimen = 0) {
		if(dimen == 0)
			return block_i;
		else if(dimen == 1)
			return block_j;
		
		return -1u;
	}
	
	PARALLEL_FUNCTION
	unsigned block_qtd() {
		return block_qtd_tot;
	}
	
	PARALLEL_FUNCTION
	unsigned block_qtd_dim(unsigned dimen = 0) {
		if(dimen == 0)
			return block_qtd_i;
		else if(dimen == 1)
			return block_qtd_j;
		
		return -1u;
	}
	
	PARALLEL_FUNCTION
	unsigned size(unsigned dimen = 0) {
		if(dimen == 0)
			return size_i;
		else if(dimen == 1)
			return size_j;
		
		return -1u;
	}
	
private:
	unsigned i = -1u;
	unsigned j = -1u;
	
	unsigned block_tot = -1u;
	unsigned block_i   = -1u;
	unsigned block_j   = -1u;
	
	unsigned block_qtd_tot = -1u;
	unsigned block_qtd_i   = -1u;
	unsigned block_qtd_j   = -1u;
	
	unsigned size_i = -1u;
	unsigned size_j = -1u;
};
template<int dim>
using index = Parallel_Index<dim>;

struct Parallel_Block_Range {
	unsigned view_id;
	unsigned size          = 0;
	unsigned line_number   = 0;
	unsigned column_number = 0;
	unsigned block_size    = 0;
	
	Parallel_Block_Range(unsigned view_id_, unsigned size_, unsigned block_size_) : 
		view_id(view_id_), size(size_), block_size(block_size_) {}
		
	Parallel_Block_Range(unsigned view_id_, unsigned line_number_, unsigned column_number_, unsigned block_size_) : 
		view_id(view_id_), line_number(line_number_), column_number(column_number_), block_size(block_size_) {}
};
using block_range = Parallel_Block_Range;

struct Parallel_UniqueID {
	unsigned ID;
	
	Parallel_UniqueID() {
		ID = ++nextID;
	}
	
	Parallel_UniqueID(const Parallel_UniqueID& other) {
		ID = other.ID;
	}
	
	Parallel_UniqueID& operator=(const Parallel_UniqueID& other) {
		ID = other.ID;
		return *this;
	}

protected:
	static unsigned nextID;
};
using unique_id = Parallel_UniqueID;

struct Parallel_FunctorID {
	std::string key = "";
};
using functor_id = Parallel_FunctorID;

struct Parallel_Backend_Key {
protected:
	int backend_key_size = 0;
	Parallel_FunctorID* backend_key;
	
	template<int aux=0>
	void new_keys(int size) {
		backend_key      = new Parallel_FunctorID[size];
		backend_key_size = 0;
	}
	
	void delete_keys();
	
	bool has_key(std::string key);
	void insert_key(std::string key);
};
using backend_key = Parallel_Backend_Key;

} //namespace parallel_utils
