library(ggplot2)
library(plyr)
entry <- c("BENCH", "backend", "size", "block_size", "thread", "gpus")
dados_all <- read.csv("../dados/idcin-2/data_R.csv", header=TRUE, sep=";")

# filters
dados <- dados_all[grep("SERIAL", dados_all$backend, invert=FALSE),] #keep just serial
dados <- dados[(dados$backend == "SERIAL"),]

# rename GPU and Backend label
dados[dados$gpus    == 0, "gpus"] <- as.character("Serial Executions")
dados$backend <- ifelse(dados$backend == "SERIAL", "Serial", as.character(dados$backend))
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
# rename sizes cfd
dados$size <- ifelse((dados$size == 98304) &  (dados$BENCH == "cfd"), "Size 1", as.character(dados$size))
dados$size <- ifelse((dados$size == 114688) & (dados$BENCH == "cfd"), "Size 2", as.character(dados$size))
dados$size <- ifelse((dados$size == 131072) & (dados$BENCH == "cfd"), "Size 3", as.character(dados$size))
dados$size <- ifelse((dados$size == 147456) & (dados$BENCH == "cfd"), "Size 4", as.character(dados$size))
dados$size <- ifelse((dados$size == 163840) & (dados$BENCH == "cfd"), "Size 5", as.character(dados$size))

dados_stat <- ddply(dados, entry, summarise, 
                    n=length(time), mean=mean(time), sd=sd(time),
                    se=sd/sqrt(n), ci=se*qt(.95/2 + .5, n-1))
                    
# The errorbars overlapped, so use position_dodge to move them horizontally
pd <- position_dodge(0.1) # move them .05 to the left and right

ggplot(dados_stat, aes(x=size, y=mean, color=factor(backend), shape=factor(backend), linetype=factor(backend), group=1))+
    geom_errorbar(aes(ymin=mean-ci, ymax=mean+ci), color="black", width=.1, position=pd) +
    geom_point() +
    geom_line() +
    theme_bw() +
    scale_colour_discrete(name="Runtime\nBackend") +
    scale_shape_discrete(name="Runtime\nBackend") +
    scale_linetype_discrete(name="Runtime\nBackend") +
    theme(axis.text.x = element_text(size=8), axis.text.y= element_text(size=8), legend.position="top") + 
    xlab("Sizes") +
    ylab("Time (s)") + 
    #scale_x_continuous(labels=c("", "", "", "", "")) +
    #scale_y_continuous(limits=c(0,110), breaks=seq(0,100,10)) +
    facet_grid(BENCH~gpus, scales = "free_y")