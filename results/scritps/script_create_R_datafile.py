import csv
import glob
import os
import utils

files_path  = "../dados/idcin-2"
output_file = "data_R.csv"

#print csvfiles
#with open(os.path.join(files_path, output_file), 'w') as OUTPUT:
#	OUTPUT.write("BENCH;backend;size;block_size;iterations;thread;gpus;time\n")
#	for fname in csvfiles:
#		print "processing", fname, "..."
#		with open(fname) as infile:
#			alltext = infile.read()
#			alltext = alltext.replace("cdf_solver", "cfd")
#			if "kaapi-numa" in fname:
#				alltext = alltext.replace("PARALLEL_BACK_OPENMP", "PARALLEL_BACK_KAAPI")
#			elif "starpu+omp" in fname:
#				alltext = alltext.replace("PARALLEL_BACK_STARPU", "PARALLEL_BACK_STARPU_OPENMP")
#			elif "starpu+kaapi" in fname:
#				alltext = alltext.replace("PARALLEL_BACK_STARPU", "PARALLEL_BACK_STARPU_KAAPI")
			
#			OUTPUT.write(alltext)
			#reader = csv.reader(alltext, delimiter=';', quoting=csv.QUOTE_NONE)
			#filtro = filter(lambda x: len(x) > 0, reader)
			#for linha in filtro:
			#print alltext
			
			#exit()

def get_csvfiles():
	csvfiles = []
	for f in os.listdir(files_path):
		if f.endswith(".csv") and f != output_file:
			csvfiles.append(os.path.join(files_path, f))
	
	return csvfiles

def is_valid_bech(line):
	return (line[utils.col_benchmark] in utils.array_benchmarks)

def get_time(line):
	return float(line[utils.col_time])

def cut_30(line, data, output_data, processed_data):
	if not is_valid_bech(line):
		return
	
	id_line = utils.get_id_line(line)
	if id_line in processed_data:
		return
	
	lines_from_id = filter(lambda x: line[utils.col_benchmark] == x[utils.col_benchmark] and 
												line[utils.col_backend]   == x[utils.col_backend] and 
												line[utils.col_size]      == x[utils.col_size] and
												line[utils.col_bl_size]   == x[utils.col_bl_size] and
												line[utils.col_threads]   == x[utils.col_threads] and
												line[utils.col_gpus]      == x[utils.col_gpus], 
								  data)
	
	#print "lines_from_id=", len(lines_from_id)
	lines_from_id.sort(key=get_time)
	
	#for l in lines_from_id:
	#	print l
	
	output_data.extend(lines_from_id[0:30])
	#print "len output_data=", len(output_data)
	
	processed_data.append(id_line)
	#print "len processed_data=", len(processed_data)

def main():
	data           = list() # array das linhas
	output_data    = list()
	processed_data = list()
	
	csvfiles = get_csvfiles()
	utils.read_input(csvfiles, data)
	print "available lines=", len(data)
	
	for line in data:
		#print line
		cut_30(line, data, output_data, processed_data)
	
	with open(os.path.join(files_path, output_file), 'w') as OUTPUT:
		OUTPUT.write("BENCH;backend;size;block_size;iterations;thread;gpus;time\n")
		
		print "output lines=", len(output_data)
		writer = csv.writer(OUTPUT, delimiter=';', quoting=csv.QUOTE_NONE)
		writer.writerows(output_data)
		#for line in output_data:
		#	OUTPUT.write("%s\n" % line)
		
		OUTPUT.close()

if __name__ == "__main__":
	main()