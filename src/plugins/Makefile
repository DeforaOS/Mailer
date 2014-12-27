TARGETS	= $(OBJDIR)mailing-lists.so $(OBJDIR)search.so $(OBJDIR)template.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W -fPIC `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs openssl` -lssl `pkg-config --libs libDesktop`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

mailing-lists_OBJS = $(OBJDIR)mailing-lists.o
mailing-lists_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
mailing-lists_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -L.. -Wl,-rpath,$(LIBDIR) -lMailer

$(OBJDIR)mailing-lists.so: $(mailing-lists_OBJS)
	$(CCSHARED) -o $(OBJDIR)mailing-lists.so $(mailing-lists_OBJS) $(mailing-lists_LDFLAGS)

search_OBJS = $(OBJDIR)search.o
search_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
search_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)search.so: $(search_OBJS)
	$(CCSHARED) -o $(OBJDIR)search.so $(search_OBJS) $(search_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

$(OBJDIR)mailing-lists.o: mailing-lists.c ../../include/Mailer.h
	$(CC) $(mailing-lists_CFLAGS) -o $(OBJDIR)mailing-lists.o -c mailing-lists.c

$(OBJDIR)search.o: search.c ../../include/Mailer.h
	$(CC) $(search_CFLAGS) -o $(OBJDIR)search.o -c search.c

$(OBJDIR)template.o: template.c ../../include/Mailer.h
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

clean:
	$(RM) -- $(mailing-lists_OBJS) $(search_OBJS) $(template_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Mailer/plugins
	$(INSTALL) -m 0755 $(OBJDIR)mailing-lists.so $(DESTDIR)$(LIBDIR)/Mailer/plugins/mailing-lists.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Mailer/plugins
	$(INSTALL) -m 0755 $(OBJDIR)search.so $(DESTDIR)$(LIBDIR)/Mailer/plugins/search.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Mailer/plugins/mailing-lists.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Mailer/plugins/search.so

.PHONY: all clean distclean install uninstall
