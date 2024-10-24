/* $Id$ */
/* Copyright (c) 2006-2024 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <libintl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <gdk/gdkkeysyms.h>
#include <Desktop.h>
#include "../include/Mailer/plugin.h"
#include "folder.h"
#include "message.h"
#include "compose.h"
#include "callbacks.h"
#include "mailer.h"
#include "../config.h"

#define _(string) gettext(string)
#define N_(string) (string)
#include "common.c"


/* constants */
#ifndef PROGNAME_MAILER
# define PROGNAME_MAILER	"mailer"
#endif
#ifndef PREFIX
# define PREFIX			"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR			PREFIX "/lib"
#endif
#ifndef PLUGINDIR
# define PLUGINDIR		LIBDIR "/Mailer"
#endif
#ifndef SYSCONFDIR
# define SYSCONFDIR		PREFIX "/etc"
#endif


/* Mailer */
/* private */
/* types */
typedef enum _AccountAssistantPage
{
	AAP_INTRO = 0,
	AAP_SETTINGS,
	AAP_CONFIRM
} AccountAssistantPage;
#define AAP_LAST AAP_CONFIRM
#define AAP_COUNT (AAP_LAST + 1)

typedef enum _MailerPluginColumn
{
	MPC_NAME = 0,
	MPC_ENABLED,
	MPC_ICON,
	MPC_NAME_DISPLAY,
	MPC_PLUGIN,
	MPC_MAILERPLUGINDEFINITION,
	MPC_MAILERPLUGIN,
	MPC_WIDGET
} MailerPluginColumn;
#define MPC_LAST MPC_WIDGET
#define MPC_COUNT (MPC_LAST + 1)

struct _Mailer
{
	Account ** available; /* XXX consider using another data type */
	unsigned int available_cnt;

	Account ** account;
	unsigned int account_cnt;
	Account * account_cur;
	Folder * folder_cur;
	Message * message_cur;

	guint source;

	/* configuration */
	Config * config;

	/* SSL */
	SSL_CTX * ssl_ctx;

	/* widgets */
	/* folders */
	GtkWidget * fo_window;
#if GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * fo_infobar;
	GtkWidget * fo_infobar_label;
#endif
	GtkTreeStore * fo_store;
	GtkWidget * fo_view;
	/* headers */
	GtkWidget * he_window;
	GtkWidget * he_view;
	GtkTreeViewColumn * he_view_from;
	GtkTreeViewColumn * he_view_to;
	/* body */
	GtkWidget * bo_window;
	GtkWidget * hdr_vbox;
	GtkWidget * hdr_subject;
	GtkWidget * hdr_from;
	GtkWidget * hdr_to;
	GtkWidget * hdr_date;
	GtkTextBuffer * bo_buffer;
	GtkWidget * bo_view;
	GtkWidget * statusbar;
	gint statusbar_id;
	GtkWidget * st_online;
	/* plug-ins */
	GtkWidget * pl_window;
	GtkWidget * pl_view;
	GtkListStore * pl_store;
	GtkWidget * pl_combo;
	GtkWidget * pl_box;
	MailerPluginHelper pl_helper;
	/* about */
	GtkWidget * ab_window;
	/* preferences */
	GtkWidget * pr_window;
	GtkWidget * pr_accounts;
	GtkWidget * pr_messages_font;
	GtkListStore * pr_plugins_store;
	GtkWidget * pr_plugins;
};

/* FIXME use a more elegant model with an AccountMessage directly */
typedef void (*MailerForeachMessageCallback)(Mailer * mailer,
		GtkTreeModel * model, GtkTreeIter * iter);


/* constants */
static const char * _title[AAP_COUNT] =
{
	N_("New account"), N_("Account settings"), N_("Account confirmation")
};


/* variables */
#ifdef EMBEDDED
static const DesktopAccel _mailer_accel[] =
{
	{ G_CALLBACK(on_quit),		GDK_CONTROL_MASK,	GDK_KEY_Q },
	{ G_CALLBACK(on_view_source),	GDK_CONTROL_MASK,	GDK_KEY_U },
	{ NULL,				0,			0	  }
};
#endif


#ifndef EMBEDDED
static const DesktopMenu _menu_file[] =
{
	{ N_("_New mail"), G_CALLBACK(on_file_new_mail), "stock_mail-compose",
		GDK_CONTROL_MASK, GDK_KEY_N },
	{ N_("_Open..."), G_CALLBACK(on_file_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Send / Receive"), G_CALLBACK(on_file_send_receive),
		"stock_mail-send-receive", GDK_CONTROL_MASK, GDK_KEY_R },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Print"), NULL, GTK_STOCK_PRINT, GDK_CONTROL_MASK, GDK_KEY_P },
	{ N_("Print pre_view"), NULL, GTK_STOCK_PRINT_PREVIEW, GDK_CONTROL_MASK,
		0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Quit"), G_CALLBACK(on_file_quit), GTK_STOCK_QUIT,
		GDK_CONTROL_MASK, GDK_KEY_Q },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_edit[] =
{
	{ N_("_Cut"), G_CALLBACK(on_edit_cut), GTK_STOCK_CUT, GDK_CONTROL_MASK,
		GDK_KEY_X },
	{ N_("Cop_y"), G_CALLBACK(on_edit_copy), GTK_STOCK_COPY,
		GDK_CONTROL_MASK, GDK_KEY_C },
	{ N_("_Paste"), G_CALLBACK(on_edit_paste), GTK_STOCK_PASTE,
		GDK_CONTROL_MASK, GDK_KEY_V },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Select _All"), G_CALLBACK(on_edit_select_all),
#if GTK_CHECK_VERSION(2, 10, 0)
		GTK_STOCK_SELECT_ALL,
#else
		"edit-select-all",
#endif
		GDK_CONTROL_MASK, GDK_KEY_A },
	{ N_("_Unselect all"), G_CALLBACK(on_edit_unselect_all), NULL, 0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Preferences"), G_CALLBACK(on_edit_preferences),
		GTK_STOCK_PREFERENCES, 0, 0 },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_message[] =
{
	{ N_("_Reply"), G_CALLBACK(on_message_reply), "stock_mail-reply", 0,
		0 },
	{ N_("Reply to _All"), G_CALLBACK(on_message_reply_to_all),
		"stock_mail-reply-to-all", 0, 0 },
	{ N_("_Forward"), G_CALLBACK(on_message_forward), "stock_mail-forward",
		0, 0 },
	{ "", NULL, NULL, 0, 0 },
	{ N_("Save _As..."), G_CALLBACK(on_message_save_as), GTK_STOCK_SAVE_AS,
		GDK_CONTROL_MASK, GDK_KEY_S },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_Delete"), G_CALLBACK(on_message_delete), GTK_STOCK_DELETE, 0,
		GDK_KEY_Delete },
	{ "", NULL, NULL, 0, 0 },
	{ N_("_View source"), G_CALLBACK(on_message_view_source), NULL,
		GDK_CONTROL_MASK, GDK_KEY_U },
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenu _menu_help[] =
{
	{ N_("_Contents"), G_CALLBACK(on_help_contents), "help-contents", 0,
		GDK_KEY_F1 },
#if GTK_CHECK_VERSION(2, 6, 0)
	{ N_("_About"), G_CALLBACK(on_help_about), GTK_STOCK_ABOUT, 0, 0 },
#else
	{ N_("_About"), G_CALLBACK(on_help_about), NULL, 0, 0 },
#endif
	{ NULL, NULL, NULL, 0, 0 }
};

static const DesktopMenubar _mailer_menubar[] =
{
	{ N_("_File"), _menu_file },
	{ N_("_Edit"), _menu_edit },
	{ N_("_Message"), _menu_message },
	{ N_("_Help"), _menu_help },
	{ NULL, NULL }
};
#endif

static DesktopToolbar _mailer_fo_toolbar[] =
{
	{ N_("New message"), G_CALLBACK(on_new_mail), "stock_mail-compose", 0,
		0, NULL },
#ifdef EMBEDDED
	{ N_("Open message"), G_CALLBACK(on_open), GTK_STOCK_OPEN,
		GDK_CONTROL_MASK, GDK_KEY_O, NULL },
#endif
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Send / Receive"), G_CALLBACK(on_file_send_receive),
		"stock_mail-send-receive", 0, 0, NULL },
	{ N_("Stop"), NULL, GTK_STOCK_STOP, 0, GDK_KEY_Escape, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
#ifndef EMBEDDED
	{ N_("Reply"), G_CALLBACK(on_reply), "stock_mail-reply", 0, 0, NULL },
	{ N_("Reply to all"), G_CALLBACK(on_reply_to_all),
		"stock_mail-reply-to-all", 0, 0, NULL },
	{ N_("Forward"), G_CALLBACK(on_forward), "stock_mail-forward", 0, 0,
		NULL},
#endif
	{ N_("Delete"), G_CALLBACK(on_delete), GTK_STOCK_DELETE, 0, 0, NULL },
	{ N_("Print"), NULL, GTK_STOCK_PRINT, 0, 0, NULL },
#ifdef EMBEDDED
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Preferences"), G_CALLBACK(on_preferences), GTK_STOCK_PREFERENCES,
		0, 0, NULL },
#endif
	{ NULL, NULL, NULL, 0, 0, NULL }
};

#ifdef EMBEDDED
static DesktopToolbar _mailer_he_toolbar[] =
{
	{ N_("New mail"), G_CALLBACK(on_file_new_mail), "stock_mail-compose", 0,
		0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Reply"), G_CALLBACK(on_reply), "stock_mail-reply", 0, 0, NULL },
	{ N_("Reply to all"), G_CALLBACK(on_reply_to_all),
		"stock_mail-reply-to-all", 0, 0, NULL },
	{ N_("Forward"), G_CALLBACK(on_forward), "stock_mail-forward", 0, 0,
		NULL},
	{ N_("Delete"), G_CALLBACK(on_delete), GTK_STOCK_DELETE, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};

static DesktopToolbar _mailer_bo_toolbar[] =
{
	{ N_("New mail"), G_CALLBACK(on_file_new_mail), "stock_mail-compose", 0,
		0, NULL },
	{ "", NULL, NULL, 0, 0, NULL },
	{ N_("Reply"), G_CALLBACK(on_reply), "stock_mail-reply", 0, 0, NULL },
	{ N_("Reply to all"), G_CALLBACK(on_reply_to_all),
		"stock_mail-reply-to-all", 0, 0, NULL },
	{ N_("Forward"), G_CALLBACK(on_forward), "stock_mail-forward", 0, 0,
		NULL},
	{ N_("Delete"), G_CALLBACK(on_delete), GTK_STOCK_DELETE, 0, 0, NULL },
	{ N_("Print"), NULL, GTK_STOCK_PRINT, 0, 0, NULL },
	{ NULL, NULL, NULL, 0, 0, NULL }
};
#endif


/* prototypes */
/* accessors */
static gboolean _mailer_plugin_is_enabled(Mailer * mailer, char const * plugin);
static char const * _mailer_get_font(Mailer * mailer);
static char * _mailer_get_config_filename(void);
static GList * _mailer_get_selected_headers(Mailer * mailer);

/* useful */
static int _mailer_config_load_account(Mailer * mailer, char const * name);
static gboolean _mailer_confirm(Mailer * mailer, char const * message);
static void _mailer_foreach_message_selected(Mailer * mailer,
		MailerForeachMessageCallback callback);
static void _mailer_refresh_plugin(Mailer * mailer);
static void _mailer_refresh_title(Mailer * mailer);
static void _mailer_update_status(Mailer * mailer);
static void _mailer_update_view(Mailer * mailer);

/* callbacks */
static void _mailer_on_online_toggled(gpointer data);
static void _mailer_on_plugin_combo_changed(gpointer data);


/* public */
/* functions */
/* mailer_new */
static int _new_accounts(Mailer * mailer);
static GtkWidget * _new_folders_view(Mailer * mailer);
static void _on_folders_changed(GtkTreeSelection * selection, gpointer data);
static GtkWidget * _new_headers_view(Mailer * mailer);
static GtkWidget * _new_headers(Mailer * mailer);
static GtkTreeViewColumn * _headers_view_column_icon(GtkTreeView * view,
		char const * title, int id, int sortid);
static GtkTreeViewColumn * _headers_view_column_text(GtkTreeView * view,
		char const * title, int id, int sortid, int boldid);
static void _on_headers_changed(GtkTreeSelection * selection, gpointer data);
static gboolean _new_idle(gpointer data);
static void _idle_config_load(Mailer * mailer);
static void _idle_plugins_load(Mailer * mailer);

Mailer * mailer_new(void)
{
	Mailer * mailer;
	GtkAccelGroup * group;
	GtkWidget * vbox;
	GtkWidget * vbox2;
	GtkWidget * hbox;
#ifndef EMBEDDED
	GtkWidget * hpaned;
	GtkWidget * hpaned2;
	GtkWidget * vpaned;
#endif
	GtkCellRenderer * renderer;
	GtkWidget * widget;
	char buf[128];

	if((mailer = object_new(sizeof(*mailer))) == NULL)
	{
		error_print(PROGNAME_MAILER);
		return NULL;
	}
	/* accounts */
	_new_accounts(mailer);
	mailer->account = NULL;
	mailer->account_cnt = 0;
	mailer->account_cur = NULL;
	mailer->folder_cur = NULL;
	mailer->message_cur = NULL;
	/* plug-ins */
	mailer->pl_helper.mailer = mailer;
	mailer->pl_helper.error = mailer_error;
	/* ssl */
	SSL_load_error_strings();
	SSL_library_init();
	if((mailer->ssl_ctx = SSL_CTX_new(TLS_client_method())) == NULL
			|| SSL_CTX_set_cipher_list(mailer->ssl_ctx,
				SSL_DEFAULT_CIPHER_LIST) != 1
			|| SSL_CTX_load_verify_locations(mailer->ssl_ctx, NULL,
				SYSCONFDIR "/openssl/certs") != 1)
	{
		mailer_error(NULL, ERR_error_string(ERR_get_error(), buf), 1);
		if(mailer->ssl_ctx != NULL)
			SSL_CTX_free(mailer->ssl_ctx);
		mailer->ssl_ctx = NULL;
	}
#if 0 /* XXX nicer for the server (knows why we shutdown) but not for us */
	else
		SSL_CTX_set_verify(mailer->ssl_ctx, SSL_VERIFY_PEER, NULL);
#endif
	SSL_CTX_set_options(mailer->ssl_ctx, SSL_OP_NO_SSLv2);
	/* widgets */
	group = gtk_accel_group_new();
	mailer->fo_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(mailer->fo_window), group);
	g_object_unref(group);
#ifndef EMBEDDED
	gtk_window_set_default_size(GTK_WINDOW(mailer->fo_window), 800, 600);
	gtk_window_set_title(GTK_WINDOW(mailer->fo_window), PACKAGE);
#else
	gtk_window_set_default_size(GTK_WINDOW(mailer->fo_window), 200, 300);
	gtk_window_set_title(GTK_WINDOW(mailer->fo_window),
			_(PACKAGE " - Folders"));
#endif
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(mailer->fo_window), "mailer");
#endif
	g_signal_connect_swapped(G_OBJECT(mailer->fo_window), "delete-event",
			G_CALLBACK(on_closex), mailer);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	/* menubar */
#ifndef EMBEDDED
	widget = desktop_menubar_create(_mailer_menubar, mailer, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
#else
	desktop_accel_create(_mailer_accel, mailer, group);
#endif
	/* toolbar */
	widget = desktop_toolbar_create(_mailer_fo_toolbar, mailer, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* infobar */
	mailer->fo_infobar = gtk_info_bar_new_with_buttons(GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE, NULL);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(mailer->fo_infobar),
			GTK_MESSAGE_ERROR);
	g_signal_connect(mailer->fo_infobar, "close", G_CALLBACK(
				gtk_widget_hide), NULL);
	g_signal_connect(mailer->fo_infobar, "response", G_CALLBACK(
				gtk_widget_hide), NULL);
	widget = gtk_info_bar_get_content_area(GTK_INFO_BAR(
				mailer->fo_infobar));
	mailer->fo_infobar_label = gtk_label_new(NULL);
	gtk_widget_show(mailer->fo_infobar_label);
	gtk_box_pack_start(GTK_BOX(widget), mailer->fo_infobar_label, TRUE,
			TRUE, 0);
	gtk_widget_set_no_show_all(mailer->fo_infobar, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), mailer->fo_infobar, FALSE, TRUE, 0);
#endif
	/* folders */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	mailer->fo_view = _new_folders_view(mailer);
	gtk_container_add(GTK_CONTAINER(widget), mailer->fo_view);
#ifndef EMBEDDED
	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_set_position(GTK_PANED(hpaned), 160);
	gtk_paned_add1(GTK_PANED(hpaned), widget);
	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_paned_set_position(GTK_PANED(vpaned), 160);
	gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 0);
#else
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
#endif
	/* statusbar */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	mailer->st_online = gtk_toggle_button_new();
	g_signal_connect_swapped(mailer->st_online, "toggled", G_CALLBACK(
				_mailer_on_online_toggled), mailer);
	/* FIXME set icon and callback */
	gtk_box_pack_start(GTK_BOX(hbox), mailer->st_online, FALSE, TRUE, 0);
	mailer->statusbar = gtk_statusbar_new();
	mailer->statusbar_id = 0;
	gtk_box_pack_start(GTK_BOX(hbox), mailer->statusbar, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(mailer->fo_window), vbox);
	gtk_widget_show_all(vbox);
	/* messages list */
#ifndef EMBEDDED
	mailer->he_window = mailer->fo_window;
#else
	group = gtk_accel_group_new();
	mailer->he_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(mailer->he_window), group);
	g_object_unref(group);
	gtk_window_set_default_size(GTK_WINDOW(mailer->he_window), 200, 300);
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(mailer->he_window), "mailer");
# endif
	gtk_window_set_title(GTK_WINDOW(mailer->he_window), _("Message list"));
	g_signal_connect_swapped(G_OBJECT(mailer->he_window), "delete-event",
			G_CALLBACK(on_headers_closex), mailer);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(mailer->he_window), vbox);
	widget = desktop_toolbar_create(_mailer_he_toolbar, mailer, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#endif
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	mailer->he_view = _new_headers_view(mailer);
	gtk_container_add(GTK_CONTAINER(widget), mailer->he_view);
#ifndef EMBEDDED
	gtk_paned_add1(GTK_PANED(vpaned), widget);
#else
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
#endif
	gtk_widget_show_all(vbox);
	/* messages body */
#ifndef EMBEDDED
	mailer->bo_window = mailer->fo_window;
#else
	group = gtk_accel_group_new();
	mailer->bo_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(mailer->bo_window), group);
	g_object_unref(group);
	gtk_window_set_default_size(GTK_WINDOW(mailer->bo_window), 200, 300);
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(mailer->bo_window), "mailer");
# endif
	gtk_window_set_title(GTK_WINDOW(mailer->bo_window), _("Message"));
	g_signal_connect_swapped(G_OBJECT(mailer->bo_window), "delete-event",
			G_CALLBACK(on_body_closex), mailer);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(mailer->bo_window), vbox);
	widget = desktop_toolbar_create(_mailer_bo_toolbar, mailer, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
#endif
	vbox2 = _new_headers(mailer);
	mailer->bo_buffer = gtk_text_buffer_new(NULL);
	mailer->bo_view = gtk_text_view_new_with_buffer(mailer->bo_buffer);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(mailer->bo_view), FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(mailer->bo_view), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(mailer->bo_view),
			GTK_WRAP_WORD_CHAR);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(widget), mailer->bo_view);
	gtk_box_pack_start(GTK_BOX(vbox2), widget, TRUE, TRUE, 0);
