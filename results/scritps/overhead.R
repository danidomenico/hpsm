library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../dados/idcin-2/data_R.csv", header=TRUE, sep=";")
serial <- dados_all[grep("^SERIAL$",dados_all$backend),]

serial_stat <- ddply(serial, entry, summarise,
                     n=length(time), mean=mean(time), sd=sd(time),
                     se=sd/sqrt(n), ci=se*qt(.95/2 + .5, n-1))

#eliminate SERIAL                     
dados <- dados_all[dados_all$backend != "SERIAL",] 
# only 1 cpu executions
dados <- dados[dados$thread == 1,]
dados <- dados[dados$gpus == 0,]

#calculate speedup
dados$speedup <- 1:nrow(dados)
for(i in 1:nrow(dados)) {
    name <- dados[i,]$BENCH
    sz <- dados[i,]$size
    tserial <- serial_stat[(serial_stat$BENCH == name) & (serial_stat$size == sz),]$mean
    dados[i,]$speedup <-  tserial / dados[i,]$time
}

# rename backends
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU", "StarPU", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_OPENMP", "OpenMP", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_SERIAL", "Sequencial", as.character(dados$backend))

dados_stat <- ddply(dados, entry, summarise, 
                    n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                    ci=se*qt(.95/2 + .5, n-1))

#dados_stat                   
# The errorbars overlapped, so use position_dodge to move them horizontally
#pd <- position_dodge(0.1) # move them .05 to the left and right

ggplot(dados_stat, aes(x=factor(backend), fill=BENCH, y=mean))+
    geom_bar(stat="identity", position="dodge") +
    theme_bw() +
    scale_y_continuous(limits=c(0,1.1), breaks=seq(0,1.1,0.1)) +
    xlab("Back-end") +
    ylab("Time Serial/Time Parallel")  