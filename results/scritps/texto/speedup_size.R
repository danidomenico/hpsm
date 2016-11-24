library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../../dados/idcin-2/data_R.csv", header=TRUE, sep=";")
serial <- dados_all[grep("^SERIAL$",dados_all$backend),]
serial_stat <- ddply(serial, entry, summarise,
                     n=length(time), mean=mean(time), sd=sd(time),
                     se=sd/sqrt(n), ci=se*qt(.95/2 + .5, n-1))

# calculate speedup
dados <- dados_all[grep("SERIAL", dados_all$backend, invert=TRUE),] #eliminate SERIAL

args <- commandArgs(trailingOnly = TRUE)
bench <- args[1]
dados <- dados[(dados$BENCH == bench),]

dados <- dados[((dados$BENCH == "hotspot") & (dados$block_size == 1024)) |
               ((dados$BENCH == "nbody") & (dados$block_size == 2048)) |
               ((dados$BENCH == "cfd") & (dados$block_size == 2048)),] 

dados <- dados[((dados$thread == 28) & (dados$gpus == 0)) |
               ((dados$thread == 27) & (dados$gpus == 1)) |
               ((dados$thread == 26) & (dados$gpus == 2)) | 
               ((dados$thread == 25) & (dados$gpus == 3)) |
               ((dados$thread == 24) & (dados$gpus == 4)) |
               ((dados$thread == 0) & (dados$gpus > 0) & (dados$backend == "PARALLEL_BACK_STARPU")),]
dados$speedup <- 1:nrow(dados)
for(i in 1:nrow(dados)){
	name <- dados[i,]$BENCH
	sz <- dados[i,]$size
	tserial <- serial_stat[(serial_stat$BENCH == name) & (serial_stat$size == sz),]$mean
	dados[i,]$speedup <- tserial / dados[i,]$time
}

# rename GPUs label
dados[dados$gpus == 0, "gpus"] <- as.character("0 GPU (apenas CPUs)")
dados[dados$gpus == 1, "gpus"] <- as.character("1 GPU")
dados[dados$gpus == 2, "gpus"] <- as.character("2 GPUs")
dados[dados$gpus == 3, "gpus"] <- as.character("3 GPUs")
dados[dados$gpus == 4, "gpus"] <- as.character("4 GPUs")
# rename backends
dados$backend <- ifelse((dados$backend == "PARALLEL_BACK_STARPU") & 
                        (dados$thread == 0),                            "StarPU(GPU)  ",   as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_KAAPI",         "Kaapi  ",         as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU",        "StarPU  ",        as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_OPENMP",        "OpenMP  ",        as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_OPENMP", "StarPU+OpenMP  ", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_KAAPI",  "StarPU+Kaapi  ",  as.character(dados$backend))
# rename sizes nbody
dados$size <- ifelse((dados$size == 65536)  & (dados$BENCH == "nbody"), 1, as.integer(dados$size))
dados$size <- ifelse((dados$size == 81920)  & (dados$BENCH == "nbody"), 2, as.integer(dados$size))
dados$size <- ifelse((dados$size == 98304)  & (dados$BENCH == "nbody"), 3, as.integer(dados$size))
dados$size <- ifelse((dados$size == 114688) & (dados$BENCH == "nbody"), 4, as.integer(dados$size))
dados$size <- ifelse((dados$size == 131072) & (dados$BENCH == "nbody"), 5, as.integer(dados$size))
# rename sizes hotspot
dados$size <- ifelse((dados$size == 12288) & (dados$BENCH == "hotspot"), 1, as.integer(dados$size))
dados$size <- ifelse((dados$size == 14336) & (dados$BENCH == "hotspot"), 2, as.integer(dados$size))
dados$size <- ifelse((dados$size == 16384) & (dados$BENCH == "hotspot"), 3, as.integer(dados$size))
dados$size <- ifelse((dados$size == 18432) & (dados$BENCH == "hotspot"), 4, as.integer(dados$size))
dados$size <- ifelse((dados$size == 20480) & (dados$BENCH == "hotspot"), 5, as.integer(dados$size))
# rename sizes cfd
dados$size <- ifelse((dados$size == 98304)  & (dados$BENCH == "cfd"), 1, as.integer(dados$size))
dados$size <- ifelse((dados$size == 114688) & (dados$BENCH == "cfd"), 2, as.integer(dados$size))
dados$size <- ifelse((dados$size == 131072) & (dados$BENCH == "cfd"), 3, as.integer(dados$size))
dados$size <- ifelse((dados$size == 147456) & (dados$BENCH == "cfd"), 4, as.integer(dados$size))
dados$size <- ifelse((dados$size == 163840) & (dados$BENCH == "cfd"), 5, as.integer(dados$size))

dados_stat <- ddply(dados, entry, summarise, 
                     n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                     ci=se*qt(.95/2 + .5, n-1))
# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

my_plot <- ggplot(dados_stat, aes(x=size, y=mean, color=factor(backend), shape=factor(backend), linetype=factor(backend))) +
    geom_errorbar(aes(ymin=mean-ci, ymax=mean+ci), color="black", linetype="solid", width=.1, position=pd) +
    geom_point(size=2.4) +
    geom_line(linetype="solid") +
    theme_bw() +
    scale_colour_discrete(name="Back-ends: ") +
    scale_shape_discrete(name="Back-ends: ") +
    scale_linetype_discrete(name="Back-ends: ") +
    #theme(axis.text.x = element_text(size=8), axis.text.y= element_text(size=8), legend.position="top") + 
    theme(text = element_text(size=13), axis.text.x=element_text(size=10, angle=40, hjust=1),
          legend.position="top", legend.key.width=unit(0.9, "cm"), legend.text=element_text(size=11)) + 
    xlab("Tamanhos") +
    ylab("Speedup") + 
    #scale_x_continuous(labels=c("", "", "", "", "")) +
    #scale_y_continuous(limits=c(0,110), breaks=seq(0,100,10)) +
    #facet_grid(. ~ gpus, scales="free_y") +
    facet_wrap(~ gpus, scales = "free_x", ncol=3) +
    guides(col=guide_legend(ncol = 6))
    
if(bench == "nbody") {
	my_plot <- my_plot + 
	           xlab("Número de partículas") +
	           scale_x_continuous(labels=c("65536", "81920", "98304", "114688", "131072")) +
	           scale_y_continuous(limits=c(0,50), breaks=seq(0,50,10))
} else if(bench == "hotspot") {
	my_plot <- my_plot + 
	           xlab("Ordem da matriz (dimensão do processador)") +
	           scale_x_continuous(labels=c("12288", "14336", "16384", "18432", "20480")) +
	           scale_y_continuous(limits=c(0,100), breaks=seq(0,100,20))
} else if(bench == "cfd") {
	my_plot <- my_plot + 
	           xlab("Número de elementos") +
	           scale_x_continuous(labels=c("98304", "114688", "131072", "147456", "163840")) +
	           scale_y_continuous(limits=c(0,60), breaks=seq(0,60,10))
}
    
#ggsave(file="chart.pdf", plot=my_plot, height=3.8, width=9)
ggsave(file="chart.pdf", plot=my_plot, height=7.5, width=9)