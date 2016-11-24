library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../dados/idcin-2/calib_starpu_size/dissertacao2016-starpu-1474476426.csv", header=TRUE, sep=";")
serial <- dados_all[grep("^SERIAL$",dados_all$backend),]
serial_stat <- ddply(serial, entry, summarise,
                     n=length(time), mean=mean(time), sd=sd(time),
                     se=sd/sqrt(n), ci=se*qt(.95/2 + .5, n-1))

# calculate speedup
dados <- dados_all[grep("SERIAL", dados_all$backend, invert=TRUE),] #eliminate SERIAL
dados$speedup <- 1:nrow(dados)
for(i in 1:nrow(dados)){
	name <- dados[i,]$BENCH
	sz <- dados[i,]$size
	tserial <- serial_stat[(serial_stat$BENCH == name) & (serial_stat$size == sz),]$mean
	dados[i,]$speedup <- tserial / dados[i,]$time
}

# rename GPUs label
dados[dados$gpus == 0, "gpus"] <- as.character("0 (only CPUs)")
dados[dados$gpus == 1, "gpus"] <- as.character("1 GPU")
dados[dados$gpus == 2, "gpus"] <- as.character("2 GPUs")
dados[dados$gpus == 3, "gpus"] <- as.character("3 GPUs")
dados[dados$gpus == 4, "gpus"] <- as.character("4 GPUs")
# rename backends
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_KAAPI", "Kaapi", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_OPENMP", "OpenMP", as.character(dados$backend))
dados$backend <- ifelse((dados$backend == "PARALLEL_BACK_STARPU") & (dados$thread == 0), "StarPU-GPU", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU", "StarPU", as.character(dados$backend))
# rename sizes nbody
dados$size <- ifelse((dados$size == 65536)  & (dados$BENCH == "nbody"), "Size 1", as.character(dados$size))
dados$size <- ifelse((dados$size == 81920)  & (dados$BENCH == "nbody"), "Size 2", as.character(dados$size))
dados$size <- ifelse((dados$size == 98304)  & (dados$BENCH == "nbody"), "Size 3", as.character(dados$size))
dados$size <- ifelse((dados$size == 114688) & (dados$BENCH == "nbody"), "Size 4", as.character(dados$size))
dados$size <- ifelse((dados$size == 131072) & (dados$BENCH == "nbody"), "Size 5", as.character(dados$size))
# rename sizes hotspot
dados$size <- ifelse((dados$size == 12288) & (dados$BENCH == "hotspot"), "Size 1", as.character(dados$size))
dados$size <- ifelse((dados$size == 14336) & (dados$BENCH == "hotspot"), "Size 2", as.character(dados$size))
dados$size <- ifelse((dados$size == 16384) & (dados$BENCH == "hotspot"), "Size 3", as.character(dados$size))
dados$size <- ifelse((dados$size == 18432) & (dados$BENCH == "hotspot"), "Size 4", as.character(dados$size))
dados$size <- ifelse((dados$size == 20480) & (dados$BENCH == "hotspot"), "Size 5", as.character(dados$size))

dados_stat <- ddply(dados, entry, summarise, 
                     n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                     ci=se*qt(.95/2 + .5, n-1))
# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

ggplot(dados_stat, aes(x=thread, y=mean, color=factor(size), shape=factor(size), linetype=factor(size)))+
    #geom_errorbar(aes(ymin=mean-ci, ymax=mean+ci), color="black", width=.1, position=pd) +
    geom_point() +
    geom_line() +
    theme_bw() +
    scale_colour_discrete(name="Runtime\nBackend") +
    scale_shape_discrete(name="Runtime\nBackend") +
    scale_linetype_discrete(name="Runtime\nBackend") +
    theme(axis.text.x = element_text(size=8), axis.text.y= element_text(size=8)) + 
    xlab("Threads") +
    ylab("Speedup") + 
    scale_x_continuous(limits=c(21,28), breaks=seq(22,28,1)) +
    #scale_y_continuous(limits=c(0,110), breaks=seq(0,100,10)) +
    facet_grid(BENCH ~ gpus, scales = "free_y")