#ifndef EMBEDDED
	gtk_paned_add2(GTK_PANED(vpaned), vbox2);
#else
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 0);
#endif
	/* plug-ins */
#ifndef EMBEDDED
	mailer->pl_window = mailer->fo_window;
	mailer->pl_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	group = gtk_accel_group_new();
	mailer->pl_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(GTK_WINDOW(mailer->pl_window), group);
	g_object_unref(group);
	gtk_window_set_default_size(GTK_WINDOW(mailer->pl_window), 200, 300);
# if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(mailer->pl_window), "mailer");
# endif
	gtk_window_set_title(GTK_WINDOW(mailer->pl_window), _("Plug-ins"));
	g_signal_connect_swapped(G_OBJECT(mailer->pl_window), "delete-event",
			G_CALLBACK(on_plugins_closex), mailer);
	mailer->pl_view = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(mailer->pl_window), mailer->pl_view);
#endif
	mailer->pl_store = gtk_list_store_new(MPC_COUNT, G_TYPE_STRING,
			G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING,
			G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER,
			G_TYPE_POINTER);
	mailer->pl_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(
				mailer->pl_store));
	g_signal_connect_swapped(G_OBJECT(mailer->pl_combo), "changed",
			G_CALLBACK(_mailer_on_plugin_combo_changed), mailer);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mailer->pl_combo), renderer,
			FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mailer->pl_combo),
			renderer, "pixbuf", MPC_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mailer->pl_combo),
			renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mailer->pl_combo),
			renderer, "text", MPC_NAME_DISPLAY, NULL);
	gtk_box_pack_start(GTK_BOX(mailer->pl_view), mailer->pl_combo, FALSE,
			TRUE, 0);
	mailer->pl_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start(GTK_BOX(mailer->pl_view), mailer->pl_box, TRUE, TRUE,
			0);
	gtk_widget_set_no_show_all(mailer->pl_view, TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(mailer->pl_view), 4);
#ifndef EMBEDDED
	hpaned2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_paned_pack1(GTK_PANED(hpaned2), vpaned, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(hpaned2), mailer->pl_view, FALSE, TRUE);
	gtk_paned_add2(GTK_PANED(hpaned), hpaned2);
#endif
	gtk_widget_show_all(vbox);
	/* widgets */
	mailer->ab_window = NULL;
	mailer->pr_window = NULL;
	/* show window */
	gtk_widget_hide(mailer->hdr_vbox);
	_mailer_update_status(mailer);
	gtk_widget_show(mailer->pl_window);
	gtk_widget_show(mailer->fo_window);
	/* load configuration */
	mailer->source = g_idle_add(_new_idle, mailer);
	return mailer;
}

static int _new_accounts(Mailer * mailer)
{
	int ret = 0;
	char * dirname;
	DIR * dir;
	struct dirent * de;
	size_t len;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	Account ** p;

	mailer->available = NULL;
	mailer->available_cnt = 0;
	if((dirname = string_new_append(PLUGINDIR, "/account", NULL)) == NULL)
		return -1;
	if((dir = opendir(dirname)) == NULL)
	{
		error_set_code(1, "%s: %s", dirname, strerror(errno));
		string_delete(dirname);
		return error_print(PROGNAME_MAILER);
	}
	for(de = readdir(dir); de != NULL; de = readdir(dir))
	{
		if((len = strlen(de->d_name)) < sizeof(ext)
				|| strcmp(ext, &de->d_name[
					len - sizeof(ext) + 1]) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
		if((p = realloc(mailer->available, (mailer->available_cnt + 1)
						* sizeof(*p))) == NULL)
		{
			error_set_print(PROGNAME_MAILER, 1, "%s",
					strerror(errno));
			continue;
		}
		mailer->available = p;
		if((p[mailer->available_cnt] = account_new(NULL, de->d_name,
						NULL, NULL)) == NULL)
		{
			error_print(PROGNAME_MAILER);
			continue;
		}
		mailer->available_cnt++;
	}
	if(closedir(dir) != 0)
		ret = error_set_print(PROGNAME_MAILER, 1, "%s: %s", dirname,
				strerror(errno));
	string_delete(dirname);
	return ret;
}

static GtkWidget * _new_folders_view(Mailer * mailer)
{
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeSelection * treesel;

	mailer->fo_store = gtk_tree_store_new(MFC_COUNT, G_TYPE_POINTER,
			G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER,
			GDK_TYPE_PIXBUF, G_TYPE_STRING);
	widget = gtk_tree_view_new_with_model(GTK_TREE_MODEL(mailer->fo_store));
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(widget), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(widget), -1,
			NULL, renderer, "pixbuf", MFC_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(widget), -1,
			_("Folders"), renderer, "text", MFC_NAME, NULL);
	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	g_signal_connect(G_OBJECT(treesel), "changed", G_CALLBACK(
				_on_folders_changed), mailer);
	return widget;
}

