DeforaOS Mailer
===============

About Mailer
------------

Mailer is a mail client application for the DeforaOS desktop.

It supports local mail folders in the mbox format, as well as POP 3 and IMAP 4
servers, including connectivity over SSL. It is possible to access GMail
through its IMAP 4 gateway. It currently requires a functional local e-mail
service to send e-mails; this is performed through the `sendmail(1)` command.

Mailer is part of the DeforaOS Project, found at https://www.defora.org/.

Compiling Mailer
----------------

Mailer depends on the following components:

 * Gtk+ 2.4 or later, or Gtk+ 3.0 or later
 * OpenSSL
 * DeforaOS libDesktop
 * an implementation of `make`
 * gettext (libintl) for translations
 * DocBook-XSL (for the manual pages)
 * GTK-Doc (for the API documentation)

With these installed, the following command should be enough to compile Keyboard
on most systems:

    $ make

The following command will then install Mailer:

    $ make install

To install (or package) Mailer in a different location:

    $ make clean
    $ make PREFIX="/another/prefix" install

Mailer also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install

On some systems, the Makefiles shipped can be re-generated accordingly thanks to
the DeforaOS configure tool.

Documentation
-------------

Manual pages for each of the executables installed are available in the `doc`
folder. They are written in the DocBook-XML format, and need libxslt and
DocBook-XSL to be installed for conversion to the HTML or man file format.

Likewise, the API reference for libMailer (accounts and plug-ins) is available
in the `doc/gtkdoc` folder, and is generated using gtk-doc.

Distributing Mailer
-------------------

DeforaOS Mailer is subject to the terms of the 2-clause BSD license. Please see
the `COPYING` file for more information.
