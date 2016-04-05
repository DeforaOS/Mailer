/* $Id$ */
/* Copyright (c) 2010 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* Redistribution and use in source and binary forms, with or without
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
 * 3. Neither the name of the authors nor the names of the contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. */




/* GtkAssistant */
/* macros */
# define GTK_ASSISTANT(widget) ((GtkAssistant*)(widget))

/* types */
typedef struct _GtkAssistant
{
	GtkWidget * window;
	GtkWidget * frame;
	GtkWidget * label;
	GtkWidget ** page;
	guint page_cnt;
	guint page_cur;
} GtkAssistant;

typedef enum _GtkAssistantPageType
{
	GTK_ASSISTANT_PAGE_CONTENT,
	GTK_ASSISTANT_PAGE_INTRO,
	GTK_ASSISTANT_PAGE_CONFIRM,
	GTK_ASSISTANT_PAGE_SUMMARY,
	GTK_ASSISTANT_PAGE_PROGRESS
} GtkAssistantPageType;


/* functions */
static GtkWidget * gtk_assistant_new(void);
static gint gtk_assistant_get_current_page(GtkAssistant * widget);
static GtkWidget * gtk_assistant_get_nth_page(GtkAssistant * widget, gint page);
static void gtk_assistant_set_current_page(GtkAssistant * widget, gint page);
static gint gtk_assistant_append_page(GtkAssistant * widget, GtkWidget * page);
static void gtk_assistant_set_page_type(GtkAssistant * widget, GtkWidget * page,
		GtkAssistantPageType type);
static void gtk_assistant_set_page_title(GtkAssistant * widget,
		GtkWidget * page, const gchar * title);
static void gtk_assistant_set_page_complete(GtkAssistant * widget,
		GtkWidget * page, gboolean complete);

/* useful */
static int _gtkassistant_error(char const * message, int ret);

/* callbacks */
static gboolean _on_gtkassistant_closex(GtkWidget * widget, GdkEvent * event,
		gpointer data);
static void _on_gtkassistant_cancel(GtkWidget * widget, gpointer data);
static void _on_gtkassistant_forward(GtkWidget * widget, gpointer data);
static void _on_gtkassistant_back(GtkWidget * widget, gpointer data);


/* functions */
/* gtk_assistant_new */
static GtkWidget * gtk_assistant_new(void)
{
	GtkAssistant * assistant;
	GtkWidget * window;
	GtkWidget * frame;
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkWidget * hbox;

	if((assistant = calloc(1, sizeof(*assistant))) == NULL)
		_gtkassistant_error("out of memory", 0);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	g_object_set_data(G_OBJECT(window), "assistant", assistant);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(
				_on_gtkassistant_closex), assistant);
	vbox = gtk_vbox_new(FALSE, 4);
	/* frame */
	frame = gtk_frame_new("");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	/* navigation buttons */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_gtkassistant_forward), assistant);
	gtk_widget_show(widget);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_GO_BACK);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_gtkassistant_back), assistant);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(
				_on_gtkassistant_cancel), assistant);
	gtk_widget_show(widget);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	if(assistant == NULL)
		return window;
	assistant->window = window;
	assistant->frame = frame;
	assistant->label = gtk_frame_get_label_widget(GTK_FRAME(frame));
	return window;
}


/* useful */
static int _gtkassistant_error(char const * message, int ret)
{
	fprintf(stderr, "%s%s%s", "*** GtkAssistant wrapper ", message,
			" ***\n");
	return ret;
}


/* callbacks */
static gboolean _on_gtkassistant_closex(GtkWidget * widget, GdkEvent * event,
		gpointer data)
{
	_on_gtkassistant_cancel(NULL, data);
	return TRUE;
}


static void _on_gtkassistant_cancel(GtkWidget * widget, gpointer data)
{
	GtkAssistant * assistant = data;

	/* FIXME signals to handle? */
	gtk_widget_destroy(assistant->window);
	free(assistant->page);
	free(assistant);
}


static void _on_gtkassistant_forward(GtkWidget * widget, gpointer data)
{
	GtkAssistant * assistant = data;

	/* FIXME */
}


static void _on_gtkassistant_back(GtkWidget * widget, gpointer data)
{
	GtkAssistant * assistant = data;

	/* FIXME */
}


static gint gtk_assistant_get_current_page(GtkAssistant * widget)
{
	/* FIXME */
	return -1;
}


static GtkWidget * gtk_assistant_get_nth_page(GtkAssistant * widget, gint page)
{
	GtkAssistant * assistant = widget;

	if(page < 0 || (guint)page >= assistant->page_cnt)
		return NULL;
	return assistant->page[page];
}


static void gtk_assistant_set_current_page(GtkAssistant * widget, gint page)
{
	/* FIXME */
}


static gint gtk_assistant_append_page(GtkAssistant * widget, GtkWidget * page)
{
	GtkAssistant * assistant;
	GtkWidget ** p;

	if((assistant = g_object_get_data(G_OBJECT(widget), "assistant"))
			== NULL)
		return _gtkassistant_error("data not found", -1);
	if((p = realloc(assistant->page, sizeof(*p) * (assistant->page_cnt+1)))
			== NULL)
		return _gtkassistant_error(strerror(errno), -1);
	assistant->page = p;
	assistant->page[assistant->page_cnt] = page;
	if(assistant->page_cnt == 0)
		gtk_container_add(GTK_CONTAINER(assistant->frame), page);
	gtk_widget_show(page);
	return assistant->page_cnt++;
}


static void gtk_assistant_set_page_type(GtkAssistant * widget, GtkWidget * page,
		GtkAssistantPageType type)
{
	/* FIXME */
}


static void gtk_assistant_set_page_title(GtkAssistant * widget,
		GtkWidget * page, const gchar * title)
{
	/* FIXME */
}


static void gtk_assistant_set_page_complete(GtkAssistant * widget,
		GtkWidget * page, gboolean complete)
{
	/* FIXME */
}