static void _on_folders_changed(GtkTreeSelection * selection, gpointer data)
{
	Mailer * mailer = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreePath * path;
	gboolean sent;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	mailer->message_cur = NULL;
	if(gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
	{
		mailer->account_cur = NULL;
		mailer->folder_cur = NULL;
		gtk_tree_view_set_model(GTK_TREE_VIEW(mailer->he_view), NULL);
		_mailer_update_status(mailer);
		return;
	}
	/* get current folder */
	gtk_tree_model_get(model, &iter, MFC_FOLDER, &mailer->folder_cur, -1);
	/* get current account */
	path = gtk_tree_model_get_path(model, &iter);
	while(gtk_tree_path_get_depth(path) > 1 && gtk_tree_path_up(path));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, MFC_ACCOUNT, &mailer->account_cur, -1);
	gtk_tree_path_free(path);
	/* display relevant columns */
	sent = (mailer->folder_cur != NULL && folder_get_type(
				mailer->folder_cur) == FT_SENT) ? TRUE : FALSE;
	gtk_tree_view_column_set_visible(mailer->he_view_from, !sent);
	gtk_tree_view_column_set_visible(mailer->he_view_to, sent);
	_mailer_update_view(mailer);
#ifdef EMBEDDED
	if(model != NULL)
		gtk_window_present(GTK_WINDOW(mailer->he_window));
#endif
}

static GtkWidget * _new_headers_view(Mailer * mailer)
{
	GtkWidget * widget;
	GtkTreeView * treeview;
	GtkTreeSelection * treesel;

	widget = gtk_tree_view_new();
	treeview = GTK_TREE_VIEW(widget);
	gtk_tree_view_set_reorderable(treeview, FALSE);
	gtk_tree_view_set_rules_hint(treeview, TRUE);
	_headers_view_column_icon(treeview, "", MHC_ICON, MHC_READ);
	_headers_view_column_text(treeview, _("Subject"), MHC_SUBJECT,
			MHC_SUBJECT, MHC_WEIGHT);
	mailer->he_view_from = _headers_view_column_text(treeview, _("From"),
			MHC_FROM, MHC_FROM, MHC_WEIGHT);
	mailer->he_view_to = _headers_view_column_text(treeview, _("To"),
			MHC_TO, MHC_TO, MHC_WEIGHT);
	_headers_view_column_text(treeview, _("Date"), MHC_DATE_DISPLAY,
			MHC_DATE, MHC_WEIGHT);
	treesel = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(treesel, GTK_SELECTION_MULTIPLE);
	g_signal_connect(G_OBJECT(treesel), "changed", G_CALLBACK(
				_on_headers_changed), mailer);
	return widget;
}

static void _on_headers_changed(GtkTreeSelection * selection, gpointer data)
{
	Mailer * mailer = data;
	GtkTreeModel * model;
	GList * sel;
	GtkTreeIter iter;
	gchar * p;
	gchar * q;
	gchar * r;
	Message * message;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	sel = gtk_tree_selection_get_selected_rows(selection, &model);
	if(sel == NULL || sel->next != NULL) /* empty or multiple */
	{
		mailer->message_cur = NULL;
		gtk_widget_hide(mailer->hdr_vbox);
		gtk_text_view_set_buffer(GTK_TEXT_VIEW(mailer->bo_view),
				mailer->bo_buffer);
		g_list_foreach(sel, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(sel);
		return;
	}
	gtk_tree_model_get_iter(model, &iter, sel->data);
	gtk_tree_model_get(model, &iter, MHC_MESSAGE, &message, -1);
	mailer->message_cur = message;
	gtk_tree_model_get(model, &iter, MHC_SUBJECT, &p, -1);
	gtk_label_set_text(GTK_LABEL(mailer->hdr_subject), p);
	g_free(p);
	gtk_tree_model_get(model, &iter, MHC_FROM, &p, MHC_FROM_EMAIL, &q, -1);
	if(q == NULL || strlen(q) == 0 || strcmp(p, q) == 0)
		r = g_strdup(p);
	else
		r = g_strdup_printf("%s <%s>", p, q);
	gtk_label_set_text(GTK_LABEL(mailer->hdr_from), r);
	g_free(p);
	g_free(q);
	g_free(r);
	gtk_tree_model_get(model, &iter, MHC_TO, &p, MHC_TO_EMAIL, &q, -1);
	if(q == NULL || strlen(q) == 0 || strcmp(p, q) == 0)
		r = g_strdup(p);
	else
		r = g_strdup_printf("%s <%s>", p, q);
	gtk_label_set_text(GTK_LABEL(mailer->hdr_to), r);
	g_free(p);
	g_free(q);
	g_free(r);
	gtk_tree_model_get(model, &iter, MHC_DATE_DISPLAY, &p, -1);
	gtk_label_set_text(GTK_LABEL(mailer->hdr_date), p);
	g_free(p);
	message_set_read(message, TRUE);
	gtk_widget_show(mailer->hdr_vbox);
	_mailer_update_view(mailer);
	g_list_foreach(sel, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(sel);
#ifdef EMBEDDED
	gtk_window_present(GTK_WINDOW(mailer->bo_window));
#endif
}

static GtkTreeViewColumn * _headers_view_column_icon(GtkTreeView * view,
		char const * title, int id, int sortid)
{
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(title, renderer,
			"pixbuf", id, NULL);
	gtk_tree_view_column_set_sort_column_id(column, sortid);
	gtk_tree_view_append_column(view, column);
	return column;
}

static GtkTreeViewColumn * _headers_view_column_text(GtkTreeView * view,
		char const * title, int id, int sortid, int weightid)
{
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END,
			NULL);
	column = gtk_tree_view_column_new_with_attributes(title, renderer,
			"text", id, (weightid >= 0) ? "weight" : NULL, weightid,
			NULL);
#if GTK_CHECK_VERSION(2, 4, 0)
	gtk_tree_view_column_set_expand(column, TRUE);
#endif
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, sortid);
	gtk_tree_view_append_column(view, column);
	return column;
}

static GtkWidget * _new_headers(Mailer * mailer)
{
	struct
	{
		char * hdr;
		GtkWidget ** widget;
	} widgets[] =
	{
		{ N_(" Subject: "),	NULL	},
		{ N_(" From: "),	NULL	},
		{ N_(" To: "),		NULL	},
		{ N_(" Date: "),	NULL	},
		{ NULL,			NULL	}
	};
	int i;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkSizeGroup * group;
	PangoFontDescription * bold;

	widgets[0].widget = &mailer->hdr_subject;
	widgets[1].widget = &mailer->hdr_from;
	widgets[2].widget = &mailer->hdr_to;
	widgets[3].widget = &mailer->hdr_date;
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	mailer->hdr_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
	for(i = 0; widgets[i].hdr != NULL; i++)
	{
		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		widget = gtk_label_new(_(widgets[i].hdr));
		gtk_widget_override_font(widget, bold);
#if GTK_CHECK_VERSION(3, 14, 0)
		g_object_set(widget, "halign", GTK_ALIGN_END,
				"valign", GTK_ALIGN_START, NULL);
#else
		gtk_misc_set_alignment(GTK_MISC(widget), 1.0, 0.0);
#endif
		gtk_size_group_add_widget(GTK_SIZE_GROUP(group), widget);
		gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
		widget = gtk_label_new("");
		*(widgets[i].widget) = widget;
		gtk_label_set_ellipsize(GTK_LABEL(widget),
				PANGO_ELLIPSIZE_MIDDLE);
#if GTK_CHECK_VERSION(3, 14, 0)
		g_object_set(widget, "halign", GTK_ALIGN_START,
				"valign", GTK_ALIGN_START, NULL);
#else
		gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.0);
#endif
		gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(mailer->hdr_vbox), hbox, FALSE,
				FALSE, 0);
	}
	gtk_box_pack_start(GTK_BOX(vbox), mailer->hdr_vbox, FALSE, FALSE, 0);
	pango_font_description_free(bold);
	return vbox;
}

static gboolean _new_idle(gpointer data)
{
	Mailer * mailer = data;

	_idle_config_load(mailer);
	_idle_plugins_load(mailer);
	return FALSE;
}

static void _idle_config_load(Mailer * mailer)
{
	char * filename;
	char const * value;
	PangoFontDescription * font;
	char * p;
	char * q;

	mailer->source = 0;
	if((mailer->config = config_new()) == NULL)
		return;
	if((filename = _mailer_get_config_filename()) == NULL)
		return;
	if(config_load(mailer->config, filename) != 0)
		mailer_error(NULL, error_get(NULL), 1);
	free(filename);
	value = _mailer_get_font(mailer);
	font = pango_font_description_from_string(value);
	gtk_widget_override_font(mailer->bo_view, font);
	pango_font_description_free(font);
	/* check if we are online */
	if((p = config_get(mailer->config, NULL, "online")) == NULL
			|| strtol(p, NULL, 10) != 0)
		mailer_set_online(mailer, TRUE);
	else
		mailer_set_online(mailer, FALSE);
	/* load accounts */
	if((value = config_get(mailer->config, NULL, "accounts")) == NULL
			|| value[0] == '\0')
		return;
	if((p = strdup(value)) == NULL)
		return;
	value = p;
	for(q = p; *q != '\0'; q++)
	{
		if(*q != ',')
			continue;
		*q = '\0';
		_mailer_config_load_account(mailer, value);
		value = q + 1;
	}
	if(value[0] != '\0')
		_mailer_config_load_account(mailer, value);
	free(p);
}

static void _idle_plugins_load(Mailer * mailer)
{
	char const * plugins;
	char * p;
	char * q;
	size_t i;

	if((plugins = config_get(mailer->config, NULL, "plugins")) == NULL
			|| strlen(plugins) == 0)
		return;
	if((p = strdup(plugins)) == NULL)
		return; /* XXX report error */
	for(q = p, i = 0;;)
	{
		if(q[i] == '\0')
		{
			mailer_load(mailer, q);
			break;
		}
		if(q[i++] != ',')
			continue;
		q[i - 1] = '\0';
		mailer_load(mailer, q);
		q += i;
		i = 0;
	}
	free(p);
}


/* mailer_delete */
static void _delete_plugins(Mailer * mailer);

void mailer_delete(Mailer * mailer)
{
	unsigned int i;

	_delete_plugins(mailer);
	if(mailer->ssl_ctx != NULL)
		SSL_CTX_free(mailer->ssl_ctx);
	if(mailer->source != 0)
		g_source_remove(mailer->source);
	for(i = 0; i < mailer->available_cnt; i++)
		account_delete(mailer->available[i]);
	free(mailer->available);
	for(i = 0; i < mailer->account_cnt; i++)
		account_delete(mailer->account[i]);
	free(mailer->account);
	g_object_unref(mailer->pl_store);
	object_delete(mailer);
}

static void _delete_plugins(Mailer * mailer)
{
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	MailerPluginDefinition * mpd;
	MailerPlugin * mp;
	Plugin * plugin;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, MPC_PLUGIN, &plugin,
				MPC_MAILERPLUGINDEFINITION, &mpd,
				MPC_MAILERPLUGIN, &mp, -1);
		if(mpd->destroy != NULL)
			mpd->destroy(mp);
		plugin_delete(plugin);
	}
}


/* accessors */
/* mailer_get_config */
char const * mailer_get_config(Mailer * mailer, char const * variable)
{
	return config_get(mailer->config, NULL, variable);
}


/* mailer_get_ssl_context */
SSL_CTX * mailer_get_ssl_context(Mailer * mailer)
{
	return mailer->ssl_ctx;
}


/* mailer_is_online */
gboolean mailer_is_online(Mailer * mailer)
{
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				mailer->st_online));
}


/* mailer_set_online */
void mailer_set_online(Mailer * mailer, gboolean online)
{
	GtkWidget * image;
	size_t i;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mailer->st_online),
			online);
	image = gtk_image_new_from_stock(online ? GTK_STOCK_CONNECT
			: GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(mailer->st_online), image);
	for(i = 0; i < mailer->account_cnt; i++)
		if(online)
			account_start(mailer->account[i]);
		else
			account_stop(mailer->account[i]);
}


