import csv
import glob
import os

#Variaveis Globais - Indice das colunas
col_benchmark  = 0
col_backend    = 1
col_size       = 2
col_bl_size    = 3
col_iter       = 4
col_threads    = 5
col_gpus       = 6
col_time       = 7

files_path_dmda   = "../dados/idcin-2"
files_path_sched  = "../dados/idcin-2/starpu_sched"
output_file = "data_R_starpu_sched.csv"
output_line = "{0};{1};{2};{3};{4};{5};{6};{7}\n"

output_data = []

#Read CSVs DMDA
csvfiles = []
for f in os.listdir(files_path_dmda):
	if f.endswith(".csv") and f != output_file:
		csvfiles.append(os.path.join(files_path_dmda, f))

for fname in csvfiles:
	if "starpu-" in fname:
		print "processing DMDA", fname, "..."
		with open(fname) as infile:
			reader = csv.reader(infile, delimiter=';', quoting=csv.QUOTE_NONE)
			filtro = filter(lambda x: "PARALLEL_BACK_STARPU" == x[col_backend], reader)
			for linha in filtro:
				if linha[col_benchmark] == "cdf_solver":
					linha[col_benchmark] = "cfd"
				linha[col_backend] = "PARALLEL_BACK_STARPU_dmda"
				output_data.append(linha)
	
	if "sequential-" in fname:
		print "processing Sequential", fname, "..."
		with open(fname) as infile:
			reader = csv.reader(infile, delimiter=';', quoting=csv.QUOTE_NONE)
			filtro = filter(lambda x: "SERIAL" == x[col_backend], reader)
			for linha in filtro:
				if linha[col_benchmark] == "cdf_solver":
					linha[col_benchmark] = "cfd"
				output_data.append(linha)


#Read CSVs DM, DMDAR, DMDAS
csvfiles = []
for f in os.listdir(files_path_sched):
	if f.endswith(".csv") and f != output_file:
		csvfiles.append(os.path.join(files_path_sched, f))

for fname in csvfiles:
	print "processing DM, DMDAR, DMDAS", fname, "..."
	with open(fname) as infile:
		reader = csv.reader(infile, delimiter=';', quoting=csv.QUOTE_NONE)
		for linha in reader:
			if "starpu-dm-" in fname:
				linha[col_backend] = "PARALLEL_BACK_STARPU_dm"
			if "starpu-dmdar-" in fname:
				linha[col_backend] = "PARALLEL_BACK_STARPU_dmdar"
			if "starpu-dmdas-" in fname:
				linha[col_backend] = "PARALLEL_BACK_STARPU_dmdas"
		
			if linha[col_benchmark] == "cdf_solver":
					linha[col_benchmark] = "cfd"
			output_data.append(linha)

#print csvfiles
with open(os.path.join(files_path_sched, output_file), 'w') as OUTPUT:
	OUTPUT.write("BENCH;backend;size;block_size;iterations;thread;gpus;time\n")
	for linha in output_data:
		OUTPUT.write(output_line.format(linha[col_benchmark], linha[col_backend], linha[col_size], linha[col_bl_size],
												  linha[col_iter], linha[col_threads], linha[col_gpus], linha[col_time]))
	OUTPUT.close()
