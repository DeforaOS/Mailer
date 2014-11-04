SUBDIRS	= gtkdoc
TARGETS	= $(OBJDIR)compose.1 $(OBJDIR)compose.html $(OBJDIR)gtkdoc/Mailer.types $(OBJDIR)gtkdoc/tmpl.stamp $(OBJDIR)gtkdoc/sgml.stamp $(OBJDIR)gtkdoc/html.stamp $(OBJDIR)mailer.1 $(OBJDIR)mailer.html
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

$(OBJDIR)compose.1: compose.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)compose.1"

$(OBJDIR)compose.html: compose.css.xml compose.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)compose.html"

$(OBJDIR)gtkdoc/Mailer.types: ../config.sh
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/Mailer.types"

$(OBJDIR)gtkdoc/tmpl.stamp: gtkdoc/Mailer.types
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/tmpl.stamp"

$(OBJDIR)gtkdoc/sgml.stamp: gtkdoc/tmpl.stamp
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/sgml.stamp"

$(OBJDIR)gtkdoc/html.stamp: gtkdoc/Mailer-docs.xml gtkdoc/sgml.stamp
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/html.stamp"

$(OBJDIR)mailer.1: mailer.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)mailer.1"

$(OBJDIR)mailer.html: mailer.css.xml mailer.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)mailer.html"

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(compose.1_OBJS) $(compose.html_OBJS) $(gtkdoc/Mailer.types_OBJS) $(gtkdoc/tmpl.stamp_OBJS) $(gtkdoc/sgml.stamp_OBJS) $(gtkdoc/html.stamp_OBJS) $(mailer.1_OBJS) $(mailer.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "compose.1"
	./docbook.sh -c -P "$(PREFIX)" -- "compose.html"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/Mailer.types"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/tmpl.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/sgml.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/html.stamp"
	./docbook.sh -c -P "$(PREFIX)" -- "mailer.1"
	./docbook.sh -c -P "$(PREFIX)" -- "mailer.html"

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(compose.1_OBJS) $(compose.html_OBJS) $(gtkdoc/Mailer.types_OBJS) $(gtkdoc/tmpl.stamp_OBJS) $(gtkdoc/sgml.stamp_OBJS) $(gtkdoc/html.stamp_OBJS) $(mailer.1_OBJS) $(mailer.html_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "compose.1"
	./docbook.sh -c -P "$(PREFIX)" -- "compose.html"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/Mailer.types"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/tmpl.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/sgml.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/html.stamp"
	./docbook.sh -c -P "$(PREFIX)" -- "mailer.1"
	./docbook.sh -c -P "$(PREFIX)" -- "mailer.html"
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)compose.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)compose.html"
	./gtkdoc.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)gtkdoc/html.stamp"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)mailer.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)mailer.html"

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "compose.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "compose.html"
	./gtkdoc.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gtkdoc/html.stamp"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "mailer.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "mailer.html"

.PHONY: all subdirs clean distclean install uninstall