/* mailer_set_status */
void mailer_set_status(Mailer * mailer, char const * status)
{
	GtkStatusbar * sb;

	if(status == NULL)
	{
		_mailer_update_status(mailer);
		return;
	}
	sb = GTK_STATUSBAR(mailer->statusbar);
	if(mailer->statusbar_id != 0)
		gtk_statusbar_remove(sb, gtk_statusbar_get_context_id(sb, ""),
				mailer->statusbar_id);
	mailer->statusbar_id = gtk_statusbar_push(sb,
			gtk_statusbar_get_context_id(sb, ""), status);
}


/* useful */
/* mailer_error */
int mailer_error(Mailer * mailer, char const * message, int ret)
{
#if !GTK_CHECK_VERSION(2, 18, 0)
	GtkWidget * dialog;
#endif

	if(mailer == NULL)
		return error_set_print(PROGNAME_MAILER, ret, "%s", message);
#if GTK_CHECK_VERSION(2, 18, 0)
	/* info bar */
	gtk_label_set_text(GTK_LABEL(mailer->fo_infobar_label), message);
	gtk_widget_show(mailer->fo_infobar);
#else
	/* dialog window */
	dialog = gtk_message_dialog_new(GTK_WINDOW(mailer->fo_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
# if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
# endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(
				mailer->fo_window));
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(
				gtk_widget_destroy), NULL);
	gtk_widget_show(dialog);
#endif
	return ret;
}


/* mailer_refresh_all */
void mailer_refresh_all(Mailer * mailer)
{
	size_t i;

	for(i = 0; i < mailer->account_cnt; i++)
		account_refresh(mailer->account[i]);
}


/* mailer_account_add */
int mailer_account_add(Mailer * mailer, Account * account)
{
	Account ** p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p)\n", __func__, (void *)account);
#endif
	if((p = realloc(mailer->account, sizeof(*p) * (mailer->account_cnt
						+ 1))) == NULL)
		return -mailer_error(mailer, "realloc", 1);
	mailer->account = p;
	mailer->account[mailer->account_cnt] = account;
	mailer->account_cnt++;
	account_store(account, mailer->fo_store);
	/* XXX check (and report) errors */
	if(mailer_is_online(mailer))
		account_start(account);
	return 0;
}


#if 0 /* FIXME deprecate? */
/* mailer_account_disable */
int mailer_account_disable(Mailer * mailer, Account * account)
{
	unsigned int i;

	for(i = 0; i < mailer->account_cnt; i++)
		if(mailer->account[i] == account)
			break;
	if(i == mailer->account_cnt)
		return 1;
	account_set_enabled(account, 0);
	return 0;
}


/* mailer_account_enable */
int mailer_account_enable(Mailer * mailer, Account * account)
{
	unsigned int i;

	for(i = 0; i < mailer->account_cnt; i++)
		if(mailer->account[i] == account)
			break;
	if(i == mailer->account_cnt)
		return 1;
	account_set_enabled(account, 1);
	return 0;
}
#endif


/* mailer_compose */
void mailer_compose(Mailer * mailer)
{
	Compose * compose;
	Account * account;
	char const * title;
	char const * name;
	char const * email;
	char const * organization;
	gchar * p;

	if((compose = compose_new(mailer->config)) == NULL)
		return; /* XXX report error */
	if((account = mailer->account_cur) == NULL)
	{
		if(mailer->account_cnt == 0)
			return;
		account = mailer->account[0];
	}
	title = account_get_title(account);
	if((name = config_get(mailer->config, title, "identity_name")) != NULL
			&& name[0] == '\0')
		name = NULL;
	if((email = config_get(mailer->config, title, "identity_email")) != NULL
			&& email[0] == '\0')
		email = NULL;
	if((p = g_strdup_printf("%s%s%s%s", (name != NULL) ? name : "",
					(name != NULL && email != NULL) ? " <"
					: "", (email != NULL) ? email : "",
					(name != NULL && email != NULL) ? ">"
					: "")) != NULL)
	{
		compose_set_from(compose, p);
		g_free(p);
	}
	organization = config_get(mailer->config, title,
			"identity_organization");
	if(organization != NULL && organization[0] != '\0')
		compose_set_header(compose, "Organization:", organization,
				TRUE);
}


/* mailer_delete_selected */
static void _mailer_delete_selected_foreach(GtkTreeRowReference * reference,
		Mailer * mailer);

void mailer_delete_selected(Mailer * mailer)
{
	/* FIXME figure which area is focused first (deleting folders) */
	GtkTreeModel * model;
	GList * selected;
	GList * s;
	GtkTreePath * path;
	GtkTreeRowReference * reference;

	if((model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->he_view)))
			== NULL)
		return;
	if((selected = _mailer_get_selected_headers(mailer)) == NULL)
		return;
	/* FIXME move messages to trash first */
	if(_mailer_confirm(mailer, _("The messages selected will be deleted.\n"
					"Continue?")) != TRUE)
	{
		g_list_free(selected);
		return;
	}
	for(s = g_list_first(selected); s != NULL; s = g_list_next(s))
	{
		if((path = s->data) == NULL)
			continue;
		reference = gtk_tree_row_reference_new(model, path);
		s->data = reference;
		gtk_tree_path_free(path);
	}
	g_list_foreach(selected, (GFunc)_mailer_delete_selected_foreach,
			mailer);
	g_list_free(selected);
}

static void _mailer_delete_selected_foreach(GtkTreeRowReference * reference,
		Mailer * mailer)
{
	GtkTreeModel * model;
	GtkTreePath * path;
	GtkTreeIter iter;
	Message * message;

	if((model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->he_view)))
			== NULL)
		return;
	if(reference == NULL)
		return;
	if((path = gtk_tree_row_reference_get_path(reference)) == NULL)
		return;
	if(gtk_tree_model_get_iter(model, &iter, path) == TRUE)
		gtk_tree_model_get(model, &iter, MHC_MESSAGE, &message, -1);
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	gtk_tree_path_free(path);
}


/* mailer_copy */
void mailer_copy(Mailer * mailer)
{
	GtkTextBuffer * buffer;
	GtkClipboard * clipboard;

	if(gtk_window_get_focus(GTK_WINDOW(mailer->fo_window))
			== mailer->bo_view)
	{
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(
					mailer->bo_view));
		clipboard = gtk_widget_get_clipboard(mailer->bo_view,
				GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_copy_clipboard(buffer, clipboard);
	}
	/* FIXME implement the other widgets */
}


/* mailer_cut */
void mailer_cut(Mailer * mailer)
{
	GtkTextBuffer * buffer;
	GtkClipboard * clipboard;

	if(gtk_window_get_focus(GTK_WINDOW(mailer->fo_window))
			== mailer->bo_view) /* XXX copies, really */
	{
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(
					mailer->bo_view));
		clipboard = gtk_widget_get_clipboard(mailer->bo_view,
				GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_cut_clipboard(buffer, clipboard, FALSE);
	}
	/* FIXME implement the other widgets */
}


/* mailer_load */
int mailer_load(Mailer * mailer, char const * plugin)
{
	Plugin * p;
	MailerPluginDefinition * mpd;
	MailerPlugin * mp;
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkIconTheme * theme;
	GdkPixbuf * icon = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, plugin);
#endif
	if(_mailer_plugin_is_enabled(mailer, plugin))
		return 0;
	if((p = plugin_new(LIBDIR, PACKAGE, "plugins", plugin)) == NULL)
		return -mailer_error(NULL, error_get(NULL), 1);
	if((mpd = plugin_lookup(p, "plugin")) == NULL)
	{
		plugin_delete(p);
		return -mailer_error(NULL, error_get(NULL), 1);
	}
	if(mpd->init == NULL || mpd->destroy == NULL
			|| (mp = mpd->init(&mailer->pl_helper)) == NULL)
	{
		plugin_delete(p);
		return -mailer_error(NULL, error_get(NULL), 1);
	}
	theme = gtk_icon_theme_get_default();
	if(mpd->icon != NULL)
		icon = gtk_icon_theme_load_icon(theme, mpd->icon, 24, 0, NULL);
	if(icon == NULL)
		icon = gtk_icon_theme_load_icon(theme, "gnome-settings", 24, 0,
				NULL);
	widget = NULL;
	if(mpd->get_widget != NULL && (widget = mpd->get_widget(mp)) != NULL)
		gtk_widget_hide(widget);
	/* FIXME hide from the list of active plug-ins if there is no widget */
	gtk_list_store_append(mailer->pl_store, &iter);
	gtk_list_store_set(mailer->pl_store, &iter, MPC_NAME, plugin,
			MPC_ICON, icon, MPC_NAME_DISPLAY, mpd->name,
			MPC_PLUGIN, p, MPC_MAILERPLUGINDEFINITION, mpd,
			MPC_MAILERPLUGIN, mp, MPC_WIDGET, widget, -1);
	if(widget == NULL)
		return 0;
	gtk_box_pack_start(GTK_BOX(mailer->pl_box), widget, TRUE, TRUE, 0);
	if(gtk_widget_get_no_show_all(mailer->pl_view) == TRUE)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(mailer->pl_combo), 0);
		gtk_widget_set_no_show_all(mailer->pl_view, FALSE);
		gtk_widget_show_all(mailer->pl_view);
	}
	return 0;
}


/* mailer_open_selected_source */
static void _open_selected_source(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter);

void mailer_open_selected_source(Mailer * mailer)
{
	_mailer_foreach_message_selected(mailer, _open_selected_source);
}

static void _open_selected_source(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter)
{
	GtkWidget * window;
	GtkWidget * scrolled;
	PangoFontDescription * font;
	char const * p;
	GtkWidget * widget;
	GtkTextBuffer * tbuf;

	gtk_tree_model_get(model, iter, MHC_MESSAGE, &mailer->message_cur, -1);
	if(mailer->message_cur == NULL)
		return;
	if((tbuf = account_select_source(mailer->account_cur,
					mailer->folder_cur,
					mailer->message_cur)) == NULL)
		return;
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	/* XXX choose a better title */
	gtk_window_set_title(GTK_WINDOW(window), _("Mailer - View source"));
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	widget = gtk_text_view_new_with_buffer(tbuf);
	if((p = config_get(mailer->config, NULL, "messages_font")) != NULL)
	{
		font = pango_font_description_from_string(p);
		gtk_widget_override_font(widget, font);
		pango_font_description_free(font);
	}
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widget), FALSE);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(widget), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolled), widget);
	gtk_container_add(GTK_CONTAINER(window), scrolled);
	gtk_widget_show_all(window);
	/* FIXME count the window */
}


/* mailer_paste */
void mailer_paste(Mailer * mailer)
{
	GtkTextBuffer * buffer;
	GtkClipboard * clipboard;

	if(gtk_window_get_focus(GTK_WINDOW(mailer->fo_window))
			== mailer->bo_view) /* XXX does nothing, really */
	{
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(
					mailer->bo_view));
		clipboard = gtk_widget_get_clipboard(mailer->bo_view,
				GDK_SELECTION_CLIPBOARD);
		gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, FALSE);
	}
	/* FIXME implement the other widgets */
}


/* mailer_reply_selected */
static void _reply_selected(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter);

void mailer_reply_selected(Mailer * mailer)
{
	_mailer_foreach_message_selected(mailer, _reply_selected);
}

static void _reply_selected(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter)
{
	Compose * compose;
	char * date;
	char * from;
	char * subject;
	char * to;
	char * p;
	char const * q;
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;

	if((compose = compose_new(mailer->config)) == NULL)
		return; /* XXX error message? */
	gtk_tree_model_get(model, iter, MHC_DATE_DISPLAY, &date,
			MHC_FROM_EMAIL, &from, MHC_SUBJECT, &subject,
			MHC_TO_EMAIL, &to, -1);
	/* from */
	if(from != NULL)
		compose_set_header(compose, "To:", from, TRUE);
	/* to */
	if(to != NULL)
		compose_set_from(compose, to);
	/* FIXME also set the In-Reply-To field */
	/* subject */
	q = N_("Re: ");
	if(subject != NULL && strncasecmp(subject, q, strlen(q)) != 0
			&& strncasecmp(subject, _(q), strlen(_(q))) != 0
			&& (p = malloc(strlen(q) + strlen(subject) + 1))
			!= NULL)
	{
		sprintf(p, "%s%s", q, subject);
		free(subject);
		subject = p;
	}
	compose_set_subject(compose, subject);
	/* body */
	compose_set_text(compose, "\nOn ");
	compose_append_text(compose, date);
	compose_append_text(compose, ", ");
	compose_append_text(compose, from);
	compose_append_text(compose, " wrote:\n");
	tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mailer->bo_view));
	for(gtk_text_buffer_get_start_iter(tbuf, &start), end = start;
			gtk_text_iter_is_end(&start) != TRUE; start = end)
	{
		gtk_text_iter_forward_line(&end);
		p = gtk_text_iter_get_text(&start, &end);
		/* stop if we recognize an unquoted signature */
		if(strcmp(p, "-- \n") == 0)
		{
			g_free(p);
			break;
		}
		compose_append_text(compose, (p[0] == '>') ? ">" : "> ");
		compose_append_text(compose, p);
		g_free(p);
	}
	compose_append_signature(compose);
	compose_set_modified(compose, FALSE);
	compose_scroll_to_offset(compose, 0);
	free(date);
	free(from);
	free(subject);
}


