/*
 * This file is part of TBO, a gnome comic editor
 * Copyright (C) 2010  Daniel Garcia Moreno <dani@danigm.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>
#include "comic.h"
#include "tbo-types.h"
#include "tbo-window.h"
#include "page.h"
#include "comic-load.h"
#include "tbo-drawing.h"
#include "tbo-utils.h"
#include "tbo-widget.h"

struct _Comic
{
    GObject parent_instance;

    char title[255];
    int width;
    int height;
    GList *pages;
    Page *current_page;
};

struct _ComicClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE (Comic, tbo_comic, G_TYPE_OBJECT);

static GList *
comic_page_link (Comic *comic, Page *page)
{
    return g_list_find (comic->pages, page);
}

static void
comic_set_current_page_fallback (Comic *comic, GList *hint)
{
    if (hint != NULL)
        comic->current_page = hint->data;
    else if (comic->pages != NULL)
        comic->current_page = comic->pages->data;
    else
        comic->current_page = NULL;
}

static void
tbo_comic_dispose (GObject *object)
{
    Comic *self = TBO_COMIC (object);

    self->current_page = NULL;

    if (self->pages != NULL)
    {
        g_list_free_full (self->pages, (GDestroyNotify) tbo_page_free);
        self->pages = NULL;
    }

    G_OBJECT_CLASS (tbo_comic_parent_class)->dispose (object);
}

static void
tbo_comic_init (Comic *self)
{
    self->title[0] = '\0';
    self->width = 0;
    self->height = 0;
    self->pages = NULL;
    self->current_page = NULL;
}

static void
tbo_comic_class_init (ComicClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = tbo_comic_dispose;
}

Comic *
tbo_comic_new (const char *title, int width, int height)
{
    Comic *new_comic;

    new_comic = g_object_new (TBO_TYPE_COMIC, NULL);
    snprintf (new_comic->title, 255, "%s", title);
    new_comic->width = width;
    new_comic->height = height;
    tbo_comic_new_page (new_comic);

    return new_comic;
}

void
tbo_comic_free (Comic *comic)
{
    if (comic != NULL)
        g_object_unref (comic);
}

const gchar *
tbo_comic_get_title (Comic *comic)
{
    return comic->title;
}

gint
tbo_comic_get_width (Comic *comic)
{
    return comic->width;
}

gint
tbo_comic_get_height (Comic *comic)
{
    return comic->height;
}

GList *
tbo_comic_get_pages (Comic *comic)
{
    return comic->pages;
}

Page *
tbo_comic_new_page (Comic *comic)
{
    Page *page;

    page = tbo_page_new (comic);
    tbo_comic_insert_page (comic, page, -1);

    return page;
}

void
tbo_comic_insert_page (Comic *comic, Page *page, int nth)
{
    if (nth < 0)
        comic->pages = g_list_append (comic->pages, page);
    else
        comic->pages = g_list_insert (comic->pages, page, nth);

    if (comic->current_page == NULL)
        comic->current_page = page;
}

void
tbo_comic_del_page (Comic *comic, int nth)
{
    Page *page;
    GList *link;
    GList *next_link;
    GList *prev_link;

    page = (Page *) g_list_nth_data (comic->pages, nth);
    if (page == NULL)
        return;

    link = comic_page_link (comic, page);
    next_link = link != NULL ? link->next : NULL;
    prev_link = link != NULL ? link->prev : NULL;

    comic->pages = g_list_remove (comic->pages, page);

    if (comic->current_page == page)
        comic_set_current_page_fallback (comic, next_link != NULL ? next_link : prev_link);

    tbo_page_free (page);
}

int
tbo_comic_len (Comic *comic)
{
    return g_list_length (comic->pages);
}

int
tbo_comic_page_index (Comic *comic)
{
    if (comic->current_page == NULL)
        return -1;

    return g_list_index (comic->pages, comic->current_page);
}

int
tbo_comic_page_nth (Comic *comic, Page *page)
{
    return g_list_index (comic->pages, page);
}

Page *
tbo_comic_next_page (Comic *comic)
{
    GList *current_link;

    if (comic->current_page == NULL)
        return NULL;

    current_link = comic_page_link (comic, comic->current_page);
    if (current_link != NULL && current_link->next != NULL)
    {
        comic->current_page = current_link->next->data;
        return tbo_comic_get_current_page (comic);
    }
    return NULL;
}

Page *
tbo_comic_prev_page (Comic *comic)
{
    GList *current_link;

    if (comic->current_page == NULL)
        return NULL;

    current_link = comic_page_link (comic, comic->current_page);
    if (current_link != NULL && current_link->prev != NULL)
    {
        comic->current_page = current_link->prev->data;
        return tbo_comic_get_current_page (comic);
    }
    return NULL;
}

Page *
tbo_comic_get_current_page (Comic *comic)
{
    return comic->current_page;
}

void
tbo_comic_set_current_page (Comic *comic, Page *page)
{
    if (page == NULL)
    {
        comic->current_page = NULL;
        return;
    }

    comic->current_page = comic_page_link (comic, page) != NULL ? page : NULL;
}

void
tbo_comic_set_current_page_nth (Comic *comic, int nth)
{
    GList *link = g_list_nth (comic->pages, nth);

    comic->current_page = link != NULL ? link->data : NULL;
}

gboolean
tbo_comic_page_first (Comic *comic)
{
    if (tbo_comic_page_index (comic) == 0)
        return TRUE;
    return FALSE;
}

gboolean
tbo_comic_page_last (Comic *comic)
{
    if (tbo_comic_page_index (comic) == tbo_comic_len (comic) - 1)
        return TRUE;
    return FALSE;
}

gboolean
tbo_comic_del_current_page (Comic *comic)
{
    int nth;
    Page *page;

    if (tbo_comic_len (comic) == 1)
        return FALSE;
    nth = tbo_comic_page_index (comic);

    page = tbo_comic_next_page (comic);
    if (page == NULL)
        page = tbo_comic_prev_page (comic);
    tbo_comic_del_page (comic, nth);
    tbo_comic_set_current_page (comic, page);
    return TRUE;
}

gboolean
tbo_comic_save (TboWindow *tbo, char *filename)
{
    GList *p;
    char buffer[255];
    FILE *file = fopen (filename, "w");
    Comic *comic = tbo->comic;

    if (!file)
    {
        perror (_("failed saving"));
        tbo_alert_show (GTK_WINDOW (tbo->window),
                        _("Failed saving"),
                        strerror (errno));
        return FALSE;
    }
    get_base_name (filename, comic->title, 255);
    gtk_window_set_title (GTK_WINDOW (tbo->window), comic->title);

    snprintf (buffer, 255, "<tbo width=\"%d\" height=\"%d\">\n",
              comic->width,
              comic->height);
    fwrite (buffer, sizeof (char), strlen (buffer), file);

    for (p = comic->pages; p; p = g_list_next (p))
    {
        tbo_page_save ((Page *) p->data, file);
    }

    snprintf (buffer, 255, "</tbo>\n");
    fwrite (buffer, sizeof (char), strlen (buffer), file);
    fclose (file);
    tbo_window_mark_clean (tbo);
    return TRUE;
}

void
tbo_comic_open (TboWindow *window, char *filename)
{
    Comic *newcomic = tbo_comic_load (filename);
    Comic *oldcomic;
    int nth;
    int n_pages;

    if (newcomic)
    {
        tbo_window_reset_document_state (window);
        oldcomic = window->comic;

        n_pages = tbo_window_get_page_count (window);
        for (nth = n_pages - 1; nth >= 0; nth--)
        {
            tbo_window_remove_page_widget (window, nth);
        }

        window->comic = newcomic;
        gtk_window_set_title (GTK_WINDOW (window->window), tbo_comic_get_title (window->comic));
        tbo_comic_free (oldcomic);

        for (nth = 0; nth < tbo_comic_len (window->comic); nth++)
        {
            tbo_window_add_page_widget (window, create_darea (window));
        }

        tbo_window_set_path (window, filename);
        tbo_window_set_current_tab_page (window, TRUE);
        tbo_drawing_adjust_scroll (TBO_DRAWING (window->drawing));
        tbo_drawing_update (TBO_DRAWING (window->drawing));
        tbo_window_refresh_status (window);
        tbo_window_mark_clean (window);
    }
}
