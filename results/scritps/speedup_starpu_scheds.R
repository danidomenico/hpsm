library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../dados/idcin-2/starpu_sched/data_R_starpu_sched.csv", header=TRUE, sep=";")
serial <- dados_all[grep("^SERIAL$",dados_all$backend),]
serial_stat <- ddply(serial, entry, summarise,
                     n=length(time), mean=mean(time), sd=sd(time),
                     se=sd/sqrt(n), ci=se*qt(.95/2 + .5, n-1))

# calculate speedup
dados <- dados_all[grep("SERIAL", dados_all$backend, invert=TRUE),] #eliminate SERIAL
dados <- dados[((dados$BENCH == "hotspot") & (dados$size == 16384) & (dados$block_size == 1024)) | 
					((dados$BENCH == "nbody") & (dados$size == 98304) & (dados$block_size == 2048)) |
					((dados$BENCH == "cfd") & (dados$size == 131072) & (dados$block_size == 2048)),]
dados <- dados[(dados$thread != 2) & (dados$thread != 4) &
					(dados$thread != 6) & (dados$thread != 8) & 
					(dados$thread != 10) & (dados$thread != 12) & 
					(dados$thread != 14) & (dados$thread != 16) &
					(dados$thread != 25) & (dados$thread != 27),]
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
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_dmda", "dmda", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_dm", "dm", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_dmdar", "dmdar", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU_dmdas", "dmdas", as.character(dados$backend))
# renamete thread 0
dados[dados$thread == 0, "thread"] <- as.integer(16)

dados_stat <- ddply(dados, entry, summarise, 
                     n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                     ci=se*qt(.95/2 + .5, n-1))

# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

ggplot(dados_stat, aes(x=thread, y=mean, color=factor(backend), shape=factor(backend), linetype=factor(backend)))+
    geom_errorbar(aes(ymin=mean-ci, ymax=mean+ci), color="black", width=.1, position=pd) +
    geom_point() +
    geom_line() +
    theme_bw() +
    scale_colour_discrete(name="Runtime\nBackend") +
    scale_shape_discrete(name="Runtime\nBackend") +
    scale_linetype_discrete(name="Runtime\nBackend") +
    theme(axis.text.x = element_text(size=8), axis.text.y= element_text(size=8)) + 
    xlab("Threads") +
    ylab("Speedup") + 
    scale_x_continuous(limits=c(15,28), breaks=c(16,18,20,22,24,26,28), labels=c("0", "18", "20", "22", "24", "26", "28")) +
    #scale_y_continuous(limits=c(0,110), breaks=seq(0,100,10)) +
    facet_grid(BENCH ~ gpus, scales = "free_y")