/* mailer_reply_selected_to_all */
static void _reply_selected_to_all(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter);
void mailer_reply_selected_to_all(Mailer * mailer)
{
	_mailer_foreach_message_selected(mailer, _reply_selected_to_all);
}

static void _reply_selected_to_all(Mailer * mailer, GtkTreeModel * model,
		GtkTreeIter * iter)
{
	/* FIXME really implement */
	_reply_selected(mailer, model, iter);
}


/* mailer_save_selected */
gboolean mailer_save_selected(Mailer * mailer, char const * filename)
{
	if(mailer->message_cur == NULL)
		return TRUE;
	if(filename == NULL)
		return mailer_save_selected_dialog(mailer);
	return (message_save(mailer->message_cur, filename) == 0)
		? TRUE : FALSE;
}


/* mailer_save_selected_dialog */
gboolean mailer_save_selected_dialog(Mailer * mailer)
{
	gboolean ret;
	GtkWidget * dialog;
	char * filename = NULL;

	if(mailer->message_cur == NULL)
		return TRUE;
	dialog = gtk_file_chooser_dialog_new(_("Save as..."),
			GTK_WINDOW(mailer->he_window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = mailer_save_selected(mailer, filename);
	g_free(filename);
	return ret;
}


/* mailer_select_all */
void mailer_select_all(Mailer * mailer)
{
	GtkTreeSelection * treesel;
#if GTK_CHECK_VERSION(2, 4, 0)
	GtkTextBuffer * tbuf;
	GtkTextIter start;
	GtkTextIter end;
#endif

#if GTK_CHECK_VERSION(2, 4, 0)
	if(gtk_window_get_focus(GTK_WINDOW(mailer->fo_window))
			== mailer->bo_view)
	{
		tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mailer->bo_view));
		gtk_text_buffer_get_start_iter(tbuf, &start);
		gtk_text_buffer_get_end_iter(tbuf, &end);
		gtk_text_buffer_select_range(tbuf, &start, &end);
		return;
	}
#endif
	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mailer->he_view));
	gtk_tree_selection_select_all(treesel);
}


/* messages */
gboolean mailer_message_open(Mailer * mailer, char const * filename)
{
	Message * message;
	Compose * compose;

	if(filename == NULL)
		return mailer_message_open_dialog(mailer);
	if((message = message_new_open(mailer, filename)) == NULL)
	{
		mailer_error(mailer, error_get(NULL), 1);
		return FALSE;
	}
	compose = compose_new_open(mailer->config, message);
	message_delete(message);
	return (compose != NULL) ? TRUE : FALSE;
}


/* mailer_message_open_dialog */
gboolean mailer_message_open_dialog(Mailer * mailer)
{
	gboolean ret;
	GtkWidget * dialog;
	char * filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open..."),
			GTK_WINDOW(mailer->he_window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
					dialog));
	gtk_widget_destroy(dialog);
	if(filename == NULL)
		return FALSE;
	ret = mailer_message_open(mailer, filename);
	g_free(filename);
	return ret;
}


/* mailer_show_about */
static gboolean _about_on_closex(gpointer data);

void mailer_show_about(Mailer * mailer, gboolean show)
{
	GtkWidget * dialog;

	if(mailer->ab_window != NULL)
	{
		if(show)
			gtk_window_present(GTK_WINDOW(mailer->ab_window));
		else
			gtk_widget_hide(mailer->ab_window);
		return;
	}
	dialog = desktop_about_dialog_new();
	mailer->ab_window = dialog;
	g_signal_connect_swapped(G_OBJECT(dialog), "delete-event", G_CALLBACK(
				_about_on_closex), mailer);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(
				mailer->fo_window));
	desktop_about_dialog_set_name(dialog, PACKAGE);
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog, _comments);
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_logo_icon_name(dialog, "mailer");
	desktop_about_dialog_set_translator_credits(dialog,
			_("translator-credits"));
	desktop_about_dialog_set_website(dialog, _website);
	gtk_widget_show(dialog);
}

static gboolean _about_on_closex(gpointer data)
{
	Mailer * mailer = data;

	gtk_widget_hide(mailer->ab_window);
	return TRUE;
}


/* mailer_show_body */
void mailer_show_body(Mailer * mailer, gboolean show)
{
	if(show == TRUE)
		gtk_window_present(GTK_WINDOW(mailer->bo_window));
	else
		gtk_widget_hide(mailer->bo_window);
}


/* mailer_show_headers */
void mailer_show_headers(Mailer * mailer, gboolean show)
{
	if(show == TRUE)
		gtk_window_present(GTK_WINDOW(mailer->he_window));
	else
		gtk_widget_hide(mailer->he_window);
}


/* mailer_show_plugins */
void mailer_show_plugins(Mailer * mailer, gboolean show)
{
	if(show == TRUE)
		gtk_window_present(GTK_WINDOW(mailer->pl_window));
	else
		gtk_widget_hide(mailer->pl_window);
}


/* mailer_show_preferences */
typedef enum _AccountColumn
{
	AC_DATA = 0,
	AC_ACTIVE,
	AC_ENABLED,
	AC_TITLE,
	AC_TYPE,
	AC_WIDGET
} AccountColumn;
#define AC_LAST AC_WIDGET
#define AC_COUNT (AC_LAST + 1)

static void _preferences_accounts(Mailer * mailer, GtkWidget * notebook);
static void _preferences_display(Mailer * mailer, GtkWidget * notebook);
static void _preferences_plugins(Mailer * mailer, GtkWidget * notebook);
static void _preferences_set(Mailer * mailer);
static void _preferences_set_plugins(Mailer * mailer);

/* callbacks */
static gboolean _on_preferences_closex(gpointer data);
static void _on_preferences_response(GtkWidget * widget, gint response,
		gpointer data);
static void _on_preferences_account_delete(gpointer data);
static void _on_preferences_account_edit(gpointer data);
static void _on_preferences_account_move_down(gpointer data);
static void _on_preferences_account_move_up(gpointer data);
static void _on_preferences_account_new(gpointer data);
static void _on_preferences_account_toggle(GtkCellRendererToggle * renderer,
		char * path, gpointer data);
static void _on_preferences_cancel(gpointer data);
static void _on_preferences_ok(gpointer data);
static int _preferences_ok_accounts(Mailer * mailer);
static int _preferences_ok_display(Mailer * mailer);
static int _preferences_ok_plugins(Mailer * mailer);
static int _preferences_ok_save(Mailer * mailer);
static void _preferences_on_plugin_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data);

void mailer_show_preferences(Mailer * mailer, gboolean show)
{
	GtkWidget * vbox;
	GtkWidget * notebook;

	if(mailer->pr_window != NULL)
	{
		if(show)
			gtk_window_present(GTK_WINDOW(mailer->pr_window));
		else
			gtk_widget_hide(mailer->pr_window);
		return;
	}
	mailer->pr_window = gtk_dialog_new_with_buttons(
			_("Mailer preferences"), GTK_WINDOW(mailer->fo_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
#ifndef EMBEDDED
	gtk_window_set_default_size(GTK_WINDOW(mailer->pr_window), 400, 300);
#else
	gtk_window_set_default_size(GTK_WINDOW(mailer->pr_window), 200, 300);
#endif
	g_signal_connect_swapped(G_OBJECT(mailer->pr_window), "delete-event",
			G_CALLBACK(_on_preferences_closex), mailer);
	g_signal_connect(G_OBJECT(mailer->pr_window), "response",
			G_CALLBACK(_on_preferences_response), mailer);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(mailer->pr_window));
#else
	vbox = GTK_DIALOG(mailer->pr_window)->vbox;
#endif
	notebook = gtk_notebook_new();
	/* accounts */
	_preferences_accounts(mailer, notebook);
	/* display */
	_preferences_display(mailer, notebook);
	/* plug-ins */
	_preferences_plugins(mailer, notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	_preferences_set(mailer);
	gtk_widget_show_all(vbox);
	if(show)
		gtk_widget_show(mailer->pr_window);
	else
		gtk_widget_hide(mailer->pr_window);
}

static void _preferences_accounts(Mailer * mailer, GtkWidget * notebook)
{
	GtkWidget * vbox2;
	GtkWidget * vbox3;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkListStore * store;
	GtkCellRenderer * renderer;
	size_t i;
	Account * ac;
	GtkTreeIter iter;

	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	store = gtk_list_store_new(AC_COUNT, G_TYPE_POINTER, G_TYPE_BOOLEAN,
			G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_POINTER);
	for(i = 0; i < mailer->account_cnt; i++)
	{
		ac = mailer->account[i];
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, AC_DATA, ac, AC_ACTIVE, TRUE,
				AC_ENABLED, account_get_enabled(ac),
				AC_TITLE, account_get_title(ac),
				AC_TYPE, account_get_type(ac), -1);
	}
	mailer->pr_accounts = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mailer->pr_accounts),
			TRUE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(mailer->pr_accounts),
			FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mailer->pr_accounts), TRUE);
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(
				_on_preferences_account_toggle), store);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_accounts),
			gtk_tree_view_column_new_with_attributes(_("Enabled"),
				renderer, "active", AC_ENABLED, NULL));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_accounts),
			gtk_tree_view_column_new_with_attributes(_("Name"),
				renderer, "text", AC_TITLE, NULL));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_accounts),
			gtk_tree_view_column_new_with_attributes(_("Type"),
				renderer, "text", AC_TYPE, NULL));
	gtk_container_add(GTK_CONTAINER(widget), mailer->pr_accounts);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* new account */
	widget = gtk_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_preferences_account_new), mailer);
	gtk_box_pack_start(GTK_BOX(vbox3), widget, FALSE, TRUE, 0);
	/* edit account */
#if GTK_CHECK_VERSION(2, 6, 0)
	widget = gtk_button_new_from_stock(GTK_STOCK_EDIT);
#else
	widget = gtk_button_new_with_mnemonic("_Edit");
#endif
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_preferences_account_edit), mailer);
	gtk_box_pack_start(GTK_BOX(vbox3), widget, FALSE, TRUE, 0);
	/* delete account */
	widget = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_preferences_account_delete), mailer);
	gtk_box_pack_start(GTK_BOX(vbox3), widget, FALSE, TRUE, 0);
	/* move up */
	widget = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_preferences_account_move_up), mailer);
	gtk_box_pack_start(GTK_BOX(vbox3), widget, FALSE, TRUE, 0);
	/* move down */
	widget = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	g_signal_connect_swapped(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_preferences_account_move_down), mailer);
	gtk_box_pack_start(GTK_BOX(vbox3), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox2, gtk_label_new(
				_("Accounts")));
}

static void _preferences_display(Mailer * mailer, GtkWidget * notebook)
{
	GtkSizeGroup * group;
	GtkWidget * vbox2;
	GtkWidget * hbox;
	GtkWidget * widget;

	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
	/* default font */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Messages font:"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	mailer->pr_messages_font = gtk_font_button_new();
	widget = mailer->pr_messages_font;
	gtk_size_group_add_widget(group, widget);
	gtk_font_button_set_use_font(GTK_FONT_BUTTON(widget), TRUE);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox2, gtk_label_new(
				_("Display")));
}

