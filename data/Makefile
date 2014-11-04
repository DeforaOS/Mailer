SUBDIRS	= 16x16 24x24 48x48
MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f
TARGETS	= $(OBJDIR)Mailer.pc
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

$(OBJDIR)Mailer.pc: Mailer.pc.in ../config.sh
	./pkgconfig.sh -P "$(PREFIX)" -- "$(OBJDIR)Mailer.pc"

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(Mailer.pc_OBJS)
	./pkgconfig.sh -c -P "$(PREFIX)" -- "Mailer.pc"

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(Mailer.pc_OBJS)
	./pkgconfig.sh -c -P "$(PREFIX)" -- "Mailer.pc"
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	./pkgconfig.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)Mailer.pc"
	$(MKDIR) $(DESTDIR)$(PREFIX)/share/applications
	$(INSTALL) -m 0644 deforaos-mailer.desktop $(DESTDIR)$(PREFIX)/share/applications/deforaos-mailer.desktop

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	./pkgconfig.sh -P "$(DESTDIR)$(PREFIX)" -u -- "Mailer.pc"
	$(RM) -- $(DESTDIR)$(PREFIX)/share/applications/deforaos-mailer.desktop

.PHONY: all subdirs clean distclean install uninstall
