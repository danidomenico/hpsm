import sys
import argparse
import utils

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

def main():
	dados = list() # array das linhas
	
	parser = argparse.ArgumentParser(description='Estatistica')
	parser.add_argument("-e", "--entrada", nargs='+', help="arquivos de entrada")
	parser.add_argument("-b", "--benchmark", help="benchmark")
	parser.add_argument("-bk", "--backend", help="backend")
	parser.add_argument("-s", "--size", help="size")
	parser.add_argument("-bs", "--block_size", help="block size")
	parser.add_argument("-t", "--threads", help="threads")
	parser.add_argument("-g", "--gpus", help="gpus")
	
	args = parser.parse_args()
	
	bench   = args.benchmark
	back    = args.backend
	size    = args.size
	bs      = args.block_size
	threads = args.threads
	gpus    = args.gpus
	
	#if len(args.entrada) <= 0:
	#	print "Informe os arquivos de entrada com o paremetro -e"
	#	return
	
	utils.read_input(args.entrada, dados)
	
	check_amostra(dados, bench, back, size, bs, threads, gpus, True)
	
if __name__ == "__main__":
	main()