static void _preferences_plugins(Mailer * mailer, GtkWidget * notebook)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	mailer->pr_plugins_store = gtk_list_store_new(4, G_TYPE_STRING,
			G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING);
	mailer->pr_plugins = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				mailer->pr_plugins_store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(mailer->pr_plugins),
			FALSE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(mailer->pr_plugins), FALSE);
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(
				_preferences_on_plugin_toggled), mailer);
	column = gtk_tree_view_column_new_with_attributes(_("Enabled"),
			renderer, "active", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_plugins), column);
	column = gtk_tree_view_column_new_with_attributes(NULL,
			gtk_cell_renderer_pixbuf_new(), "pixbuf", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_plugins), column);
	column = gtk_tree_view_column_new_with_attributes(_("Name"),
			gtk_cell_renderer_text_new(), "text", 3, NULL);
	gtk_tree_view_column_set_sort_column_id(column, 3);
	gtk_tree_view_append_column(GTK_TREE_VIEW(mailer->pr_plugins), column);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(
				mailer->pr_plugins_store), 3,
			GTK_SORT_ASCENDING);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(mailer->pr_plugins), TRUE);
	gtk_container_add(GTK_CONTAINER(widget), mailer->pr_plugins);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(
				_("Plug-ins")));
}

static void _preferences_set(Mailer * mailer)
{
	char const * p;

	p = _mailer_get_font(mailer);
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(mailer->pr_messages_font),
			p);
	_preferences_set_plugins(mailer);
}

static void _preferences_set_plugins(Mailer * mailer)
{
	DIR * dir;
	struct dirent * de;
	GtkIconTheme * theme;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	size_t len;
	Plugin * p;
	MailerPluginDefinition * mpd;
	GtkTreeIter iter;
	gboolean enabled;
	GdkPixbuf * icon;

	gtk_list_store_clear(mailer->pr_plugins_store);
	if((dir = opendir(LIBDIR "/" PACKAGE "/plugins")) == NULL)
		return;
	theme = gtk_icon_theme_get_default();
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, de->d_name);
#endif
		if((p = plugin_new(LIBDIR, PACKAGE, "plugins", de->d_name))
				== NULL)
			continue;
		if((mpd = plugin_lookup(p, "plugin")) == NULL)
		{
			plugin_delete(p);
			continue;
		}
		enabled = _mailer_plugin_is_enabled(mailer, de->d_name);
		icon = (mpd->icon != NULL) ? gtk_icon_theme_load_icon(theme,
				mpd->icon, 24, 0, NULL) : NULL;
		if(icon == NULL)
			icon = gtk_icon_theme_load_icon(theme, "gnome-settings",
					24, 0, NULL);
		gtk_list_store_append(mailer->pr_plugins_store, &iter);
		gtk_list_store_set(mailer->pr_plugins_store, &iter,
				MPC_NAME, de->d_name, MPC_ENABLED, enabled,
				MPC_ICON, icon, MPC_NAME_DISPLAY, mpd->name,
				-1);
		plugin_delete(p);
	}
	closedir(dir);
}

static gboolean _on_preferences_closex(gpointer data)
{
	Mailer * mailer = data;

	gtk_widget_hide(mailer->pr_window);
	return TRUE;
}

static void _on_preferences_response(GtkWidget * widget, gint response,
		gpointer data)
{
	Mailer * mailer = data;

	gtk_widget_hide(widget);
	if(response == GTK_RESPONSE_OK)
		_on_preferences_ok(mailer);
	else if(response == GTK_RESPONSE_CANCEL)
		_on_preferences_cancel(mailer);
}


/* _on_preferences_account_new */
/* types */
typedef struct _AccountData
{
	Mailer * mailer;
	char * title;
	AccountIdentity identity;
	unsigned int available;
	Account * account;
	GtkWidget * assistant;
	GtkWidget * settings;
	GtkWidget * confirm;
} AccountData;

/* functions */
static GtkWidget * _assistant_account_select(AccountData * ad);
static GtkWidget * _assistant_account_config(AccountConfig * config);

#if !GTK_CHECK_VERSION(2, 10, 0)
# include "gtkassistant.c"
#endif
static void _on_assistant_cancel(GtkWidget * widget, gpointer data);
static void _on_assistant_close(GtkWidget * widget, gpointer data);
static void _on_assistant_apply(gpointer data);
static void _on_assistant_prepare(GtkWidget * widget, GtkWidget * page,
		gpointer data);
static void _on_entry_changed(GtkWidget * widget, gpointer data);
static void _on_account_type_changed(GtkWidget * widget, gpointer data);

static void _on_preferences_account_new(gpointer data)
{
	Mailer * mailer = data;
	AccountData * ad;
	GtkAssistant * assistant;
	GtkWidget * page;

	if(mailer->available_cnt == 0)
	{
		mailer_error(mailer, _("No account plug-in available"), 0);
		return;
	}
	if((ad = malloc(sizeof(*ad))) == NULL)
	{
		mailer_error(mailer, strerror(errno), 0);
		return;
	}
	ad->mailer = mailer;
	ad->title = strdup("");
	memset(&(ad->identity), 0, sizeof(ad->identity));
	ad->available = 0;
	ad->account = NULL;
	ad->assistant = gtk_assistant_new();
	assistant = GTK_ASSISTANT(ad->assistant);
	g_signal_connect(G_OBJECT(ad->assistant), "cancel", G_CALLBACK(
				_on_assistant_cancel), ad);
	g_signal_connect(G_OBJECT(ad->assistant), "close", G_CALLBACK(
				_on_assistant_close), ad);
	g_signal_connect_swapped(G_OBJECT(ad->assistant), "apply", G_CALLBACK(
				_on_assistant_apply), ad);
	g_signal_connect(G_OBJECT(ad->assistant), "prepare", G_CALLBACK(
				_on_assistant_prepare), ad);
	/* plug-in selection */
	page = _assistant_account_select(ad);
	gtk_assistant_append_page(assistant, page);
	gtk_assistant_set_page_title(assistant, page, _(_title[AAP_INTRO]));
	gtk_assistant_set_page_type(assistant, page, GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_complete(assistant, page, FALSE);
	/* plug-in preferences */
	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_ref(page); /* XXX currently leaks memory */
	ad->settings = page;
	gtk_widget_show(page);
	gtk_assistant_append_page(assistant, page);
	gtk_assistant_set_page_title(assistant, page, _(_title[AAP_SETTINGS]));
	gtk_assistant_set_page_type(assistant, page,
			GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete(assistant, page, TRUE);
	/* confirmation page */
	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	ad->confirm = page;
	gtk_widget_show(page);
	gtk_assistant_append_page(assistant, page);
	gtk_assistant_set_page_title(assistant, page, _(_title[AAP_CONFIRM]));
	gtk_assistant_set_page_type(assistant, page,
			GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_complete(assistant, page, TRUE);
	gtk_widget_show(ad->assistant);
}

static void _on_assistant_cancel(GtkWidget * widget, gpointer data)
{
	_on_assistant_close(widget, data);
}

static void _on_assistant_close(GtkWidget * widget, gpointer data)
{
	AccountData * ad = data;

	if(ad->account != NULL)
		account_delete(ad->account);
	free(ad);
	gtk_widget_destroy(widget);
}

static void _on_assistant_apply(gpointer data)
{
	AccountData * ad = data;
	GtkTreeModel * model;
	GtkTreeIter iter;

	/* XXX check for errors */
	account_init(ad->account);
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(ad->mailer->pr_accounts));
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
#ifdef DEBUG
	fprintf(stderr, "%s%p%s%s%s%s\n", "AC_DATA ", (void *)ad->account,
			", AC_ACTIVE FALSE, AC_ENABLED TRUE, AC_TITLE ",
			account_get_title(ad->account), ", AC_TYPE ",
			account_get_type(ad->account));
#endif
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			AC_DATA, ad->account, AC_ACTIVE, FALSE,
			AC_ENABLED, TRUE,
			AC_TITLE, account_get_title(ad->account),
			AC_TYPE, account_get_type(ad->account), -1);
	ad->account = NULL;
	/* _on_assistant_close is then automatically called */
}

/* on_assistant_prepare */
static GtkWidget * _account_display(Account * account);

static void _on_assistant_prepare(GtkWidget * widget, GtkWidget * page,
		gpointer data)
{
	static int old = AAP_INTRO;
	AccountData * ad = data;
	unsigned int i;
	Account * ac;

	i = gtk_assistant_get_current_page(GTK_ASSISTANT(widget));
	gtk_window_set_title(GTK_WINDOW(widget), _(_title[i]));
	if(i == AAP_SETTINGS)
	{
		gtk_container_remove(GTK_CONTAINER(page), ad->settings);
		if(old == AAP_INTRO)
		{
			if(ad->account != NULL)
				account_delete(ad->account);
			ac = ad->mailer->available[ad->available];
			ad->account = account_new(ad->mailer,
					account_get_type(ac), ad->title, NULL);
		}
		if(ad->account == NULL)
		{
			mailer_error(ad->mailer, error_get(NULL), 0);
			gtk_assistant_set_current_page(GTK_ASSISTANT(widget),
					AAP_INTRO);
			ad->settings = _assistant_account_select(ad);
		}
		else
			ad->settings = _assistant_account_config(
					account_get_config(ad->account));
		gtk_container_add(GTK_CONTAINER(page), ad->settings);
		gtk_widget_show_all(ad->settings);
	}
	else if(i == AAP_CONFIRM)
	{
		gtk_container_remove(GTK_CONTAINER(page), ad->confirm);
		ad->confirm = _account_display(ad->account);
		gtk_container_add(GTK_CONTAINER(page), ad->confirm);
	}
	old = i;
}

/* _assistant_account_select */
static void _on_account_name_changed(GtkWidget * widget, gpointer data);
static void _account_add_label(GtkWidget * box, PangoFontDescription * desc,
		GtkSizeGroup * group, char const * text);

static GtkWidget * _assistant_account_select(AccountData * ad)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkSizeGroup * group;
	PangoFontDescription * desc;
	GtkWidget * widget;
	unsigned int i;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	desc = pango_font_description_new();
	pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
	_account_add_label(hbox, desc, group, _("Account name"));
	widget = gtk_entry_new();
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_account_name_changed), ad);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	/* default identity */
	/* FIXME seems to not be remembered */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	_account_add_label(hbox, NULL, group, _("Your name"));
	widget = gtk_entry_new();
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_entry_changed), &(ad->identity.from));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	_account_add_label(hbox, NULL, group, _("e-mail address"));
	widget = gtk_entry_new();
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_entry_changed), &(ad->identity.email));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	_account_add_label(hbox, NULL, group, _("Type of account"));
#if GTK_CHECK_VERSION(2, 24, 0)
	widget = gtk_combo_box_text_new();
#else
	widget = gtk_combo_box_new_text();
#endif
	/* XXX this works because there is no plug-in list reload
	 *     would it be implemented this will need validation later */
	for(i = 0; i < ad->mailer->available_cnt; i++)
#if GTK_CHECK_VERSION(2, 24, 0)
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget),
				account_get_name(ad->mailer->available[i]));
#else
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget),
				account_get_name(ad->mailer->available[i]));
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_account_type_changed), ad);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	pango_font_description_free(desc);
	gtk_widget_show_all(vbox);
	return vbox;
}

static void _on_account_name_changed(GtkWidget * widget, gpointer data)
{
	AccountData * ad = data;
	int current;
	GtkWidget * page;
	gboolean complete;
	unsigned int i;

	_on_entry_changed(widget, &ad->title);
	current = gtk_assistant_get_current_page(GTK_ASSISTANT(ad->assistant));
	page = gtk_assistant_get_nth_page(GTK_ASSISTANT(ad->assistant),
			current);
	if((complete = (strlen(ad->title) > 0) ? TRUE : FALSE) == TRUE)
		for(i = 0; i < ad->mailer->account_cnt; i++)
			if(strcmp(account_get_title(ad->mailer->account[i]),
						ad->title) == 0)
			{
				complete = FALSE;
				break;
			}
#if GTK_CHECK_VERSION(2, 16, 0)
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget),
			GTK_ENTRY_ICON_SECONDARY,
			(strlen(ad->title) > 0 && complete == FALSE)
			? "gtk-dialog-warning" : NULL);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget),
			GTK_ENTRY_ICON_SECONDARY,
			_("An account with that name already exists"));
#endif
	gtk_assistant_set_page_complete(GTK_ASSISTANT(ad->assistant), page,
			complete);
}

