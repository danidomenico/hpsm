import sys
import argparse
import utils

output_thread    = "thread"
output_size      = "size"
output_size_best = "size_best"
output_block     = "starpu_block"

def lines(dados, bench, backend, size, block_size, threads, gpus):
	linhas = filter(lambda x: bench == x[utils.col_benchmark] and 
									  backend == x[utils.col_backend] and 
									  str(size) == x[utils.col_size] and
									  str(block_size) == x[utils.col_bl_size] and
									  str(threads) == x[utils.col_threads] and
									  str(gpus) == x[utils.col_gpus], dados)
	return len(linhas)

def count_inputs(dados, output_version):
	print len(dados)
	
	for bench in utils.array_benchmarks:
		sizes_bench = utils.sizes[bench]
		block_size  = utils.block_sizes[bench]
		
		
		print bench, "BS:" + str(block_size) if output_version != output_block else "", ":"
		
		for back in utils.array_backends:
			
			#Speedup threads
			if output_version == output_thread:
				for t in utils.array_threads_t:
					if back == utils.nm_backend_starpu or back == utils.nm_backend_starpu_omp or back == utils.nm_backend_starpu_kaapi:
						for g in utils.array_gpus:
							if t == 0 and g == 0:
								continue
							
							count = lines(dados, bench, back, sizes_bench["thread"], block_size, t, g)
							print "\t", count, bench, back, sizes_bench["thread"], t, g
						
					else:
						if(t > 0):
							count = lines(dados, bench, back, sizes_bench["thread"], block_size, t, 0)
							print "\t", count, bench, back, sizes_bench["thread"], t, 0
						
			#Speedup size
			elif output_version == output_size:
				for size in sizes_bench["size"]:
					if back in [utils.nm_backend_starpu, utils.nm_backend_starpu_omp, utils.nm_backend_starpu_kaapi]:
						#Just GPU
						if back == utils.nm_backend_starpu:
							for g in utils.array_gpus[1:]:
								count = lines(dados, bench, back, size, block_size, 0, g)
								print "\t", count, bench, back, size, 0, g
						
						#GPU + CPU
						for i in range(len(utils.array_threads_s)):
							count = lines(dados, bench, back, size, block_size, utils.array_threads_s[i], utils.array_gpus[i])
							print "\t", count, bench, back, size, utils.array_threads_s[i], utils.array_gpus[i]
							
					else:
						count = lines(dados, bench, back, size, block_size, utils.max_threads, 0)
						print "\t", count, bench, back, size, utils.max_threads, 0
						
			#Speedup size best
			elif output_version == output_size_best:
				for size in sizes_bench["size"]:
					if back in [utils.nm_backend_starpu, utils.nm_backend_starpu_omp, utils.nm_backend_starpu_kaapi]:
						array_threads = utils.starpu_best_config[bench][back]
						for i in range(len(array_threads)):
							count = lines(dados, bench, back, size, block_size, array_threads[i], utils.array_gpus[i])
							print "\t", count, bench, back, size, array_threads[i], utils.array_gpus[i]
							
					else:
						count = lines(dados, bench, back, size, block_size, utils.max_threads, 0)
						print "\t", count, bench, back, size, utils.max_threads, 0
			
			#Speedup starpu block
			elif output_version == output_block:
				block_size_large = utils.block_sizes_large[bench]
				
				for t in utils.array_threads_t:
					if back == utils.nm_backend_starpu or back == utils.nm_backend_starpu_omp or back == utils.nm_backend_starpu_kaapi:
						for g in utils.array_gpus:
							if t == 0 and g == 0:
								continue
							
							#count = lines(dados, bench, back, sizes_bench["thread"], block_size, t, g)
							#print "\t", count, bench, back, sizes_bench["thread"], block_size, t, g
							
							count = lines(dados, bench, back, sizes_bench["thread"], block_size_large, t, g)
							print "\t", count, bench, back, sizes_bench["thread"], block_size_large, t, g

def main():
	dados = list() # array das linhas
	
	parser = argparse.ArgumentParser(description='Estatistica')
	parser.add_argument("-e", "--entrada", nargs='+', help="arquivos de entrada")
	parser.add_argument("-v", "--version", choices=[output_thread, output_size, output_size_best, output_block], help="Arquitetura")
	args = parser.parse_args()
	
	if args.entrada == None:
		print "Informe os arquivos de entrada com o paremetro -e"
		return
	
	output_version = args.version
	if output_version == None:
		output_version = output_thread
	
	utils.read_input(args.entrada, dados)
	
	count_inputs(dados, output_version)
	
if __name__ == "__main__":
	main()