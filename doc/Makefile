# Makefile for disktype documentation

FOP = ./fop.sh

SOURCE      = disktype.xml
TARGET_HTML = $(patsubst %.xml,%.html,$(SOURCE))
TARGET_FO   = $(patsubst %.xml,%.fo,$(SOURCE))
TARGET_PDF  = $(patsubst %.xml,%.pdf,$(SOURCE))

all: $(TARGET_HTML)

%.html: %.xml html.xsl
	xsltproc -o $@ html.xsl $<
#	xsltproc -o $@ /sw/share/xml/xsl/html/docbook.xsl $<
#	xsltproc -o $@ /sw/share/xml/xsl/html/chunk.xsl $<

%.fo: %.xml
	xsltproc -o $@ /sw/share/xml/xsl/fo/docbook.xsl $<

%.pdf: %.fo
	$(FOP) -fo $< -pdf $@

check:
	xmllint --noout --valid $(SOURCE)

clean:
	rm -f $(TARGET_HTML) $(TARGET_FO) $(TARGET_PDF)

# EOF
