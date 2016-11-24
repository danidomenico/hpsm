#!/usr/bin/gnuplot -persist

#set term postscript eps enhanced color
set terminal pdfcairo enhanced size 4,3.5
set output "starpu_15funcComputeFlux_boxes.pdf"
set title "Model for codelet 15funcComputeFlux"
set xlabel "Total data size"
set ylabel "Time (ms)"

#set key top left
set key box outside center right horizontal Left reverse font 'Arial,12' maxcolumns 1
#set logscale x
set logscale y

#set xrange [1:10**9]

set macros

set boxwidth 0.12
set style fill transparent solid 1

plot 	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.91):3  with boxes title "GPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.78):11 with boxes title "1 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.65):13 with boxes title "2 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.52):17 with boxes title "4 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.39):21 with boxes title "6 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.26):25 with boxes title "8 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1-0.13):29 with boxes title "10 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using 1:33:xtic(2) with boxes title "12 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.13):37 with boxes title "14 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.26):41 with boxes title "16 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.39):45 with boxes title "18 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.52):49 with boxes title "20 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.65):53 with boxes title "22 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.78):57 with boxes title "24 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+0.91):61 with boxes title "26 CPU",\
	'starpu_15funcComputeFlux_avg_boxes.data' using ($1+1.02):65 with boxes title "28 CPU"
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:6:7 with errorlines title "GPU 2",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:8:9 with errorlines title "GPU 3",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:10:11 with errorlines title "GPU 4",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:14:15 with errorlines title "3 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:18:19 with errorlines title "5 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:22:23 with errorlines title "7 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:26:27 with errorlines title "9 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:30:31 with errorlines title "11 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:34:35 with errorlines title "13 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:38:39 with errorlines title "15 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:42:43 with errorlines title "17 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:46:47 with errorlines title "19 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:50:51 with errorlines title "21 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:54:55 with errorlines title "23 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:58:59 with errorlines title "25 CPU",\
#	"starpu_15funcComputeFlux_avg_boxes.data" using 1:62:63 with errorlines title "27 CPU",\
