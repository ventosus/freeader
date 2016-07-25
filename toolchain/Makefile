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
PIXEL_X := 800
PIXEL_Y := 600
WIDTH := 88.0
HEIGHT := 66.0
DPI := 231
TEMPLATE := good_4.3.tex

BOOKS := flatland.pig
BOOKS += voyage.pig
BOOKS += zhenskoe.pig
BOOKS += menu.pig

all:	freeader_emu freeader_enc freeader_gen $(BOOKS)

voyage.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/4791/pg4791.html

# get source html from Project Gutenberg
flatland.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/97/pg97.html

zhenskoe.html:
	wget -O $@ https://www.gutenberg.org/cache/epub/37196/pg37196.html

# convert html to tex
flatland.tex:	flatland.html $(TEMPLATE)
	pandoc \
		--template=$(TEMPLATE) \
		--variable=lang:english \
		--variable=title:"Flatland" \
		--variable=author:"Edwin Abbott" \
		-o $@ $<

# convert html to tex
voyage.tex:	voyage.html $(TEMPLATE)
	pandoc \
		--template=$(TEMPLATE) \
		--variable=lang:french \
		--variable=title:"Voyage au centre de la terre" \
		--variable=author:"Jules Verne" \
		-o $@ $<

# convert html to tex
zhenskoe.tex:	zhenskoe.html $(TEMPLATE)
	pandoc \
		--template=$(TEMPLATE) \
		--variable=lang:russian \
		--variable=title:"Zhenskoe Mezhdunarodnoe Dvizhenie Sbornik Statei" \
		--variable=author:"Various" \
		-o $@ $<

# render tex to dvi
%.dvi:	%.tex
	latex $<

# render pdf from preview
%.pdf:	%.tex
	xelatex $<

menu.pig: freeader_gen theme.edj
	$(eval TMP := $(shell mktemp -d))
	./freeader_gen $(TMP)/%08d.pbm
	./freeader_enc $@ $(TMP)/????????.pbm
	rm -rf $(TMP)

# create raw bilevel image
%.pig: %.pdf freeader_enc
	$(eval TMP := $(shell mktemp -d))
	pdftoppm $< -r $(DPI) -W $(PIXEL_X) -H $(PIXEL_Y) -mono -thinlinemode shape $(TMP)/frm_
	./freeader_enc $@ $(TMP)/*.pbm
	rm -rf $(TMP)

# create raw bilevel image
#%.pig: %.dvi freeader_enc
#	$(eval TMP := $(shell mktemp -d))
#	dvipng --gamma 1.0 -z 1 -D $(DPI) -T $(WIDTH)mm,$(HEIGHT)mm -O $(OFFSET_X)mm,$(OFFSET_Y)mm -o $(TMP)/%08d.png $<
#	./freeader_enc $@ $(TMP)/????????.png
#	rm -rf $(TMP)

freeader_emu:	freeader_emu.c freeader.h
	$(CC) -O3 -std=gnu11 -o $@ $< -I./ \
		$(shell pkg-config --cflags --libs elementary) -ljbig85

theme.edj: theme.edc
	edje_cc -fd ./ -id ./ $< $@

freeader_gen:	freeader_gen.c freeader.h theme.edj
	$(CC) -O3 -g -std=gnu11 -D_GNU_SOURCE=1 -o $@ $< -I./ \
		$(shell pkg-config --cflags --libs edje) \
		$(shell pkg-config --cflags --libs evas) \
		$(shell pkg-config --cflags --libs ecore) \
		$(shell pkg-config --cflags --libs ecore-evas) -ljbig85

freeader_enc:	freeader_enc.c freeader.h
	$(CC) -O3 -std=gnu11 -o $@ $< -I./ -lpng -ljbig85

clean: 
	rm -f *.html *.dvi *.pdf *.aux *.log *.out *.pig

.PHONY: clear	
