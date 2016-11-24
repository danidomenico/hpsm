import sys
import argparse
import utils

def get_id_line(line):
	return line[utils.col_benchmark] + ";" + \
			 line[utils.col_backend]   + ";" + \
			 line[utils.col_size]      + ";" + \
			 line[utils.col_bl_size]   + ";" + \
			 line[utils.col_threads]   + ";" + \
			 line[utils.col_gpus]

def check_sample(dados, bench, back, size, bs, threads, gpus):
	lines = filter(lambda x: bench == x[utils.col_benchmark] and 
									  back == x[utils.col_backend] and 
									  str(size) == x[utils.col_size] and
									  str(bs) == x[utils.col_bl_size] and
									  str(threads) == x[utils.col_threads] and
									  str(gpus) == x[utils.col_gpus], dados)
	
	length = len(lines)
	if length <= 0:
		print "ERROR - Samples were not found. Verify it!"
		return
	
	sample = []
	for l in lines:
		sample.append(float(l[utils.col_time]))
	sample.sort()
	
	min_time, max_time = sample[0], sample[length-1]
	diff_margin        = min_time * 0.20 #20% 
	
	mean, std, error   = utils.calc_mean_error(sample)
	
	if (error > 0.9) or ((max_time - min_time) > diff_margin):
		print utils.get_id_line(lines[0])

def check_inputs(dados):
	for bench in utils.array_benchmarks:
		print bench, ":"
		sizes_bench = utils.sizes[bench]
		block_size  = utils.block_sizes[bench]
		
		for back in utils.array_backends:
			
			#Speedup threads
			for t in utils.array_threads_t:
				if back == utils.nm_backend_starpu or back == utils.nm_backend_starpu_omp or back == utils.nm_backend_starpu_kaapi:
					for g in utils.array_gpus:
						if t == 0 and g == 0:
							continue
						
						check_sample(dados, bench, back, sizes_bench["thread"], block_size, t, g)
					
				else:
					if(t > 0):
						check_sample(dados, bench, back, sizes_bench["thread"], block_size, t, 0)
						
			#Speedup size
			#for size in sizes_bench["other"]:
			#	if back == utils.nm_backend_starpu:
			#		for i in range(len(utils.array_threads_s)):
			#			count = lines(dados, bench, back, size, block_size, utils.array_threads_s[i], utils.array_gpus[i])
			#			print "\t", count, bench, back, size, array_threads_s[i], array_gpus[i]
						
			#	else:
			#		count = lines(dados, bench, back, size, block_size, utils.max_threads, 0)
			#		print "\t", count, bench, back, size, max_threads, 0

def main():
	data = list() # array das linhas
	
	parser = argparse.ArgumentParser(description='Estatistica')
	parser.add_argument("-e", "--entrada", nargs='+', help="arquivos de entrada")
	args = parser.parse_args()
	
	#if len(args.entrada) <= 0:
	#	print "Informe os arquivos de entrada com o paremetro -e"
	#	return
	
	utils.read_input(args.entrada, data)
	
	check_inputs(data)
	
if __name__ == "__main__":
	main()