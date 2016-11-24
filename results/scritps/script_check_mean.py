import sys
import argparse
import utils

def get_speedup(dados, bench, size, parallel_time):
	linhas = filter(lambda x: bench == x[utils.col_benchmark] and 
									  utils.nm_serial == x[utils.col_backend] and 
									  str(size) == x[utils.col_size] and
									  str(size) == x[utils.col_bl_size], dados)
	
	sample = []
	for l in linhas:
		sample.append(float(l[utils.col_time]))

	if len(sample) > 0:
		mean, std, error = utils.calc_mean_error(sample)
		return mean/parallel_time
	else:
		return 0.0
	

def check_amostra(dados, bench, back, size, bs, threads, gpus, analitycal=False):
	linhas = filter(lambda x: bench == x[utils.col_benchmark] and 
									  back == x[utils.col_backend] and 
									  str(size) == x[utils.col_size] and
									  str(bs) == x[utils.col_bl_size] and
									  str(threads) == x[utils.col_threads] and
									  str(gpus) == x[utils.col_gpus], dados)
	
	print "Sample description:", bench, back, size, bs, threads, gpus 
	
	length = len(linhas)
	if length <= 0:
		print "ERROR - Samples were not found. Verify sample description!"
		return
	
	sample = []
	for l in linhas:
		sample.append(float(l[utils.col_time]))
	sample.sort()
	
	mean, std, error = utils.calc_mean_error(sample)
	
	print "\tSample size:", length
	print "\tSample MIN MAX:", sample[0], sample[length-1]
	if analitycal:
		print "\tSamples:"
		i = 0;
		a_txt = ""
		for a in sample:
			a_txt = a_txt + str(a) + ", "
			i = i+1
			if i%6 == 0:
				print "\t\t", a_txt
				a_txt = ""
		print "\t\t", a_txt #imprime o que sobrou
	
	print "\tSample MEAN, STD, ERROR:", mean, std, error
	print "\tSpeedup:", get_speedup(dados, bench, size, mean)

def main():
	dados = list() # array das linhas
	
	parser = argparse.ArgumentParser(description='Estatistica')
	parser.add_argument("-e", "--entrada", nargs='+', help="input files")
	parser.add_argument("-c", "--config", help="input configurations")
	
	args = parser.parse_args()
	
	input_arg = args.config.split(";")
	#print input_arg
	
	bench   = input_arg[utils.col_benchmark]
	back    = input_arg[utils.col_backend]
	size    = input_arg[utils.col_size]
	bs      = input_arg[utils.col_bl_size]
	threads = input_arg[utils.col_threads]
	gpus    = input_arg[utils.col_gpus]
	#print bench, back, size, bs, threads, gpus
	
	#if len(args.entrada) <= 0:
	#	print "Informe os arquivos de entrada com o paremetro -e"
	#	return
	
	utils.read_input(args.entrada, dados)
	
	check_amostra(dados, bench, back, size, bs, threads, gpus, True)
	
if __name__ == "__main__":
	main()