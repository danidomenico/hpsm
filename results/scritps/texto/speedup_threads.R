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

dados <- dados[((dados$BENCH == "hotspot") & (dados$size == 16384) &  (dados$block_size == 1024)) | 
               ((dados$BENCH == "nbody")   & (dados$size == 98304) &  (dados$block_size == 2048)) |
               ((dados$BENCH == "cfd")     & (dados$size == 131072) & (dados$block_size == 2048)),]
dados <- dados[(dados$thread %% 2) == 0,]

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
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_KAAPI",         "Kaapi  ",         as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU",        "StarPU  ",        as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_OPENMP",        "OpenMP  ",        as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_OPENMP", "StarPU+OpenMP  ", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_KAAPI",  "StarPU+Kaapi  ",  as.character(dados$backend))

dados_stat <- ddply(dados, entry, summarise, 
                    n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                    ci=se*qt(.95/2 + .5, n-1))

# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

my_plot <- ggplot(dados_stat, aes(x=thread, y=mean, color=factor(backend), shape=factor(backend), linetype=factor(backend)))+
    geom_errorbar(aes(ymin=mean-ci, ymax=mean+ci), color="black", linetype="solid", width=.1, position=pd) +
    geom_point(size=2.4) +
    geom_line(linetype="solid") +
    theme_bw() +
    scale_colour_discrete(name="Back-ends: ") +
    scale_shape_discrete(name="Back-ends: ") +
    #scale_linetype_discrete(name="Back-ends: ") +
    #theme(axis.text.x = element_text(size=8), axis.text.y= element_text(size=8), legend.position="top") + 
    theme(text = element_text(size=13), 
          legend.position="top", legend.key.width=unit(1, "cm"), legend.text=element_text(size=11)) + 
    xlab("Threads") +
    ylab("Speedup") + 
    scale_x_continuous(limits=c(-1,28), breaks=seq(0,28,4)) +
    #scale_y_continuous(limits=c(0,110), breaks=seq(0,100,10)) +
    facet_wrap(~ gpus, scales = "free_x", ncol=3)
    #facet_grid(. ~ gpus, scales="free_y")
    
if(bench == "cfd") {
	my_plot <- my_plot + scale_y_continuous(limits=c(0,60), breaks=seq(0,60,10))
} else if (bench == "hotspot") {
	my_plot <- my_plot + scale_y_continuous(limits=c(0,105), breaks=seq(0,100,20))
}

#ggsave(file="chart.pdf", plot=my_plot, height=3.8, width=9)
ggsave(file="chart.pdf", plot=my_plot, height=7.5, width=9)