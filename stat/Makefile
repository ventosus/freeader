OFFSET_X := 25.4
OFFSET_Y := 24.6
THRESH := 255

# good displays 4.3"
WIDTH := 88.0
HEIGHT := 66.0
DPI := 231
TEMPLATE := good_4.3.tex

TARGETS := flatland.png flatland.pbm flatland.pbm.gz flatland.pbm.bz2 flatland.pbm.lzo flatland.pbm.xz flatland.group3.tiff flatland.group4.tiff flatland.jbig flatland.lossless.jbig2 flatland.lossy.jbig2

PNGS := $(wildcard png/00??.png)
OBJS := $(subst png/,cum/,$(PNGS))
OBJS := $(OBJS:.png=)

CUMS := $(OBJS:=.png)
CUMS += $(OBJS:=.pbm)
CUMS += $(OBJS:=.pbm.gz)
CUMS += $(OBJS:=.pbm.bz2)
CUMS += $(OBJS:=.pbm.lzo)
CUMS += $(OBJS:=.pbm.xz)
CUMS += $(OBJS:=.group3.tiff)
CUMS += $(OBJS:=.group4.tiff)
CUMS += $(OBJS:=.jbig)
CUMS += $(OBJS:=.lossless.jbig2)
CUMS += $(OBJS:=.lossy.jbig2)

all: stat.neg.png

# get source html from Project Gutenberg
flatland.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/97/pg97.html

# convert html to tex
%.tex: %.html $(TEMPLATE)
	pandoc \
		--template=$(TEMPLATE) \
		--variable=lang:english \
		--variable=title:"Flatland" \
		--variable=author:"Edwin Abbott" \
		-o $@ $<

# render tex to dvi
%.dvi: %.tex
	latex $<

# render dvi to sequence of png's
dir_png: flatland.dvi
	mkdir -p png
	dvipng --gamma 1.0 -z 0 -D $(DPI) -T $(WIDTH)mm,$(HEIGHT)mm -O $(OFFSET_X)mm,$(OFFSET_Y)mm -o png/%04d.png $<;

# combine png's into one
flatland.png:
	convert -append -define PNG:compression_level=9 -threshold 50\% png/00??.png $@

cum/%.png: png/%.png
	mkdir -p cum
	convert -define PNG:compression_level=9 -threshold 50\% $< $@

%.pbm:	%.png
	convert $< -threshold 50\% $@

%.pbm.gz:	%.pbm
	cat $< | gzip -9 > $@

%.pbm.bz2:	%.pbm
	cat $< | bzip2 -9 > $@

%.pbm.lzo:	%.pbm
	cat $< | lzop > $@

%.pbm.xz:	%.pbm
	cat $< | xz > $@

%.group3.tiff:	%.pbm
	convert $< -compress Fax $@

%.group4.tiff:	%.pbm
	convert $< -compress Group4 $@

%.jbig:	%.pbm
	pbmtojbg85 $< $@

%.lossless.jbig2:	%.pbm
	jbig2 $< > $@

%.lossy.jbig2:	%.pbm
	jbig2 -s $< > $@

stat.csv:	$(TARGETS)
	ls -l --sort size $+ | gawk '{print $$5", "$$9}' > $@

stat2.csv: $(CUMS)
	rm -rf stat2.csv;
	for suffix in png pbm pbm.gz pbm.bz2 pbm.lzo pbm.xz group3.tiff group4.tiff jbig lossless.jbig2 lossy.jbig2; do \
		du -bc cum/*.$$suffix | tail -1 | sed -e "s/\t.*/, $$suffix/g" >> $@; done

stat.png:	stat.csv stat2.csv stat.r
	Rscript stat.r

%.neg.png:	%.png
	convert -transparent white $< -negate $@

clean:
	rm -f flatland.* x.log png/*.png $(TARGETS) $(PNGS) $(CUMS) stat*.csv stat*.png

.PHONY: clear	
