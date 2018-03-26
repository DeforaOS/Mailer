DeforaOS Mailer
===============

About Mailer
------------

Mailer is a mail client application for the DeforaOS desktop.

Mailer is part of the DeforaOS Project, and distributed under the terms of the
BSD License (2-clause).


Compiling Mailer
----------------

Mailer depends on the following components:

 * Gtk+ 2 or 3
 * OpenSSL
 * DeforaOS libDesktop
 * DocBook-XSL (for the manual pages)
 * GTK-Doc (for the API documentation)

With GCC, this should then be enough to compile and install Mailer:

    $ make install

To install (or package) Mailer in a different location:

    $ make PREFIX="/another/prefix" install

Mailer also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install


Documentation
-------------

Manual pages for each of the executables installed are available in the `doc`
folder. They are written in the DocBook-XML format, and need libxslt and
DocBook-XSL to be installed for conversion to the HTML or man file format.

Likewise, the API reference for libMailer (accounts and plug-ins) is available
in the `doc/gtkdoc` folder, and is generated using gtk-doc.
