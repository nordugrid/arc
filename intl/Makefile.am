LANGUAGES = sv ru

TRANSLATIONS = $(foreach lang, $(LANGUAGES), Arc.$(lang).po)

SRC_FILES = $(shell find ../src -name *.cpp -o -name *.h | sort)

localedir = $(prefix)/share/locale

all-local:  $(patsubst %.po,%.mo,$(TRANSLATIONS))

%.mo: %.po
	msgfmt -o $@ $^

$(TRANSLATIONS): Arc.pot
	if test ! -f $@ ; then rm -rf $@; cp template.po $@ ; fi
	cp $@ $@.old
	rm -rf $@
	msgmerge $@.old $< -o $@
	rm -f $@.old

Arc.pot: $(SRC_FILES)
	xgettext -d Arc -o Arc.pot -kmsg:2 $^

install-exec-hook:
	@for lang in $(LANGUAGES) ; do \
		dir=$(localedir)/$${lang}/LC_MESSAGES ; \
		mkdir -p $(DESTDIR)$${dir} && \
		cp Arc.$${lang}.mo $(DESTDIR)$${dir}/Arc.mo ; \
	done

uninstall-local:
	@for lang in $(LANGUAGES) ; do \
		dir=$(localedir)/$${lang}/LC_MESSAGES ; \
		rm -f $${dir}/Arc.mo ; \
	done

clean-local:
	rm -f *.mo
	rm -f *.pot

EXTRA_DIST = $(TRANSLATIONS) template.po

MAINTAINERCLEANFILES = $(TRANSLATIONS)
