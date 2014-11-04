TARGETS	= $(OBJDIR)date $(OBJDIR)email $(OBJDIR)imap4 $(OBJDIR)plugins $(OBJDIR)tests.log
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W
CFLAGS	= -Wall -g -O2 -ffreestanding
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

date_OBJS = $(OBJDIR)date.o
date_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
date_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)date: $(date_OBJS)
	$(CC) -o $(OBJDIR)date $(date_OBJS) $(date_LDFLAGS)

email_OBJS = $(OBJDIR)email.o
email_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
email_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -L ../src -Wl,-rpath,../src -lMailer

$(OBJDIR)email: $(email_OBJS)
	$(CC) -o $(OBJDIR)email $(email_OBJS) $(email_LDFLAGS)

imap4_OBJS = $(OBJDIR)imap4.o
imap4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem` `pkg-config --cflags openssl`
imap4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem` `pkg-config --libs openssl` -lssl

$(OBJDIR)imap4: $(imap4_OBJS)
	$(CC) -o $(OBJDIR)imap4 $(imap4_OBJS) $(imap4_LDFLAGS)

plugins_OBJS = $(OBJDIR)plugins.o
plugins_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags gtk+-2.0`
plugins_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs gtk+-2.0`

$(OBJDIR)plugins: $(plugins_OBJS)
	$(CC) -o $(OBJDIR)plugins $(plugins_OBJS) $(plugins_LDFLAGS)

$(OBJDIR)tests.log: date email imap4 plugins tests.sh
	./tests.sh -P "$(PREFIX)" -- "$(OBJDIR)tests.log"

$(OBJDIR)date.o: date.c ../src/helper.c
	$(CC) $(date_CFLAGS) -o $(OBJDIR)date.o -c date.c

$(OBJDIR)email.o: email.c ../src/libMailer.a
	$(CC) $(email_CFLAGS) -o $(OBJDIR)email.o -c email.c

$(OBJDIR)imap4.o: imap4.c ../src/account/imap4.c
	$(CC) $(imap4_CFLAGS) -o $(OBJDIR)imap4.o -c imap4.c

$(OBJDIR)plugins.o: plugins.c
	$(CC) $(plugins_CFLAGS) -o $(OBJDIR)plugins.o -c plugins.c

clean:
	$(RM) -- $(date_OBJS) $(email_OBJS) $(imap4_OBJS) $(plugins_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
