library(ggplot2)
library(plyr)
library(data.table)
library(RColorBrewer) 

# Defining non-computation states:
#def_states<-c("Initializing","Deinitializing","Overhead","Nothing","Sleeping","Freeing","Allocating","WritingBack","FetchingInput","PushingOutput","Callback","Progressing","Unpartitioning","AllocatingReuse","Reclaiming","DriverCopy","DriverCopyAsync","Scheduling","Executing")
def_states<-c("Initializing","Deinitializing","Overhead","Nothing","Freeing","Allocating","WritingBack","PushingOutput","Callback","Progressing","Unpartitioning","Reclaiming","Scheduling")

# Function for reading .csv file
read_df <- function(file,range1,range2) {
	df<-read.csv(file, header=FALSE, strip.white=TRUE)
	names(df) <- c("Nature","ResourceId","Type","Start","End","Duration", "Depth", "Value")
	df = df[!(names(df) %in% c("Nature","Type", "Depth"))]
	df$Origin<-as.factor(as.character(file))

	# Changing names if needed:
	df$Value <- as.character(df$Value)
	df$Value <- ifelse(df$Value == "F", "Freeing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "A", "Allocating", as.character(df$Value))
	df$Value <- ifelse(df$Value == "W", "WritingBack", as.character(df$Value))
	df$Value <- ifelse(df$Value == "No", "Nothing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "I", "Initializing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "D", "Deinitializing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Fi", "FetchingInput", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Po", "PushingOutput", as.character(df$Value))
	df$Value <- ifelse(df$Value == "C", "Callback", as.character(df$Value))
	df$Value <- ifelse(df$Value == "B", "Overhead", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Sc", "Scheduling", as.character(df$Value))
	df$Value <- ifelse(df$Value == "E", "Executing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Sl", "Sleeping", as.character(df$Value))
	df$Value <- ifelse(df$Value == "P", "Progressing", as.character(df$Value))
	df$Value <- ifelse(df$Value == "U", "Unpartitioning", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Ar", "AllocatingReuse", as.character(df$Value))
	df$Value <- ifelse(df$Value == "R", "Reclaiming", as.character(df$Value))
	df$Value <- ifelse(df$Value == "Co", "DriverCopy", as.character(df$Value))
	df$Value <- ifelse(df$Value == "CoA", "DriverCopyAsync", as.character(df$Value))
	df$Value <- ifelse(df$Value == "11funcHotspot", "0-Hotspot", as.character(df$Value))

	# Small cleanup
	df$Start<-round(df$Start,digit=1)
	df$End<-round(df$End,digit=1)
	df$ResourceId<-as.factor(df$ResourceId)
	df$Value<-as.factor(df$Value)

	# Start from zero
	m <- min(df$Start)
	df$Start <- df$Start - m
	df$End <- df$Start+df$Duration

	# Return data frame
	df
}

input_traces <- c("../../dados/idcin-2/rastros/hotspot_1GPU_26CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_1GPU_27CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_2GPU_26CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_2GPU_25CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_3GPU_25CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_3GPU_24CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_4GPU_24CPU.trace.csv",
                  "../../dados/idcin-2/rastros/hotspot_4GPU_23CPU.trace.csv")

idx <- 1
for (i in 1:(length(input_traces)/2)) {
	df<-data.frame()
	
	dfs<-read_df(input_traces[idx])
	df<-rbindlist(list(df,dfs))
  
	dfs<-read_df(input_traces[idx+1])
	df<-rbindlist(list(df,dfs))

	# rename origin
	#df[df$Origin == 'paje_native_hybrid.csv',"Origin"] <- "Native"
	df[grep("1GPU_27CPU", df$Origin), "Origin"] <- "1 GPU + 27 threads"
	df[grep("1GPU_26CPU", df$Origin), "Origin"] <- "1 GPU + 26 threads"
	df[grep("2GPU_26CPU", df$Origin), "Origin"] <- "2 GPUs + 26 threads"
	df[grep("2GPU_25CPU", df$Origin), "Origin"] <- "2 GPUs + 25 threads"
	df[grep("3GPU_25CPU", df$Origin), "Origin"] <- "3 GPUs + 25 threads"
	df[grep("3GPU_24CPU", df$Origin), "Origin"] <- "3 GPUs + 24 threads"
	df[grep("4GPU_24CPU", df$Origin), "Origin"] <- "4 GPUs + 24 threads"
	df[grep("4GPU_23CPU", df$Origin), "Origin"] <- "4 GPUs + 23 threads"

	# Color palettes
	colourCount = length(unique(df$Value))
	getPalette = colorRampPalette(brewer.pal(9, "Set1"))

	# Order of Value so we can have good colors
	ker_states<-as.character(unique(df[!(df$Value %in% def_states),Value]))
	ordered_states<-append(sort(ker_states), def_states)
	df$Value <- factor(df$Value, levels=ordered_states)

	# Order of ResourceId so we can have y-axis
	df$ResourceId <- factor(df$ResourceId, levels=sort(as.character(unique(df$ResourceId))))

	# Select only computation kernels
	df1 <- df[!(df$Value %in% def_states),]

	# Start from zero
	m <- min(df1$Start)
	df1$Start <- df1$Start - m
	df1$End <- df1$Start+df1$Duration

	# Plot
	plot <- ggplot(df1,aes(x=Start,xend=End, y=factor(ResourceId), yend=factor(ResourceId),color=Value)) + 
		theme_bw() + 
      #scale_color_manual(name="State",values=getPalette(colourCount)) + 
		scale_color_manual(name="Estados", values=c("#FF0000", "#4B0082", "#00FF00", "#0000FF", 
																  "#FF1493", "#FFFF00", "#FFA500", "#2F4F4F", 
																  "#006400", "#000000", "#1E90FF")) +
		geom_segment(size=4) + 
		ylab("Recurso") + 
		xlab("Tempo (ms)") + 
		scale_x_continuous(breaks=seq(0,10000,1500)) +
		facet_wrap(~Origin,ncol=1,scale="free_y")
	
	name <- paste("hotspot", idx-(i-1), "GPU.pdf", sep="_")
	#print(name)
	
	ggsave(filename=name, plot=plot, height=10, width=9)

	idx <- idx+2
}