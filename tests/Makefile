TARGETS	= date email tests.log
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
CC	?= cc
CPPFLAGSF= -I ../include
CPPFLAGS?=
CFLAGSF	= -W
CFLAGS	= -Wall -g -O2 -ffreestanding
LDFLAGSF= -L ../src -Wl,-rpath,../src -lMailer
LDFLAGS	= 
RM	?= rm -f
LN	?= ln -f
MKDIR	?= mkdir -p
INSTALL	?= install


all: $(TARGETS)

date_OBJS = date.o
date_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
date_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

date: $(date_OBJS)
	$(CC) -o date $(date_OBJS) $(date_LDFLAGS)

email_OBJS = email.o
email_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
email_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

email: $(email_OBJS)
	$(CC) -o email $(email_OBJS) $(email_LDFLAGS)

tests.log: email
	./tests.sh -P "$(PREFIX)" -- "tests.log"

date.o: date.c ../src/libMailer.a
	$(CC) $(date_CFLAGS) -c date.c

email.o: email.c ../src/libMailer.a
	$(CC) $(email_CFLAGS) -c email.c

clean:
	$(RM) -- $(date_OBJS) $(email_OBJS) $(tests.log_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
