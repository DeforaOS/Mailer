SUBDIRS	= account
TARGETS	= $(OBJDIR)libMailer.a $(OBJDIR)libMailer.so.0.0 $(OBJDIR)libMailer.so.0 $(OBJDIR)libMailer.so $(OBJDIR)compose $(OBJDIR)mailer
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libDesktop` -lintl
LDFLAGS	= 
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

libMailer_OBJS = $(OBJDIR)account.o $(OBJDIR)callbacks.o $(OBJDIR)compose.o $(OBJDIR)folder.o $(OBJDIR)helper.o $(OBJDIR)mailer.o $(OBJDIR)message.o
libMailer_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) -fPIC
libMailer_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs openssl` -lssl

$(OBJDIR)libMailer.a: $(libMailer_OBJS)
	$(AR) -rc $(OBJDIR)libMailer.a $(libMailer_OBJS)
	$(RANLIB) $(OBJDIR)libMailer.a

$(OBJDIR)libMailer.so.0.0 $(OBJDIR)libMailer.so.0 $(OBJDIR)libMailer.so: $(libMailer_OBJS)
	$(CCSHARED) -o $(OBJDIR)libMailer.so.0.0 -Wl,-soname,libMailer.so.0 $(libMailer_OBJS) $(libMailer_LDFLAGS)
	$(LN) -s -- libMailer.so.0.0 $(OBJDIR)libMailer.so.0
	$(LN) -s -- libMailer.so.0.0 $(OBJDIR)libMailer.so

compose_OBJS = $(OBJDIR)compose-main.o
compose_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
compose_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs openssl` -lssl -L. -Wl,-rpath,$(LIBDIR) -lMailer

$(OBJDIR)compose: $(compose_OBJS) libMailer.a
	$(CC) -o $(OBJDIR)compose $(compose_OBJS) $(compose_LDFLAGS)

mailer_OBJS = $(OBJDIR)main.o
mailer_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
mailer_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs openssl` -lssl -L. -Wl,-rpath,$(LIBDIR) -lMailer

$(OBJDIR)mailer: $(mailer_OBJS) libMailer.a
	$(CC) -o $(OBJDIR)mailer $(mailer_OBJS) $(mailer_LDFLAGS)

$(OBJDIR)account.o: account.c mailer.h message.h account.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(libMailer_CFLAGS) -o $(OBJDIR)account.o -c account.c

$(OBJDIR)callbacks.o: callbacks.c account.h callbacks.h compose.h mailer.h message.h gtkassistant.c ../config.h
	$(CC) $(libMailer_CFLAGS) -o $(OBJDIR)callbacks.o -c callbacks.c

$(OBJDIR)compose.o: compose.c callbacks.h common.c mailer.h message.h compose.h ../config.h
	$(CC) $(libMailer_CFLAGS) -o $(OBJDIR)compose.o -c compose.c

$(OBJDIR)folder.o: folder.c mailer.h folder.h
	$(CC) $(libMailer_CFLAGS) -o $(OBJDIR)folder.o -c folder.c

$(OBJDIR)helper.o: helper.c ../include/Mailer/helper.h
	$(CC) $(libMailer_CFLAGS) -o $(OBJDIR)helper.o -c helper.c

$(OBJDIR)mailer.o: mailer.c account.h callbacks.h common.c compose.h message.h mailer.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(libMailer_CFLAGS) -o $(OBJDIR)mailer.o -c mailer.c

$(OBJDIR)message.o: message.c
	$(CC) $(libMailer_CFLAGS) -o $(OBJDIR)message.o -c message.c

$(OBJDIR)compose-main.o: compose-main.c compose.h message.h
	$(CC) $(compose_CFLAGS) -o $(OBJDIR)compose-main.o -c compose-main.c

$(OBJDIR)main.o: main.c mailer.h ../config.h
	$(CC) -D PREFIX=\"$(PREFIX)\" $(mailer_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(libMailer_OBJS) $(compose_OBJS) $(mailer_OBJS)

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(libMailer_OBJS) $(compose_OBJS) $(mailer_OBJS)
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	$(MKDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0644 $(OBJDIR)libMailer.a $(DESTDIR)$(LIBDIR)/libMailer.a
	$(INSTALL) -m 0755 $(OBJDIR)libMailer.so.0.0 $(DESTDIR)$(LIBDIR)/libMailer.so.0.0
	$(LN) -s -- libMailer.so.0.0 $(DESTDIR)$(LIBDIR)/libMailer.so.0
	$(LN) -s -- libMailer.so.0.0 $(DESTDIR)$(LIBDIR)/libMailer.so
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)compose $(DESTDIR)$(BINDIR)/compose
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)mailer $(DESTDIR)$(BINDIR)/mailer

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	$(RM) -- $(DESTDIR)$(LIBDIR)/libMailer.a
	$(RM) -- $(DESTDIR)$(LIBDIR)/libMailer.so.0.0
	$(RM) -- $(DESTDIR)$(LIBDIR)/libMailer.so.0
	$(RM) -- $(DESTDIR)$(LIBDIR)/libMailer.so
	$(RM) -- $(DESTDIR)$(BINDIR)/compose
	$(RM) -- $(DESTDIR)$(BINDIR)/mailer

.PHONY: all subdirs clean distclean install uninstall
