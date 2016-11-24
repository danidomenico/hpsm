echo "Threads..."
python script_count_inputs.py -e ../dados/idcin-2/data_R.csv -v thread > count_thread.txt

echo "Size..."
python script_count_inputs.py -e ../dados/idcin-2/data_R.csv -v size > count_size.txt

echo "Size best..."
python script_count_inputs.py -e ../dados/idcin-2/data_R.csv -v size_best > count_size_best.txt

echo "StarPU block..."
python script_count_inputs.py -e ../dados/idcin-2/data_R.csv -v starpu_block > count_starpu_block.txt