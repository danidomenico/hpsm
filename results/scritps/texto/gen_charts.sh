#Parameters
ALL=1
GEN_THREADS=0
GEN_SIZE=0
GEN_SIZE_BEST=0
GEN_STARPU_BLOCK=0
GEN_OVERHEAD=0

#Get parameter
if [ $# -ge 1 ]; then
	ALL=0
	if [ $1 == "-t" ]; then
		GEN_THREADS=1
	elif [ $1 == "-s" ]; then
		GEN_SIZE=1
	elif [ $1 == "-sb" ]; then
		GEN_SIZE_BEST=1
	elif [ $1 == "-b" ]; then
		GEN_STARPU_BLOCK=1
	elif [ $1 == "-o" ]; then
		GEN_OVERHEAD=1
	elif [ $1 == "-h" ] || [ $1 == "--help" ]; then
		echo "Welcome to gen_charts.sh help message."
		echo ""
		echo "./gen_charts [OPT]:"
		echo "<blank>: generate all charts;"
		echo "-t: generate threads speedup charts;"
		echo "-s: generate max. configuration speedup charts (size);"
		echo "-sb: generate best configuration speedup charts (size best);"
		echo "-b: generate speedup charts varying block size (just StarPU versions);"
		echo "-o: generate overhead chart;"
		echo ""
	else
		echo ""
		echo "Invalid parameter. It must be blank, -t, -s, -sb. -b or -o!"
		echo "Use -h to see the help message!"
		echo ""
		exit
	fi
fi

if [ $ALL == 1 ] || [ $GEN_THREADS == 1 ]; then
	echo "generating speedup_threads - NBody..."
	Rscript speedup_threads.R nbody
	mv chart.pdf nbody_threads.pdf

	echo "generating speedup_threads - Hotspot..."
	Rscript speedup_threads.R hotspot
	mv chart.pdf hotspot_threads.pdf

	echo "generating speedup_threads - CFD..."
	Rscript speedup_threads.R cfd
	mv chart.pdf cfd_threads.pdf
fi

if [ $ALL == 1 ] || [ $GEN_SIZE == 1 ]; then
	echo "generating speedup_size - NBody..."
	Rscript speedup_size.R nbody
	mv chart.pdf nbody_size.pdf

	echo "generating speedup_size - Hotspot..."
	Rscript speedup_size.R hotspot
	mv chart.pdf hotspot_size.pdf

	echo "generating speedup_size - CFD..."
	Rscript speedup_size.R cfd
	mv chart.pdf cfd_size.pdf
fi

if [ $ALL == 1 ] || [ $GEN_SIZE_BEST == 1 ]; then
	echo "generating speedup_size_best - NBody..."
	Rscript speedup_size_best.R nbody
	mv chart.pdf nbody_size_best.pdf

	echo "generating speedup_size_best - Hotspot..."
	Rscript speedup_size_best.R hotspot
	mv chart.pdf hotspot_size_best.pdf

	echo "generating speedup_size_best - CFD..."
	Rscript speedup_size_best.R cfd
	mv chart.pdf cfd_size_best.pdf
fi

if [ $ALL == 1 ] || [ $GEN_STARPU_BLOCK == 1 ]; then
	echo "generating speedup_starpu_block - NBody..."
	Rscript speedup_starpu_block.R nbody
	mv chart.pdf nbody_starpu_block.pdf

	echo "generating speedup_starpu_block - Hotspot..."
	Rscript speedup_starpu_block.R hotspot
	mv chart.pdf hotspot_starpu_block.pdf

	echo "generating speedup_starpu_block - CFD..."
	Rscript speedup_starpu_block.R cfd
	mv chart.pdf cfd_starpu_block.pdf
fi

if [ $ALL == 1 ] || [ $GEN_OVERHEAD == 1 ]; then
	echo "generating overhead..."
	Rscript overhead.R
	mv chart.pdf overhead.pdf
fi