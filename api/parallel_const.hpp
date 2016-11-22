#pragma once

#define ENUM_TO_STR(name) #name

/*
 * External enumerators
 */
namespace hpsm {

namespace AccessMode {
	
enum Parallel_AccessMode {
	In,
	Out,
	InOut
};
	
} //namespace AccessMode

namespace PartitionMode {

enum Parallel_PartitionMode {
	Vector,
	Matrix_Horiz,
	Matrix_Vert,
	Matrix_Vert_Horiz
};

} //namespace PartitionMode

namespace ReduxMode {

enum Parallel_ReduxMode {
	Sum,
	Mult,
	Max,
	Min
};

} //namespace ReduxMode

namespace BlockTile {

enum Parallel_BlockTile {
	Intercalary,
	Sequentially
};

} //namespace BlockTile

namespace Workers {

enum Parallel_Workers {
	Cpu,
	Gpu
};

} //namespace Workers

} //namespace hpsm


/*
 * Internal enumerators
 */
enum Parallel_MessageType {
	PARALLEL_MESSAGE_ERROR,
	PARALLEL_MESSAGE_WARNING
};

enum Parallel_Backend {
	//PARALLEL_BACK_CUDA,
	PARALLEL_BACK_OPENMP,
	PARALLEL_BACK_SERIAL,
	PARALLEL_BACK_STARPU
};

/*
 * Aux structures
 */
static const char* parallel_Map_Str[] = {
  ENUM_TO_STR(parallel::PartitionMode::Vector),
  ENUM_TO_STR(parallel::PartitionMode::Matrix_Horiz),
  ENUM_TO_STR(parallel::PartitionMode::Matrix_Vert),
  ENUM_TO_STR(parallel::PartitionMode::Matrix_Vert_Horiz)
};

static const char* parallel_Backend_Str[] = {
  //ENUM_TO_STR(PARALLEL_BACK_CUDA),
  ENUM_TO_STR(PARALLEL_BACK_OPENMP),
  ENUM_TO_STR(PARALLEL_BACK_SERIAL),
  ENUM_TO_STR(PARALLEL_BACK_STARPU)
};