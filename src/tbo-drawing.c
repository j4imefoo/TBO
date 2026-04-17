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


#include <glib.h>
#include <cairo.h>
#include <stdio.h>
#include <math.h>
#include "tbo-types.h"
#include "tbo-drawing.h"
#include "dnd.h"
#include "comic.h"
#include "frame.h"
#include "page.h"
#include "tbo-tool-bubble.h"
#include "tbo-tool-doodle.h"
#include "tbo-tooltip.h"

G_DEFINE_TYPE (TboDrawing, tbo_drawing, GTK_TYPE_DRAWING_AREA);

static gboolean
queue_redraw_cb (gpointer data)
{
    TboDrawing *self = TBO_DRAWING (data);

    self->redraw_source_id = 0;
    gtk_widget_queue_draw (GTK_WIDGET (self));
    return G_SOURCE_REMOVE;
}

static void
get_view_size (TboDrawing *self, gint *width, gint *height)
{
    GtkWidget *scrolled;

    scrolled = gtk_widget_get_ancestor (GTK_WIDGET (self), GTK_TYPE_SCROLLED_WINDOW);
    if (scrolled != NULL)
    {
        *width = gtk_widget_get_width (scrolled);
        *height = gtk_widget_get_height (scrolled);
    }
    else
    {
        *width = gtk_widget_get_width (GTK_WIDGET (self));
        *height = gtk_widget_get_height (GTK_WIDGET (self));
    }
}

/* private methods */
static void
draw_func (GtkDrawingArea *area, cairo_t *cr, gint width, gint height, gpointer data)
{
    TboDrawing *self = TBO_DRAWING (area);

    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill (cr);

    tbo_drawing_draw (TBO_DRAWING (area), cr);

    tbo_tooltip_draw (cr);

    // Update drawing helpers
    if (self->tool)
        self->tool->drawing (self->tool, cr);
}

static void
motion_notify_cb (GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer user_data)
{
    TboDrawing *self = TBO_DRAWING (user_data);
    TboPointerEvent event = {
        .x = x / self->zoom,
        .y = y / self->zoom,
        .button = 0,
        .n_press = 0,
        .state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller)),
    };

    if (self->tool)
        self->tool->on_move (self->tool, GTK_WIDGET (self), &event);
}

static void
click_pressed_cb (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
    TboDrawing *self = TBO_DRAWING (user_data);
    TboPointerEvent event = {
        .x = x / self->zoom,
        .y = y / self->zoom,
        .button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture)),
        .n_press = n_press,
        .state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (gesture)),
    };

    gtk_widget_grab_focus (GTK_WIDGET (self));

    if (self->tool) {
        if (TBO_IS_TOOL_BUBBLE (self->tool) || TBO_IS_TOOL_DOODLE (self->tool))
        {
            tbo_toolbar_set_selected_tool (self->tool->tbo->toolbar, TBO_TOOLBAR_SELECTOR);
        }
        self->tool->on_click (self->tool, GTK_WIDGET (self), &event);
    }
}

static void
click_released_cb (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
    TboDrawing *self = TBO_DRAWING (user_data);
    TboPointerEvent event = {
        .x = x / self->zoom,
        .y = y / self->zoom,
        .button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture)),
        .n_press = n_press,
        .state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (gesture)),
    };

    if (self->tool)
        self->tool->on_release (self->tool, GTK_WIDGET (self), &event);
}

/* init methods */

static void
tbo_drawing_init (TboDrawing *self)
{
    GtkEventController *motion;
    GtkGesture *click;

    self->current_frame = NULL;
    self->zoom = 1;
    self->comic = NULL;
    self->tool = NULL;
    self->redraw_source_id = 0;
    gtk_widget_set_focusable (GTK_WIDGET (self), TRUE);

    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (self), draw_func, NULL, NULL);

    motion = gtk_event_controller_motion_new ();
    g_signal_connect (motion, "motion", G_CALLBACK (motion_notify_cb), self);
    gtk_widget_add_controller (GTK_WIDGET (self), motion);

    click = gtk_gesture_click_new ();
    g_signal_connect (click, "pressed", G_CALLBACK (click_pressed_cb), self);
    g_signal_connect (click, "released", G_CALLBACK (click_released_cb), self);
    gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click));
}

