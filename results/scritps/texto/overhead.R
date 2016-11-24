library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../../dados/idcin-2/data_R.csv", header=TRUE, sep=";")

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
    dados[i,]$speedup <-  dados[i,]$time / tserial
}

# rename backends
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_STARPU", "StarPU", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_OPENMP", "OpenMP", as.character(dados$backend))
dados$backend <- ifelse(dados$backend == "PARALLEL_BACK_SERIAL", "Serial", as.character(dados$backend))

# rename benchmarks
dados$BENCH <- ifelse(dados$BENCH == "cfd",     "CFD  ",     as.character(dados$BENCH))
dados$BENCH <- ifelse(dados$BENCH == "nbody",   "N-Body  ",  as.character(dados$BENCH))
dados$BENCH <- ifelse(dados$BENCH == "hotspot", "Hotspot  ", as.character(dados$BENCH))

dados_stat <- ddply(dados, entry, summarise, 
                    n=length(speedup), mean=mean(speedup), sd=sd(speedup), se=sd/sqrt(n),
                    ci=se*qt(.95/2 + .5, n-1))

#dados_stat                   
# The errorbars overlapped, so use position_dodge to move them horizontally
#pd <- position_dodge(0.1) # move them .05 to the left and right

my_plot <- ggplot(dados_stat, aes(x=factor(backend), fill=BENCH, y=mean))+
    geom_bar(stat="identity", position="dodge") +
    coord_flip() +
    theme_bw() +
    scale_fill_discrete(name="Aplicações:  ") +
    theme(text = element_text(size=11), 
         legend.position="top", legend.text=element_text(size=9)) + 
    scale_y_continuous(limits=c(0,1.2), breaks=seq(0,1.2,0.1)) +
    xlab("Back-ends") +
    ylab("Tempo HPSM / Tempo sequencial")  
    
#ggsave(file="chart.pdf", plot=my_plot, height=3.5, width=2.5)
ggsave(file="chart.pdf", plot=my_plot, height=3.0, width=5.0)