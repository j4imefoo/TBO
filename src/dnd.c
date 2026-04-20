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


#include <string.h>
#include <gtk/gtk.h>
#include "dnd.h"
#include "tbo-drawing.h"
#include "frame.h"
#include "tbo-object-svg.h"
#include "tbo-object-pixmap.h"
#include "tbo-files.h"
#include "tbo-window.h"
#include "tbo-tool-selector.h"
#include "tbo-undo.h"
#include "tbo-widget.h"

static TboObjectBase *
create_asset (const gchar *asset_path, gint x, gint y)
{
    if (tbo_files_is_svg_file ((gchar *) asset_path))
        return TBO_OBJECT_BASE (tbo_object_svg_new_with_params (x, y, 0, 0, (gchar *) asset_path));

    return TBO_OBJECT_BASE (tbo_object_pixmap_new_with_params (x, y, 0, 0, (gchar *) asset_path));
}

static void
select_inserted_asset (TboWindow *tbo, Frame *frame, TboObjectBase *asset)
{
    TboToolSelector *selector;
    TboToolBase *current_tool = tbo->toolbar->selected_tool;

    if (current_tool == tbo->toolbar->tools[TBO_TOOLBAR_DOODLE] ||
        current_tool == tbo->toolbar->tools[TBO_TOOLBAR_BUBBLE])
    {
        return;
    }

    tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_SELECTOR);
    selector = TBO_TOOL_SELECTOR (tbo->toolbar->tools[TBO_TOOLBAR_SELECTOR]);
    tbo_tool_selector_set_selected (selector, frame);
    tbo_tool_selector_set_selected_obj (selector, asset);
}

static const gchar *
get_drag_asset_path (GtkWidget *widget, const gchar *key, gpointer fallback)
{
    const gchar *path = g_object_get_data (G_OBJECT (widget), key);

    if (path != NULL)
        return path;

    return fallback;
}

static GdkContentProvider *
drag_prepare_handl (GtkDragSource *source,
                    gdouble        x,
                    gdouble        y,
                    gpointer       user_data)
{
    GtkWidget *widget = GTK_WIDGET (user_data);
    const gchar *asset_path = get_drag_asset_path (widget, "tbo-asset-relative-path", NULL);

    if (asset_path == NULL)
        return NULL;

    return gdk_content_provider_new_typed (G_TYPE_STRING, asset_path);
}

static void
drag_begin_handl (GtkDragSource *source,
                  GdkDrag       *drag,
                  gpointer       user_data)
{
    GtkWidget *widget = GTK_WIDGET (user_data);
    GtkWidget *child = tbo_widget_get_first_child (widget);
    GdkPaintable *paintable = NULL;

    if (GTK_IS_PICTURE (child))
        paintable = gtk_picture_get_paintable (GTK_PICTURE (child));
    else if (GTK_IS_IMAGE (child))
        paintable = gtk_image_get_paintable (GTK_IMAGE (child));

    if (paintable != NULL)
        gtk_drag_source_set_icon (source, paintable, 0, 0);
}

static gboolean
drop_handl (GtkDropTarget *target,
            const GValue  *value,
            gdouble        x,
            gdouble        y,
            gpointer       user_data)
{
    TboWindow *tbo = user_data;
    GtkAdjustment *adj;
    gdouble zoom = tbo_drawing_get_zoom (TBO_DRAWING (tbo->drawing));
    const gchar *asset_path = g_value_get_string (value);
    gint rx;
    gint ry;

    if (asset_path == NULL)
        return FALSE;

    adj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (tbo->dw_scroll));
    rx = tbo_frame_get_base_x ((x + gtk_adjustment_get_value (adj)) / zoom);
    adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tbo->dw_scroll));
    ry = tbo_frame_get_base_y ((y + gtk_adjustment_get_value (adj)) / zoom);

    return tbo_dnd_insert_asset (tbo, asset_path, rx, ry) != NULL;
}

TboObjectBase *
tbo_dnd_insert_asset (TboWindow *tbo, const gchar *asset_path, gint x, gint y)
{
    Frame *frame = tbo_drawing_get_current_frame (TBO_DRAWING (tbo->drawing));
    TboObjectBase *asset;

    if (frame == NULL || asset_path == NULL || *asset_path == '\0')
        return NULL;

    if (x < 0 || y < 0 || x > tbo_frame_get_width (frame) || y > tbo_frame_get_height (frame))
        return NULL;

    asset = create_asset (asset_path, x, y);
    tbo_frame_add_obj (frame, asset);
    tbo_undo_stack_insert (tbo->undo_stack, tbo_action_object_add_new (frame, asset));
    select_inserted_asset (tbo, frame, asset);
    tbo_window_mark_dirty (tbo);
    tbo_drawing_update (TBO_DRAWING (tbo->drawing));
    tbo_toolbar_update (tbo->toolbar);
    return asset;
}

void
tbo_dnd_setup_asset_source (GtkWidget *widget, const gchar *full_path, const gchar *relative_path)
{
    GtkDragSource *source = gtk_drag_source_new ();

    gtk_drag_source_set_actions (source, GDK_ACTION_COPY);
    g_object_set_data_full (G_OBJECT (widget), "tbo-asset-full-path", g_strdup (full_path), g_free);
    g_object_set_data_full (G_OBJECT (widget), "tbo-asset-relative-path", g_strdup (relative_path), g_free);
    g_signal_connect (source, "prepare", G_CALLBACK (drag_prepare_handl), widget);
    g_signal_connect (source, "drag-begin", G_CALLBACK (drag_begin_handl), widget);
    gtk_widget_add_controller (widget, GTK_EVENT_CONTROLLER (source));
}

void
tbo_dnd_setup_drawing_dest (TboDrawing *drawing, TboWindow *tbo)
{
    GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY);

    g_signal_connect (target, "drop", G_CALLBACK (drop_handl), tbo);
    gtk_widget_add_controller (GTK_WIDGET (drawing), GTK_EVENT_CONTROLLER (target));
}
