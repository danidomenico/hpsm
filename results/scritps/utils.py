import csv
from scipy.stats import t
import numpy
import math

#Variaveis Globais - Indice das colunas
col_benchmark  = 0
col_backend    = 1
col_size       = 2
col_bl_size    = 3
col_iter       = 4
col_threads    = 5
col_gpus       = 6
col_time       = 7

#Variaveis Globais - Backends
#nm_backend_serial      = "PARALLEL_BACK_SERIAL"
nm_backend_openmp       = "PARALLEL_BACK_OPENMP"
nm_backend_starpu       = "PARALLEL_BACK_STARPU"
nm_backend_kaapi        = "PARALLEL_BACK_KAAPI"
nm_backend_starpu_omp   = "PARALLEL_BACK_STARPU_OPENMP"
nm_backend_starpu_kaapi = "PARALLEL_BACK_STARPU_KAAPI"
nm_serial               = "SERIAL"

array_backends = [nm_backend_openmp, nm_backend_starpu, nm_backend_kaapi, nm_backend_starpu_omp, nm_backend_starpu_kaapi]

#Variaveis Globais - Benchmarks
array_benchmarks = ["hotspot", "nbody", "cfd"]

sizes = {
	"hotspot" : {
		"thread" : 16384,
		"size"  : [12288, 14336, 16384, 18432, 20480]
	},
		
	"nbody" : {
		"thread" : 98304,
		"size"  : [65536, 81920, 98304, 114688, 131072]
	},
	
	"cfd" : {
		"thread" : 131072,
		"size"  : [98304, 114688, 131072, 147456, 163840] 
	}
}
   
block_sizes = {
	"hotspot" : 1024,
	"nbody"   : 2048,
	"cfd"     : 2048,
}

block_sizes_large = {
	"hotspot" : 2048,
	"nbody"   : 4096,
	"cfd"     : 4096,
}

starpu_best_config = {
	"nbody" : {
		nm_backend_starpu       : [28, 27, 26, 25, 24],
		nm_backend_starpu_omp   : [28, 27, 26, 25, 24],
		nm_backend_starpu_kaapi : [28, 27, 26, 25, 24],
	},
	"hotspot" : {
		nm_backend_starpu       : [28, 26, 25, 24, 23],
		nm_backend_starpu_omp   : [28, 12, 12, 10, 10],
		nm_backend_starpu_kaapi : [28, 12, 12, 10, 10],
	},
	"cfd" : {
		nm_backend_starpu        : [28, 26, 12, 10, 10],
		nm_backend_starpu_omp    : [28, 10, 10, 10, 10],
		nm_backend_starpu_kaapi  : [28, 10, 10, 10, 10],
	}
}

#Variaveis Globais - Configurations
array_threads_t = [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28]
array_threads_s = [28, 27, 26, 25, 24]
array_gpus      = [0, 1, 2, 3, 4]
max_threads     = 28


#Function to read input files
def read_input(input_files, data):
	for fname in input_files:
		print "processing", fname, "..."
		with open(fname) as infile:
			reader = csv.reader(infile, delimiter=';', quoting=csv.QUOTE_NONE)
			filtro = filter(lambda x: len(x) > 0, reader)
			for linha in filtro:
				if "kaapi-numa" in fname:
					linha[col_backend] = "PARALLEL_BACK_KAAPI"
				if "starpu+omp" in fname:
					linha[col_backend] = "PARALLEL_BACK_STARPU_OPENMP"
				if "starpu+kaapi" in fname:
					linha[col_backend] = "PARALLEL_BACK_STARPU_KAAPI"
				if linha[col_benchmark] == "cdf_solver":
					linha[col_benchmark] = "cfd"
				data.append(linha)

#Function to calc mean, sd and error margim
def calc_mean_error(sample):
	# confidence interval of 95%
	tdist = t.ppf(0.95, len(sample)-1)
	mean = numpy.mean(sample)
	std = numpy.std(sample)
	error = tdist*(std/math.sqrt(len(sample)))
	return mean, std, error

#Return the id of a line
def get_id_line(line):
	return line[col_benchmark] + ";" + \
			 line[col_backend]   + ";" + \
			 line[col_size]      + ";" + \
			 line[col_bl_size]   + ";" + \
			 line[col_iter]      + ";" + \
			 line[col_threads]   + ";" + \
			 line[col_gpus]
