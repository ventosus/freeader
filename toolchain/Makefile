CC ?= gcc

OFFSET_X := 25.4
OFFSET_Y := 24.6
THRESH := 255

# pervasive displays 4.41"
#WIDTH := 90.0
#HEIGHT := 67.5
#OFFSET := 25.5
#DPI := 113
#TEMPLATE := pervasive_4.41.tex

# good displays 4.3"
WIDTH := 88.0
HEIGHT := 66.0
DPI := 231
TEMPLATE := good_4.3.tex

BOOKS := flatland.pig
#BOOKS += voyage_au_centre_de_la_terre.pig

all:	freeader_emu freeader_enc $(BOOKS)

voyage_au_centre_de_la_terre.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/4791/pg4791.html

# get source html from Project Gutenberg
flatland.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/97/pg97.html

# convert html to tex
%.tex:	%.html $(TEMPLATE)
	pandoc \
		--template=$(TEMPLATE) \
		--variable=lang:english \
		--variable=title:"Flatland" \
		--variable=author:"Edwin Abbott" \
		-o $@ $<

# render tex to dvi
%.dvi:	%.tex
	latex $<

# render pdf from preview
%.pdf:	%.tex
	pdflatex $<

# create raw bilevel image
%.pig: %.dvi freeader_enc
	$(eval TMP := $(shell mktemp -d))
	dvipng --gamma 1.0 -z 1 -D $(DPI) -T $(WIDTH)mm,$(HEIGHT)mm -O $(OFFSET_X)mm,$(OFFSET_Y)mm -o $(TMP)/%08d.png $<
	./freeader_enc $@ $(TMP)/????????.png
	rm -rf $(TMP)

freeader_emu:	freeader_emu.c freeader.h
	$(CC) -O3 -std=gnu11 -o $@ $< -I./ $(shell pkg-config --cflags --libs elementary) -ljbig85

freeader_enc:	freeader_enc.c freeader.h
	$(CC) -O3 -std=gnu11 -o $@ $< -I./ -lpng -ljbig85

clean: 
	rm -f *.html *.dvi *.pdf *.aux *.log *.out *.pig

.PHONY: clear	
