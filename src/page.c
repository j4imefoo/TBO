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
#include <string.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "comic.h"
#include "page.h"
#include "frame.h"

struct _Page
{
    GObject parent_instance;

    GList *frames;
    Frame *current_frame;
};

struct _PageClass
{
    GObjectClass parent_class;
};

G_DEFINE_TYPE (Page, tbo_page, G_TYPE_OBJECT);

static GList *
page_frame_link (Page *page, Frame *frame)
{
    return g_list_find (page->frames, frame);
}

static void
page_set_current_frame_fallback (Page *page, GList *hint)
{
    if (hint != NULL)
        page->current_frame = hint->data;
    else if (page->frames != NULL)
        page->current_frame = page->frames->data;
    else
        page->current_frame = NULL;
}

static void
tbo_page_dispose (GObject *object)
{
    Page *self = TBO_PAGE (object);

    self->current_frame = NULL;

    if (self->frames != NULL)
    {
        g_list_free_full (self->frames, (GDestroyNotify) tbo_frame_free);
        self->frames = NULL;
    }

    G_OBJECT_CLASS (tbo_page_parent_class)->dispose (object);
}

static void
tbo_page_init (Page *self)
{
    self->frames = NULL;
    self->current_frame = NULL;
}

static void
tbo_page_class_init (PageClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = tbo_page_dispose;
}

Page *
tbo_page_new (Comic *comic)
{
    (void) comic;
    return g_object_new (TBO_TYPE_PAGE, NULL);
}

void
tbo_page_free (Page *page)
{
    if (page != NULL)
        g_object_unref (page);
}

Frame *
tbo_page_new_frame (Page *page, int x, int y, int w, int h)
{
    Frame *frame;

    frame = tbo_frame_new (x, y, w, h);
    tbo_page_insert_frame (page, frame, -1);

    return frame;
}

void
tbo_page_add_frame (Page *page, Frame *frame)
{
    tbo_page_insert_frame (page, frame, -1);
}

void
tbo_page_insert_frame (Page *page, Frame *frame, int nth)
{
    if (nth < 0)
        page->frames = g_list_append (page->frames, frame);
    else
        page->frames = g_list_insert (page->frames, frame, nth);

    if (page->current_frame == NULL)
        page->current_frame = frame;
}

void
tbo_page_del_frame_by_index (Page *page, int nth)
{
    Frame *frame;

    frame = (Frame *) g_list_nth_data (page->frames, nth);
    tbo_page_del_frame (page, frame);
}

void
tbo_page_del_frame (Page *page, Frame *frame)
{
    GList *link;
    GList *next_link;
    GList *prev_link;

    if (frame == NULL)
        return;

    link = page_frame_link (page, frame);
    if (link == NULL)
        return;

    next_link = link->next;
    prev_link = link->prev;
    page->frames = g_list_remove (page->frames, frame);

    if (page->current_frame == frame)
        page_set_current_frame_fallback (page, next_link != NULL ? next_link : prev_link);

    tbo_frame_free (frame);
}

int
tbo_page_len (Page *page)
{
    return g_list_length (page->frames);
}

int
tbo_page_frame_index (Page *page)
{
    if (page->current_frame == NULL)
        return 0;

    return g_list_index (page->frames, page->current_frame) + 1;
}

int
tbo_page_frame_nth (Page *page, Frame *frame)
{
    return g_list_index (page->frames, frame);
}

gboolean
tbo_page_frame_first (Page *page)
{
    if (tbo_page_frame_index (page) == 1)
        return TRUE;
    return FALSE;
}

gboolean
tbo_page_frame_last (Page *page)
{
    if (tbo_page_frame_index (page) == tbo_page_len (page))
        return TRUE;
    return FALSE;
}

Frame *
tbo_page_next_frame (Page *page)
{
    GList *current_link;

    if (page->current_frame == NULL)
        return NULL;

    current_link = page_frame_link (page, page->current_frame);
    if (current_link != NULL && current_link->next != NULL)
    {
        page->current_frame = current_link->next->data;
        return tbo_page_get_current_frame (page);
    }
    return NULL;
}

Frame *
tbo_page_prev_frame (Page *page)
{
    GList *current_link;

    if (page->current_frame == NULL)
        return NULL;

    current_link = page_frame_link (page, page->current_frame);
    if (current_link != NULL && current_link->prev != NULL)
    {
        page->current_frame = current_link->prev->data;
        return tbo_page_get_current_frame (page);
    }
    return NULL;
}

Frame *
tbo_page_get_current_frame (Page *page)
{
    return page->current_frame;
}

void
tbo_page_set_current_frame (Page *page, Frame *frame)
{
    if (frame == NULL)
    {
        page->current_frame = NULL;
        return;
    }

    page->current_frame = page_frame_link (page, frame) != NULL ? frame : NULL;
}

Frame *
tbo_page_first_frame (Page *page)
{
    if (page->frames != NULL)
    {
        page->current_frame = page->frames->data;
        return page->current_frame;
    }

    page->current_frame = NULL;
    return NULL;
}

GList *
tbo_page_get_frames (Page *page)
{
    return page->frames;
}

void
tbo_page_save (Page *page, FILE *file)
{
    char buffer[255];
    GList *f;

    snprintf (buffer, 255, " <page>\n");
    fwrite (buffer, sizeof (char), strlen (buffer), file);

    for (f = page->frames; f; f = g_list_next (f))
    {
        tbo_frame_save ((Frame *) f->data, file);
    }

    snprintf (buffer, 255, " </page>\n");
    fwrite (buffer, sizeof (char), strlen (buffer), file);
}
