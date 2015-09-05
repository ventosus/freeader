stat <- read.csv('stat.csv', header=FALSE, sep=',', stringsAsFactors=FALSE)
stat2 <- read.csv('stat2.csv', header=FALSE, sep=',', stringsAsFactors=FALSE)

# stat
fmt <- gsub('flatland.', '', stat[,2])
tot <- stat[1,1]
perc <- round(stat[,1] / tot * 100, 1)

# stat2
ord <- order(stat2[,1], decreasing=TRUE)
stat2 <- stat2[ord,]
tot2 <- stat2[1,1]
perc2 <- round(stat2[,1] / tot2 * 100, 1)

png('stat.png', width=800, height=1600, pointsize=20)
	par(mfcol=c(2,1))

	midpoints <- barplot(perc, names.arg=fmt, las=3, cex.names=0.8, col=1,
		ylab='% of pbm size', ylim=c(0,110), main='1 x 99 pages per file')
	text(midpoints, perc + 5, perc)

	midpoints <- barplot(perc2, names.arg=stat2[,2], las=3, cex.names=0.8, col=1,
		ylab='% of pbm size', ylim=c(0,110), main='99 x 1 page per file')
	text(midpoints, perc2 + 5, perc2)

dev.off()