static void _account_add_label(GtkWidget * box, PangoFontDescription * desc,
		GtkSizeGroup * group, char const * text)
{
	static char buf[80]; /* XXX hard-coded size */
	GtkWidget * label;

	snprintf(buf, sizeof(buf), "%s:", text);
	label = gtk_label_new(buf);
	if(desc != NULL)
		gtk_widget_override_font(label, desc);
	if(group != NULL)
		gtk_size_group_add_widget(group, label);
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(label, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);
}

/* _assistant_account_config */
static GtkWidget * _update_string(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _update_password(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _update_file(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _update_uint16(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _update_boolean(AccountConfig * config);

static GtkWidget * _assistant_account_config(AccountConfig * config)
{
	GtkWidget * vbox;
	GtkSizeGroup * group;
	GtkWidget * widget;
	size_t i;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	for(i = 0; config != NULL && config[i].type != ACT_NONE; i++)
	{
		switch(config[i].type)
		{
			case ACT_STRING:
				widget = _update_string(&config[i], NULL,
						group);
				break;
			case ACT_PASSWORD:
				widget = _update_password(&config[i], NULL,
						group);
				break;
			case ACT_FILE:
				widget = _update_file(&config[i], NULL, group);
				break;
			case ACT_UINT16:
				widget = _update_uint16(&config[i], NULL,
						group);
				break;
			case ACT_BOOLEAN:
				widget = _update_boolean(&config[i]);
				break;
			case ACT_SEPARATOR:
				widget = gtk_separator_new(
						GTK_ORIENTATION_HORIZONTAL);
				break;
			default: /* should not happen */
				assert(0);
				continue;
		}
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}
	return vbox;
}

static GtkWidget * _update_string(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_entry_new();
	if(config->value != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), config->value);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_entry_changed), &config->value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static GtkWidget * _update_password(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
	if(config->value != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), config->value);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(
				_on_entry_changed), &config->value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static void _on_file_activated(GtkWidget * widget, gpointer data);

static GtkWidget * _update_file(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_file_chooser_button_new(_("Choose file"),
			GTK_FILE_CHOOSER_ACTION_OPEN);
	if(config->value != NULL)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget),
				config->value);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(widget),
			config->title);
	g_signal_connect(G_OBJECT(widget), "file-set", G_CALLBACK(
				_on_file_activated), &config->value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static void _on_file_activated(GtkWidget * widget, gpointer data)
{
	char * filename;
	char ** value = data;
	char * p = NULL;

	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%s)\n", __func__, filename);
#endif
	if(filename == NULL)
		free(*value);
	else if((p = realloc(*value, strlen(filename) + 1)) == NULL)
	{
		mailer_error(NULL, strerror(errno), 0);
		return;
	}
	*value = p;
	if(filename != NULL)
		strcpy(p, filename);
}

static void _on_uint16_changed(GtkWidget * widget, gpointer data);

static GtkWidget * _update_uint16(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;
	uint16_t u16 = (intptr_t)(config->value);
	gdouble value = u16;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_spin_button_new_with_range(0, 65535, 1);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(widget), 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(
				_on_uint16_changed), &config->value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static void _on_uint16_changed(GtkWidget * widget, gpointer data)
{
	int * value = data;

	*value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
#ifdef DEBUG
	fprintf(stderr, "DEBUG: new value is %d\n", *value);
#endif
}

static void _on_boolean_toggled(GtkWidget * widget, gpointer data);
static GtkWidget * _update_boolean(AccountConfig * config)
{
	GtkWidget * widget;

	widget = gtk_check_button_new_with_label(config->title);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
			config->value != NULL);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(
				_on_boolean_toggled), &config->value);
	return widget;
}

static void _on_boolean_toggled(GtkWidget * widget, gpointer data)
{
	int * value = data;

	*value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static GtkWidget * _display_string(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _display_password(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _display_file(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _display_uint16(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _display_boolean(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group);
static GtkWidget * _account_display(Account * account)
{
	AccountConfig * config;
	AccountConfig p;
	GtkWidget * vbox;
	GtkSizeGroup * group;
	PangoFontDescription * desc;
	GtkWidget * widget;
	unsigned int i;

	config = account_get_config(account);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	p.name = NULL;
	p.title = _("Account name");
	p.value = (void *)account_get_title(account);
	desc = pango_font_description_new();
	pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
	widget = _display_string(&p, desc, group);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	for(i = 0; config[i].type != ACT_NONE; i++)
	{
		switch(config[i].type)
		{
			case ACT_STRING:
				widget = _display_string(&config[i], desc,
						group);
				break;
			case ACT_PASSWORD:
				widget = _display_password(&config[i], desc,
						group);
				break;
			case ACT_FILE:
				widget = _display_file(&config[i], desc, group);
				break;
			case ACT_UINT16:
				widget = _display_uint16(&config[i], desc,
						group);
				break;
			case ACT_BOOLEAN:
				widget = _display_boolean(&config[i], desc,
						group);
				break;
			case ACT_SEPARATOR:
				widget = gtk_separator_new(
						GTK_ORIENTATION_HORIZONTAL);
				break;
			default: /* should not happen */
				assert(0);
				continue;
		}
		gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}
	pango_font_description_free(desc);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget * _display_string(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_label_new(config->value);
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static GtkWidget * _display_file(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	return _display_string(config, desc, group);
}

static GtkWidget * _display_password(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_label_new(_("hidden"));
	desc = pango_font_description_new();
	pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
	gtk_widget_override_font(widget, desc);
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static GtkWidget * _display_uint16(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;
	char buf[6];
	uint16_t u16 = (intptr_t)config->value;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	snprintf(buf, sizeof(buf), "%hu", u16);
	widget = gtk_label_new(buf);
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static GtkWidget * _display_boolean(AccountConfig * config,
		PangoFontDescription * desc, GtkSizeGroup * group)
{
	GtkWidget * hbox;
	GtkWidget * widget;

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	_account_add_label(hbox, desc, group, config->title);
	widget = gtk_label_new(config->value != 0 ? _("Yes") : _("No"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	return hbox;
}

static void _on_entry_changed(GtkWidget * widget, gpointer data)
{
	const char * text;
	char ** value = data;
	char * p;

	text = gtk_entry_get_text(GTK_ENTRY(widget));
	if((p = realloc(*value, strlen(text) + 1)) == NULL)
	{
		mailer_error(NULL, strerror(errno), 0);
		return;
	}
	*value = p;
	strcpy(p, text);
}

static void _on_account_type_changed(GtkWidget * widget, gpointer data)
{
	AccountData * ad = data;

	ad->available = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
#ifdef DEBUG
	fprintf(stderr, "%s%u%s", _("Account type "), ad->available,
			_(" active\n"));
#endif
}

static void _on_preferences_account_toggle(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	GtkListStore * store = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
	gtk_list_store_set(store, &iter, AC_ENABLED, 
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
}

/* _on_preferences_account_edit */
static GtkWidget * _account_edit(Mailer * mailer, Account * account);
static gboolean _account_edit_on_closex(GtkWidget * widget, GdkEvent * event,
		gpointer data);
static void _account_edit_on_response(GtkWidget * widget, gint response,
		gpointer data);

static void _on_preferences_account_edit(gpointer data)
{
	Mailer * mailer = data;
	GtkTreeSelection * selection;
	GtkTreeModel * model;
	GtkTreeIter iter;
	Account * account;
	GtkWidget * widget;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
				mailer->pr_accounts));
	if(gtk_tree_selection_get_selected(selection, &model, &iter) != TRUE)
		return;
	gtk_tree_model_get(model, &iter, AC_DATA, &account, AC_WIDGET, &widget,
			-1);
	if(widget != NULL)
	{
		gtk_window_present(GTK_WINDOW(widget));
		return;
	}
	widget = _account_edit(mailer, account);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, AC_WIDGET, widget, -1);
}

static GtkWidget * _account_edit(Mailer * mailer, Account * account)
{
	char const * title;
	char const * p;
	GtkWidget * window;
	char buf[80];
	GtkWidget * content;
	GtkWidget * notebook;
	GtkWidget * vbox;
	GtkWidget * frame;
	GtkWidget * vbox2;
	GtkWidget * widget;
	GtkWidget * hbox;
	GtkSizeGroup * group;

	title = account_get_title(account);
	snprintf(buf, sizeof(buf), "%s%s", _("Edit account: "), title);
	window = gtk_dialog_new_with_buttons(buf, GTK_WINDOW(mailer->pr_window),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(
				_account_edit_on_closex), NULL);
	g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(
				_account_edit_on_response), NULL);
#if GTK_CHECK_VERSION(2, 14, 0)
	content = gtk_dialog_get_content_area(GTK_DIALOG(window));
#else
	content = GTK_DIALOG(window)->vbox;
#endif
	gtk_container_set_border_width(GTK_CONTAINER(content), 4);
	notebook = gtk_notebook_new();
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/* account tab */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	/* account name */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Account name:"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(widget), title);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* identity */
	frame = gtk_frame_new(_("Identity:"));
	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
	/* identity: name */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Name:"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	if((p = config_get(mailer->config, title, "identity_name")) != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), p);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* identity: address */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Address:"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	if((p = config_get(mailer->config, title, "identity_email")) != NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), p);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* identity: organization */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Organization:"));
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	if((p = config_get(mailer->config, title, "identity_organization"))
			!= NULL)
		gtk_entry_set_text(GTK_ENTRY(widget), p);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new(
				_("Account")));
	/* settings tab */
	/* FIXME this affects the account directly (eg cancel does not work) */
	widget = _assistant_account_config(account_get_config(account));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(
				_("Settings")));
	gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);
	gtk_widget_show_all(window);
	return window;
}

static gboolean _account_edit_on_closex(GtkWidget * widget, GdkEvent * event,
		gpointer data)
{
	_account_edit_on_response(widget, GTK_RESPONSE_CANCEL, data);
	return TRUE;
}

static void _account_edit_on_response(GtkWidget * widget, gint response,
		gpointer data)
{
	/* FIXME really implement */
	gtk_widget_hide(widget);
}

static void _on_preferences_account_delete(gpointer data)
{
	Mailer * mailer = data;
	GtkTreePath * path;
	GtkTreeModel * model;
	GtkTreeIter iter;

	gtk_tree_view_get_cursor(GTK_TREE_VIEW(mailer->pr_accounts), &path,
			NULL);
	if(path == NULL)
		return;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->pr_accounts));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_path_free(path);
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	/* FIXME non-interface code (flag account as deleted and on ok apply) */
}

static void _on_preferences_account_move_down(gpointer data)
{
	Mailer * mailer = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter iter2;
	GtkTreeSelection * treesel;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
				mailer->pr_accounts));
	if(!gtk_tree_selection_get_selected(treesel, &model, &iter))
		return;
	iter2 = iter;
	if(!gtk_tree_model_iter_next(model, &iter))
		return;
	gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &iter2);
}

static void _on_preferences_account_move_up(gpointer data)
{
	Mailer * mailer = data;
	GtkTreeModel * model;
	GtkTreeIter iter;
	GtkTreeIter iter2;
	GtkTreePath * path;
	GtkTreeSelection * treesel;

	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
				mailer->pr_accounts));
	if(!gtk_tree_selection_get_selected(treesel, &model, &iter))
		return;
	path = gtk_tree_model_get_path(model, &iter);
	gtk_tree_path_prev(path);
	gtk_tree_model_get_iter(model, &iter2, path);
	gtk_tree_path_free(path);
	gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &iter2);
}

static void _on_preferences_cancel(gpointer data)
{
	Mailer * mailer = data;

	gtk_widget_hide(mailer->pr_window);
	_preferences_set(mailer);
}

static void _on_preferences_ok(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_preferences(mailer, FALSE);
	if(_preferences_ok_accounts(mailer) != 0
			|| _preferences_ok_display(mailer) != 0
			|| _preferences_ok_plugins(mailer) != 0
			|| _preferences_ok_save(mailer) != 0)
		mailer_error(mailer, _("An error occured while saving"
					" preferences"), 0);
}

