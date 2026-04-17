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

Comic *
tbo_comic_new (const char *title, int width, int height)
{
    Comic *new_comic;

    new_comic = malloc(sizeof(Comic));
    snprintf (new_comic->title, 255, "%s", title);
    new_comic->width = width;
    new_comic->height = height;
    new_comic->pages = NULL;
    tbo_comic_new_page (new_comic);

    return new_comic;
}

void
tbo_comic_free (Comic *comic)
{
    GList *p;

    for (p=g_list_first (comic->pages); p; p = g_list_next(p))
    {
        tbo_page_free ((Page *) p->data);
    }

    g_list_free (g_list_first (comic->pages));
    free (comic);
}

Page *
tbo_comic_new_page (Comic *comic){
    Page *page;

    page = tbo_page_new (comic);
    comic->pages = g_list_append (g_list_first (comic->pages), page);

    return page;
}

void
tbo_comic_del_page (Comic *comic, int nth)
{
    Page *page;

    page = (Page *) g_list_nth_data (g_list_first (comic->pages), nth);
    comic->pages = g_list_remove (g_list_first (comic->pages), page);
    tbo_page_free (page);
}

int
tbo_comic_len (Comic *comic)
{
    return g_list_length (g_list_first (comic->pages));
}

int
tbo_comic_page_index (Comic *comic)
{
    return g_list_position ( g_list_first (comic->pages), comic->pages);
}

int
tbo_comic_page_nth (Comic *comic, Page *page)
{
    return g_list_index (g_list_first (comic->pages), page);
}

Page *
tbo_comic_next_page (Comic *comic)
{
    if (comic->pages->next)
    {
        comic->pages = comic->pages->next;
        return tbo_comic_get_current_page (comic);
    }
    return NULL;
}

Page *
tbo_comic_prev_page (Comic *comic)
{
    if (comic->pages->prev)
    {
        comic->pages = comic->pages->prev;
        return tbo_comic_get_current_page (comic);
    }
    return NULL;
}

Page *
tbo_comic_get_current_page (Comic *comic)
{
    return (Page *)comic->pages->data;
}

void
tbo_comic_set_current_page (Comic *comic, Page *page)
{
    comic->pages = g_list_find (g_list_first (comic->pages), page);
}

void
tbo_comic_set_current_page_nth (Comic *comic, int nth)
{
    comic->pages = g_list_nth (g_list_first (comic->pages), nth);
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

    for (p = g_list_first (comic->pages); p; p = g_list_next(p))
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
        oldcomic = window->comic;
        window->comic = newcomic;
        gtk_window_set_title (GTK_WINDOW (window->window), window->comic->title);

        n_pages = tbo_window_get_page_count (window);
        for (nth = n_pages - 1; nth >= 0; nth--)
        {
            tbo_window_remove_page_widget (window, nth);
        }

        tbo_comic_free (oldcomic);

        for (nth=0; nth<tbo_comic_len (window->comic); nth++)
        {
            tbo_window_add_page_widget (window, create_darea (window));
        }

        tbo_window_set_path (window, filename);
        tbo_window_set_current_tab_page (window, TRUE);
        tbo_drawing_adjust_scroll (TBO_DRAWING (window->drawing));
        tbo_drawing_update (TBO_DRAWING (window->drawing));
        tbo_window_update_status (window, 0, 0);
        tbo_window_mark_clean (window);
    }
}
