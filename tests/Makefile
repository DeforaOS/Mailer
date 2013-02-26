TARGETS	= date email imap4 tests.log
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

date_OBJS = date.o
date_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
date_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

date: $(date_OBJS)
	$(CC) -o date $(date_OBJS) $(date_LDFLAGS)

email_OBJS = email.o
email_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
email_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -L ../src -Wl,-rpath,../src -lMailer

email: $(email_OBJS)
	$(CC) -o email $(email_OBJS) $(email_LDFLAGS)

imap4_OBJS = imap4.o
imap4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS) `pkg-config --cflags glib-2.0 libSystem` `pkg-config --cflags openssl`
imap4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) `pkg-config --libs glib-2.0 libSystem` `pkg-config --libs openssl` -lssl

imap4: $(imap4_OBJS)
	$(CC) -o imap4 $(imap4_OBJS) $(imap4_LDFLAGS)

tests.log: date email imap4
	./tests.sh -P "$(PREFIX)" -- "tests.log"

date.o: date.c ../src/helper.c
	$(CC) $(date_CFLAGS) -c date.c

email.o: email.c ../src/libMailer.a
	$(CC) $(email_CFLAGS) -c email.c

imap4.o: imap4.c ../src/account/imap4.c
	$(CC) $(imap4_CFLAGS) -c imap4.c

clean:
	$(RM) -- $(date_OBJS) $(email_OBJS) $(imap4_OBJS) $(tests.log_OBJS)
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
