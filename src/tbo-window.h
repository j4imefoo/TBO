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


#ifndef __TBO_WINDOW__
#define __TBO_WINDOW__

#include <gtk/gtk.h>
#include "tbo-toolbar.h"
#include "tbo-types.h"
#include "tbo-undo.h"

struct _TboWindow
{
    GtkWidget *window;
    GtkWidget *dw_scroll;
    GtkWidget *scroll2;
    GtkWidget *toolarea;
    GtkWidget *notebook;
    GtkWidget *drawing;
    GtkWidget *status;
    GtkWidget *vbox;
    TboToolbar *toolbar;
    TboUndoStack *undo_stack;
    Comic *comic;
    gchar *path;
    gchar *browse_path;
    gchar *export_path;
    gboolean dirty;
    gboolean destroying;
};

TboWindow *tbo_window_new (GtkWidget *window, GtkWidget *dw_scroll, GtkWidget *scroll2, GtkWidget *notebook, GtkWidget *toolarea, GtkWidget *status, GtkWidget *vbox, Comic *comic);
void tbo_window_free (TboWindow *tbo);
gboolean tbo_window_free_cb (GtkWidget *widget, GdkEvent *event, TboWindow *tbo);
gboolean tbo_window_close_request_cb (GtkWindow *window, TboWindow *tbo);
TboWindow * tbo_new_tbo (GtkApplication *app, int width, int height);
void tbo_window_update_status (TboWindow *tbo, int x, int y);
void tbo_empty_tool_area (TboWindow *tbo);
void tbo_window_set_path (TboWindow *tbo, const gchar *path);
void tbo_window_set_browse_path (TboWindow *tbo, const gchar *path);
void tbo_window_set_export_path (TboWindow *tbo, const gchar *path);
gchar *tbo_window_get_open_dir (TboWindow *tbo);
gchar *tbo_window_get_export_dir (TboWindow *tbo);
void tbo_window_mark_dirty (TboWindow *tbo);
void tbo_window_mark_clean (TboWindow *tbo);
gboolean tbo_window_has_unsaved_changes (TboWindow *tbo);
void tbo_window_add_page_widget (TboWindow *tbo, GtkWidget *page);
void tbo_window_remove_page_widget (TboWindow *tbo, gint nth);
gint tbo_window_get_page_count (TboWindow *tbo);
void tbo_window_set_current_tab_page (TboWindow *tbo, gboolean setit);
GtkWidget *create_darea (TboWindow *tbo);
void tbo_window_set_key_binder (TboWindow *tbo, gboolean keyb);
void tbo_window_enter_frame (TboWindow *tbo, Frame *frame);
void tbo_window_leave_frame (TboWindow *tbo);
gboolean tbo_window_undo_cb (GtkWidget *widget, TboWindow *tbo);
gboolean tbo_window_redo_cb (GtkWidget *widget, TboWindow *tbo);


#endif
