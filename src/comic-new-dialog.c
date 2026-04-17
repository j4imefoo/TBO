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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "comic-new-dialog.h"
#include "tbo-widget.h"
#include "tbo-window.h"

struct new_comic_dialog_data {
    GMainLoop *loop;
    gint response;
};

static gboolean
new_comic_close_request_cb (GtkWindow *dialog, struct new_comic_dialog_data *data)
{
    if (data->response == GTK_RESPONSE_NONE)
        data->response = GTK_RESPONSE_REJECT;
    g_main_loop_quit (data->loop);
    return TRUE;
}

static void
new_comic_button_cb (GtkButton *button, GtkWindow *dialog)
{
    struct new_comic_dialog_data *data = g_object_get_data (G_OBJECT (dialog), "tbo-new-comic-data");
    gint response = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "tbo-response"));

    data->response = response;
    gtk_window_close (dialog);
}

gboolean
tbo_comic_new_dialog (GtkWidget *widget, TboWindow *window)
{
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *actions;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *spin_w;
    GtkWidget *spin_h;
    GtkAdjustment *adjustment;
    struct new_comic_dialog_data data;

    int width;
    int height;

    dialog = gtk_window_new ();
    gtk_window_set_title (GTK_WINDOW (dialog), _("New Comic"));
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (window->window));
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start (vbox, 12);
    gtk_widget_set_margin_end (vbox, 12);
    gtk_widget_set_margin_top (vbox, 12);
    gtk_widget_set_margin_bottom (vbox, 12);
    tbo_widget_add_child (dialog, vbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new (_("width: "));
    gtk_widget_set_size_request (label, 60, -1);
    gtk_label_set_xalign (GTK_LABEL (label), 0.0);
    gtk_label_set_yalign (GTK_LABEL (label), 0.5);
    adjustment = gtk_adjustment_new (800, 0, 10000, 100, 100, 0);
    spin_w = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
    tbo_box_pack_start (hbox, label, FALSE, FALSE, 0);
    tbo_box_pack_start (hbox, spin_w, TRUE, TRUE, 0);
    tbo_widget_add_child (vbox, hbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new (_("height: "));
    gtk_widget_set_size_request (label, 60, -1);
    gtk_label_set_xalign (GTK_LABEL (label), 0.0);
    gtk_label_set_yalign (GTK_LABEL (label), 0.5);
    adjustment = gtk_adjustment_new (500, 0, 10000, 100, 100, 0);
    spin_h = gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
    tbo_box_pack_start (hbox, label, FALSE, FALSE, 0);
    tbo_box_pack_start (hbox, spin_h, TRUE, TRUE, 0);
    tbo_widget_add_child (vbox, hbox);

    actions = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign (actions, GTK_ALIGN_END);

    button = gtk_button_new_with_mnemonic (_("_Cancel"));
    g_object_set_data (G_OBJECT (button), "tbo-response", GINT_TO_POINTER (GTK_RESPONSE_REJECT));
    g_signal_connect (button, "clicked", G_CALLBACK (new_comic_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    button = gtk_button_new_with_mnemonic (_("_OK"));
    gtk_widget_add_css_class (button, "suggested-action");
    g_object_set_data (G_OBJECT (button), "tbo-response", GINT_TO_POINTER (GTK_RESPONSE_ACCEPT));
    g_signal_connect (button, "clicked", G_CALLBACK (new_comic_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    tbo_widget_add_child (vbox, actions);

    data.loop = g_main_loop_new (NULL, FALSE);
    data.response = GTK_RESPONSE_NONE;
    g_object_set_data (G_OBJECT (dialog), "tbo-new-comic-data", &data);
    g_signal_connect (dialog, "close-request", G_CALLBACK (new_comic_close_request_cb), &data);
    tbo_widget_show_all (dialog);
    gtk_window_present (GTK_WINDOW (dialog));

    g_main_loop_run (data.loop);

    if (data.response == GTK_RESPONSE_ACCEPT)
    {
        width = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_w));
        height = (int) gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_h));
        tbo_new_tbo (gtk_window_get_application (GTK_WINDOW (window->window)), width, height);
    }

    gtk_window_destroy (GTK_WINDOW (dialog));
    g_main_loop_unref (data.loop);

    return FALSE;
}