static void
tbo_drawing_finalize (GObject *self)
{
    if (TBO_DRAWING (self)->redraw_source_id != 0)
        g_source_remove (TBO_DRAWING (self)->redraw_source_id);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (tbo_drawing_parent_class)->finalize (self);
}

static void
tbo_drawing_class_init (TboDrawingClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = tbo_drawing_finalize;
}

/* object functions */

GtkWidget *
tbo_drawing_new (void)
{
    GtkWidget *drawing;
    drawing = g_object_new (TBO_TYPE_DRAWING, NULL);
    return drawing;
}

GtkWidget *
tbo_drawing_new_with_params (Comic *comic)
{
    GtkWidget *drawing = tbo_drawing_new ();
    TBO_DRAWING (drawing)->comic = comic;
    gtk_widget_set_size_request (drawing, comic->width + 2, comic->height + 2);

    return drawing;
}

void
tbo_drawing_update (TboDrawing *self)
{
    if (self->redraw_source_id != 0)
        return;

    self->redraw_source_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                              queue_redraw_cb,
                                              g_object_ref (self),
                                              g_object_unref);
}

void
tbo_drawing_set_current_frame (TboDrawing *self, Frame *frame)
{
    self->current_frame = frame;
}

Frame *
tbo_drawing_get_current_frame (TboDrawing *self)
{
    return self->current_frame;
}

void
tbo_drawing_draw (TboDrawing *self, cairo_t *cr)
{
    Frame *frame;
    GList *frame_list;
    Page *page;

    int w, h;

    w = self->comic->width;
    h = self->comic->height;
    // white background
    if (tbo_drawing_get_current_frame (self))
        cairo_set_source_rgb(cr, 0, 0, 0);
    else
        cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, w*self->zoom, h*self->zoom);
    cairo_fill(cr);

    cairo_scale (cr, self->zoom, self->zoom);

    page = tbo_comic_get_current_page (self->comic);

    if (!self->current_frame)
    {
        for (frame_list = tbo_page_get_frames (page); frame_list; frame_list = frame_list->next)
        {
            // draw each frame
            frame = (Frame *)frame_list->data;
            tbo_frame_draw (frame, cr);
        }
    }
    else
    {
        tbo_frame_draw_scaled (self->current_frame, cr, w, h);
    }
}

/* TODO this method should be in TboPage */
void
tbo_drawing_draw_page (TboDrawing *self, cairo_t *cr, Page *page, gint w, gint h)
{
    Frame *frame;
    GList *frame_list;

    // white background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    for (frame_list = tbo_page_get_frames (page); frame_list; frame_list = frame_list->next)
    {
        // draw each frame
        frame = (Frame *)frame_list->data;
        tbo_frame_draw (frame, cr);
    }
}

void
tbo_drawing_zoom_in (TboDrawing *self)
{
    self->zoom += ZOOM_STEP;
    tbo_drawing_adjust_scroll (self);
}

void
tbo_drawing_zoom_out (TboDrawing *self)
{
    self->zoom -= ZOOM_STEP;
    tbo_drawing_adjust_scroll (self);
}

void
tbo_drawing_zoom_100 (TboDrawing *self)
{
    self->zoom = 1;
    tbo_drawing_adjust_scroll (self);
}

void
tbo_drawing_zoom_fit (TboDrawing *self)
{
    float z1, z2;
    int w, h;

    get_view_size (self, &w, &h);

    z1 = fabs ((float)w / (float)self->comic->width);
    z2 = fabs ((float)h / (float)self->comic->height);
    self->zoom = z1 < z2 ? z1 : z2;
    tbo_drawing_adjust_scroll (self);
}

gdouble
tbo_drawing_get_zoom (TboDrawing *self)
{
    return self->zoom;
}

void
tbo_drawing_adjust_scroll (TboDrawing *self)
{
    gint width;
    gint height;

    if (!self->comic)
        return;

    width = MAX (1, ceil (self->comic->width * self->zoom));
    height = MAX (1, ceil (self->comic->height * self->zoom));
    gtk_widget_set_size_request (GTK_WIDGET (self), width, height);
    tbo_drawing_update (self);
}

void
tbo_drawing_init_dnd (TboDrawing *self, TboWindow *tbo)
{
    tbo_dnd_setup_drawing_dest (self, tbo);
}