static int _preferences_ok_accounts(Mailer * mailer)
{
	GtkTreeModel * model;
	GtkTreeIter iter;
	gboolean loop;
	Account * account;
	gboolean active;
	gboolean enabled;
	char * title;
	size_t title_len;
	char * accounts = NULL;
	size_t accounts_len = 0;
	char * p;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->pr_accounts));
	for(loop = gtk_tree_model_get_iter_first(model, &iter); loop == TRUE;
			loop = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, AC_DATA, &account,
				AC_ACTIVE, &active, AC_ENABLED, &enabled,
				AC_TITLE, &title, -1);
		title_len = strlen(title);
		if(account_config_save(account, mailer->config) != 0)
			return 1;
		if((p = realloc(accounts, accounts_len + title_len + 2))
				== NULL)
		{
			free(accounts);
			return 1;
		}
		accounts = p;
		sprintf(&accounts[accounts_len], "%s%s", accounts_len ? ","
				: "", title);
		accounts_len += title_len + (accounts_len ? 1 : 0);
		if(active)
		{
			if(enabled)
				continue;
#if 0 /* FIXME API+behaviour change here */
			if(mailer_account_disable(mailer, account) == 0)
				gtk_list_store_set(GTK_LIST_STORE(model), &iter,
						AC_ACTIVE, FALSE, -1);
#endif
		}
		else if(enabled && mailer_account_add(mailer, account) == 0)
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
					AC_ACTIVE, TRUE, -1);
	}
	/* FIXME delete accounts that need to be */
	/* FIXME force a refresh of the ones remaining and not just added */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: saved accounts \"%s\"\n", accounts);
#endif
	config_set(mailer->config, "", "accounts", accounts);
	free(accounts);
	return 0;
}

static int _preferences_ok_display(Mailer * mailer)
{
	PangoFontDescription * font = NULL;
	char const * p;

	p = gtk_font_button_get_font_name(GTK_FONT_BUTTON(
				mailer->pr_messages_font));
	config_set(mailer->config, "", "messages_font", p);
	if(p != NULL)
		font = pango_font_description_from_string(p);
	gtk_widget_override_font(mailer->bo_view, font);
	if(font != NULL)
		pango_font_description_free(font);
	return 0;
}

static int _preferences_ok_plugins(Mailer * mailer)
{
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pr_plugins_store);
	GtkTreeIter iter;
	gboolean valid;
	gchar * p;
	gboolean enabled;
	String * value = string_new("");
	String * sep = "";

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, 0, &p, 1, &enabled, -1);
		if(enabled)
		{
			mailer_load(mailer, p);
			string_append(&value, sep);
			string_append(&value, p);
			sep = ",";
		}
		else if(_mailer_plugin_is_enabled(mailer, p))
			mailer_unload(mailer, p);
		g_free(p);
	}
	config_set(mailer->config, NULL, "plugins", value);
	string_delete(value);
	return 0;
}

static int _preferences_ok_save(Mailer * mailer)
{
	int ret;
	char * p;

	if((p = _mailer_get_config_filename()) == NULL)
		return 1;
	ret = config_save(mailer->config, p);
	free(p);
	return ret;
}

static void _preferences_on_plugin_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Mailer * mailer = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(
				mailer->pr_plugins_store), &iter, path);
	gtk_list_store_set(mailer->pr_plugins_store, &iter, 1,
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
}


/* mailer_unload */
int mailer_unload(Mailer * mailer, char const * plugin)
{
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	gchar * p;
	Plugin * pp;
	MailerPluginDefinition * mpd;
	MailerPlugin * mp;
	gboolean enabled = FALSE;

	/* XXX this code is duplicated with _mailer_plugin_is_enabled() */
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, MPC_NAME, &p, MPC_PLUGIN, &pp,
				MPC_MAILERPLUGINDEFINITION, &mpd,
				MPC_MAILERPLUGIN, &mp, -1);
		enabled = (strcmp(p, plugin) == 0) ? TRUE : FALSE;
		g_free(p);
		if(enabled)
			break;
	}
	if(enabled != TRUE)
		return 0;
	gtk_list_store_remove(mailer->pl_store, &iter);
	if(mpd->destroy != NULL)
		mpd->destroy(mp);
	plugin_delete(pp);
	return 0;
}


/* mailer_unselect_all */
void mailer_unselect_all(Mailer * mailer)
{
	GtkTreeSelection * treesel;
#if GTK_CHECK_VERSION(2, 4, 0)
	GtkTextBuffer * tbuf;
	GtkTextMark * mark;
	GtkTextIter iter;
#endif

	if(gtk_window_get_focus(GTK_WINDOW(mailer->fo_window))
			== mailer->bo_view)
	{
#if GTK_CHECK_VERSION(2, 4, 0)
		tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(mailer->bo_view));
		mark = gtk_text_buffer_get_mark(tbuf, "insert");
		gtk_text_buffer_get_iter_at_mark(tbuf, &iter, mark);
		gtk_text_buffer_select_range(tbuf, &iter, &iter);
#endif
		return;
	}
	treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(mailer->he_view));
	gtk_tree_selection_unselect_all(treesel);
}


/* private */
/* functions */
/* accessors */
/* mailer_get_config_filename */
static char * _mailer_get_config_filename(void)
	/* FIXME consider replacing with mailer_save_config() */
{
	char const * homedir;
	char * filename;

	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((filename = malloc(strlen(homedir) + sizeof(MAILER_CONFIG_FILE) + 1))
			== NULL)
		return NULL;
	sprintf(filename, "%s/%s", homedir, MAILER_CONFIG_FILE);
	return filename;
}


/* mailer_plugin_is_enabled */
static gboolean _mailer_plugin_is_enabled(Mailer * mailer, char const * plugin)
{
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pl_store);
	GtkTreeIter iter;
	gchar * p;
	gboolean valid;
	int res;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, MPC_NAME, &p, -1);
		res = strcmp(p, plugin);
		g_free(p);
		if(res == 0)
			return TRUE;
	}
	return FALSE;
}


/* mailer_get_font */
static char const * _mailer_get_font(Mailer * mailer)
{
	char const * p;
	char * q = NULL;
	GtkSettings * settings;
	PangoFontDescription * desc;

	if((p = mailer_get_config(mailer, "messages_font")) != NULL)
		return p;
	settings = gtk_settings_get_default();
	g_object_get(G_OBJECT(settings), "gtk-font-name", &q, NULL);
	if(q == NULL)
		return MAILER_MESSAGES_FONT;
	desc = pango_font_description_from_string(q);
	g_free(q);
	pango_font_description_set_family(desc, "monospace");
	q = pango_font_description_to_string(desc);
	config_set(mailer->config, NULL, "messages_font", q);
	g_free(q);
	pango_font_description_free(desc);
	if((p = config_get(mailer->config, NULL, "messages_font")) != NULL)
		return p;
	return MAILER_MESSAGES_FONT;
}


/* mailer_get_selected_headers */
static GList * _mailer_get_selected_headers(Mailer * mailer)
{
	GtkTreeModel * model;
	GtkTreeSelection * treesel;

	if((model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->he_view)))
			== NULL)
		return NULL;
	if((treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
						mailer->he_view))) == NULL)
		return NULL;
	return gtk_tree_selection_get_selected_rows(treesel, NULL);
}


/* useful */
/* mailer_config_load_account */
static int _mailer_config_load_account(Mailer * mailer, char const * name)
{
	Account * account;
	char const * type;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, name);
#endif
	if((type = config_get(mailer->config, name, "type")) == NULL)
		return -1;
	if((account = account_new(mailer, type, name, mailer->fo_store))
			== NULL)
		return -mailer_error(mailer, error_get(NULL), 1);
	if(account_init(account) != 0
			|| account_config_load(account, mailer->config) != 0
			|| mailer_account_add(mailer, account) != 0)
	{
		account_delete(account);
		return -1;
	}
	return 0;
}


/* mailer_confirm */
static gboolean _mailer_confirm(Mailer * mailer, char const * message)
{
	GtkWidget * dialog;
	int res;

	dialog = gtk_message_dialog_new(GTK_WINDOW(mailer->fo_window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 8, 0)
			"%s", _("Question"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return (res == GTK_RESPONSE_YES) ? TRUE : FALSE;
}


/* mailer_foreach_message_selected */
static void _mailer_foreach_message_selected(Mailer * mailer,
		MailerForeachMessageCallback callback)
{
	GtkTreeModel * model;
	GtkTreeSelection * treesel;
	GList * selected;
	GList * s;
	GtkTreePath * path;
	GtkTreeIter iter;

	if((model = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->he_view)))
			== NULL)
		return;
	if((treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
						mailer->he_view))) == NULL)
		return;
	if((selected = gtk_tree_selection_get_selected_rows(treesel, NULL))
			== NULL)
		return;
	for(s = g_list_first(selected); s != NULL; s = g_list_next(s))
	{
		if((path = s->data) == NULL)
			continue;
		gtk_tree_model_get_iter(model, &iter, path);
		callback(mailer, model, &iter);
	}
	g_list_free(selected);
}


/* mailer_refresh_plugin */
static void _mailer_refresh_plugin(Mailer * mailer)
{
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pl_store);
	GtkTreeIter iter;
	MailerPluginDefinition * mpd;
	MailerPlugin * mp;

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(mailer->pl_combo), &iter)
			!= TRUE)
		return;
	gtk_tree_model_get(model, &iter, MPC_MAILERPLUGINDEFINITION, &mpd,
			MPC_MAILERPLUGIN, &mp, -1);
	if(mpd->refresh == NULL)
		return;
	mpd->refresh(mp, mailer->folder_cur, mailer->message_cur);
}


/* mailer_refresh_title */
static void _mailer_refresh_title(Mailer * mailer)
{
	char buf[80];

	if(mailer->folder_cur == NULL)
		snprintf(buf, sizeof(buf), "%s - %s (%s)", PACKAGE,
				_("Account"),
				account_get_title(mailer->account_cur));
	else
		snprintf(buf, sizeof(buf), "%s - %s (%s)", PACKAGE,
				folder_get_name(mailer->folder_cur),
				account_get_title(mailer->account_cur));
	gtk_window_set_title(GTK_WINDOW(mailer->he_window), buf);
}


/* mailer_update_status */
static void _mailer_update_status(Mailer * mailer)
{
	GtkTreeModel * store;
	int cnt;
	char buf[256];

	if((store = gtk_tree_view_get_model(GTK_TREE_VIEW(mailer->he_view)))
			!= NULL)
	{
		cnt = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),
				NULL);
		snprintf(buf, sizeof(buf), _("%s/%s: %d %s"),
				account_get_name(mailer->account_cur),
				folder_get_name(mailer->folder_cur), cnt,
				(cnt > 1) ? _("messages") : _("message"));
	}
	else
		snprintf(buf, sizeof(buf), "%s", _("Ready"));
	mailer_set_status(mailer, buf);
}


/* mailer_update_view */
static void _mailer_update_view(Mailer * mailer)
{
	GtkTreeStore * store;
	GtkTreeModel * model;
	GtkTextBuffer * tbuf;

	/* display headers */
	if(mailer->folder_cur != NULL && (store = folder_get_messages(
					mailer->folder_cur)) != NULL)
	{
		model = GTK_TREE_MODEL(store);
		/* display message */
		tbuf = account_select(mailer->account_cur, mailer->folder_cur,
				mailer->message_cur);
		gtk_text_view_set_buffer(GTK_TEXT_VIEW(mailer->bo_view), tbuf);
	}
	else
		model = NULL;
	gtk_tree_view_set_model(GTK_TREE_VIEW(mailer->he_view), model);
	_mailer_refresh_plugin(mailer);
	_mailer_refresh_title(mailer);
	_mailer_update_status(mailer);
}


/* callbacks */
/* mailer_on_online_toggled */
static void _mailer_on_online_toggled(gpointer data)
{
	Mailer * mailer = data;
	gboolean online;

	online = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				mailer->st_online));
	mailer_set_online(mailer, online);
}


/* mailer_on_plugin_combo_changed */
static void _mailer_on_plugin_combo_changed(gpointer data)
{
	Mailer * mailer = data;
	GtkTreeModel * model = GTK_TREE_MODEL(mailer->pl_store);
	GtkTreeIter iter;
	gboolean valid;
	GtkWidget * widget;

	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(mailer->pl_store), &iter,
				MPC_WIDGET, &widget, -1);
		gtk_widget_hide(widget);
	}
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(mailer->pl_combo), &iter)
			!= TRUE)
		return;
	gtk_tree_model_get(GTK_TREE_MODEL(mailer->pl_store), &iter, MPC_WIDGET,
			&widget, -1);
	gtk_widget_show(widget);
	_mailer_refresh_plugin(mailer);
}
