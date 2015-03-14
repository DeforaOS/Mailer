TARGETS	= $(OBJDIR)imap4.so $(OBJDIR)mbox.so $(OBJDIR)pop3.so $(OBJDIR)nntp.so $(OBJDIR)rss.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W -fPIC `pkg-config --cflags openssl` `pkg-config --cflags glib-2.0`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs openssl` -lssl `pkg-config --libs glib-2.0`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

imap4_OBJS = $(OBJDIR)imap4.o
imap4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
imap4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

$(OBJDIR)imap4.so: $(imap4_OBJS)
	$(CCSHARED) -o $(OBJDIR)imap4.so $(imap4_OBJS) $(imap4_LDFLAGS)

mbox_OBJS = $(OBJDIR)mbox.o
mbox_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
mbox_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)mbox.so: $(mbox_OBJS)
	$(CCSHARED) -o $(OBJDIR)mbox.so $(mbox_OBJS) $(mbox_LDFLAGS)

pop3_OBJS = $(OBJDIR)pop3.o
pop3_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
pop3_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs libSystem`

$(OBJDIR)pop3.so: $(pop3_OBJS)
	$(CCSHARED) -o $(OBJDIR)pop3.so $(pop3_OBJS) $(pop3_LDFLAGS)

nntp_OBJS = $(OBJDIR)nntp.o
nntp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
nntp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)nntp.so: $(nntp_OBJS)
	$(CCSHARED) -o $(OBJDIR)nntp.so $(nntp_OBJS) $(nntp_LDFLAGS)

rss_OBJS = $(OBJDIR)rss.o
rss_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
rss_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)rss.so: $(rss_OBJS)
	$(CCSHARED) -o $(OBJDIR)rss.so $(rss_OBJS) $(rss_LDFLAGS)

$(OBJDIR)imap4.o: imap4.c ../../include/Mailer.h common.c
	$(CC) $(imap4_CFLAGS) `pkg-config --cflags libSystem` -o $(OBJDIR)imap4.o -c imap4.c

$(OBJDIR)mbox.o: mbox.c ../../include/Mailer.h
	$(CC) $(mbox_CFLAGS) -o $(OBJDIR)mbox.o -c mbox.c

$(OBJDIR)pop3.o: pop3.c ../../include/Mailer.h common.c
	$(CC) $(pop3_CFLAGS) `pkg-config --cflags libSystem` -o $(OBJDIR)pop3.o -c pop3.c

$(OBJDIR)nntp.o: nntp.c ../../include/Mailer.h
	$(CC) $(nntp_CFLAGS) -o $(OBJDIR)nntp.o -c nntp.c

$(OBJDIR)rss.o: rss.c ../../include/Mailer.h
	$(CC) $(rss_CFLAGS) -o $(OBJDIR)rss.o -c rss.c

clean:
	$(RM) -- $(imap4_OBJS) $(mbox_OBJS) $(pop3_OBJS) $(nntp_OBJS) $(rss_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Mailer/account
	$(INSTALL) -m 0755 $(OBJDIR)imap4.so $(DESTDIR)$(LIBDIR)/Mailer/account/imap4.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Mailer/account
	$(INSTALL) -m 0755 $(OBJDIR)mbox.so $(DESTDIR)$(LIBDIR)/Mailer/account/mbox.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/Mailer/account
	$(INSTALL) -m 0755 $(OBJDIR)pop3.so $(DESTDIR)$(LIBDIR)/Mailer/account/pop3.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/Mailer/account/imap4.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Mailer/account/mbox.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/Mailer/account/pop3.so

.PHONY: all clean distclean install uninstall
