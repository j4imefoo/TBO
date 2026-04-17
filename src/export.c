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


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>
#include <string.h>

#include "export.h"
#include "tbo-file-dialog.h"
#include "tbo-drawing.h"
#include "tbo-ui-utils.h"
#include "tbo-types.h"
#include "tbo-widget.h"

static int LOCK = 0;

struct export_spin_args {
    gint current_size;
    gint current_size2;
    GtkWidget *spin2;
    gdouble *scale;
};

struct export_file_args {
    TboWindow *tbo;
    GtkEntry *entry;
};

struct export_dialog_data {
    GMainLoop *loop;
    gint response;
};

static gboolean
export_close_request_cb (GtkWindow *dialog, struct export_dialog_data *data)
{
    if (data->response == GTK_RESPONSE_NONE)
        data->response = GTK_RESPONSE_CANCEL;
    g_main_loop_quit (data->loop);
    return TRUE;
}

static void
export_button_cb (GtkButton *button, GtkWindow *dialog)
{
    struct export_dialog_data *data = g_object_get_data (G_OBJECT (dialog), "tbo-export-data");
    gint response = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "tbo-response"));

    data->response = response;
    gtk_window_close (dialog);
}

static void
show_export_error (TboWindow *tbo, const gchar *message)
{
    tbo_alert_show (GTK_WINDOW (tbo->window), message, NULL);
}

static gboolean
export_size_cb (GtkWidget *widget, struct export_spin_args *args)
{
    if (!LOCK)
    {
        LOCK = 1;
        gint current_size = args->current_size;
        gint current_size2 = args->current_size2;
        gint new_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
        gint new_value;
        if (new_size)
        {
            *(args->scale) = new_size / (gdouble) current_size;
            new_value = (gint) (*(args->scale) * current_size2);
            gtk_spin_button_set_value (GTK_SPIN_BUTTON (args->spin2), new_value);
        }
        LOCK = 0;
    }
    return FALSE;
}

gboolean
filedialog_cb (GtkWidget *widget, gpointer data)
{
    struct export_file_args *args = data;
    const gchar *current_text = gtk_editable_get_text (GTK_EDITABLE (args->entry));
    gchar *filename = tbo_file_dialog_save_export (args->tbo, current_text);

    if (filename != NULL)
    {
        gtk_editable_set_text (GTK_EDITABLE (args->entry), filename);
        tbo_window_set_export_path (args->tbo, filename);
        g_free (filename);
    }
    return FALSE;
}

gboolean
tbo_export (TboWindow *tbo)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr;
    gint width = tbo->comic->width;
    gint height = tbo->comic->height;
    gchar *filename = NULL;
    gchar *base_filename = NULL;
    gchar *format_pages = NULL;
    GList *page_list;
    gint i, n, n2;
    gint response;
    gdouble scale = 1.0;
    gchar *export_to = NULL;
    gint export_to_index;
    struct export_spin_args spin_args;
    struct export_spin_args spin_args2;
    struct export_file_args file_args;

    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *fileinput;
    GtkWidget *filelabel;
    GtkWidget *filebutton;
    GtkWidget *spinw;
    GtkWidget *spinh;
    GtkWidget *dropdown;
    GtkWidget *actions;

    GtkWidget *button;
    gchar *basename = NULL;
    const char *export_formats[] = {
        "guess by extension",
        ".png",
        ".pdf",
        ".svg",
        NULL,
    };

    struct export_dialog_data data;

    dialog = gtk_window_new ();
    gtk_window_set_title (GTK_WINDOW (dialog), _("Export as"));
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tbo->window));
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 420, -1);

    filebutton = gtk_button_new_with_label (_("Choose file"));
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start (vbox, 12);
    gtk_widget_set_margin_end (vbox, 12);
    gtk_widget_set_margin_top (vbox, 12);
    gtk_widget_set_margin_bottom (vbox, 12);
    tbo_widget_add_child (dialog, vbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    filelabel = gtk_label_new (_("Filename: "));
    fileinput = gtk_entry_new ();
    if (tbo->export_path != NULL)
    {
        basename = g_path_get_basename (tbo->export_path);
        gtk_editable_set_text (GTK_EDITABLE (fileinput), basename);
        g_free (basename);
    }
    else
    {
        gtk_editable_set_text (GTK_EDITABLE (fileinput), tbo->comic->title);
    }
    tbo_widget_add_child (hbox, filelabel);
    tbo_widget_add_child (hbox, fileinput);
    tbo_widget_add_child (hbox, filebutton);
    tbo_widget_add_child (vbox, hbox);

    spinw = add_spin_with_label (vbox, _("width: "), tbo->comic->width);
    spinh = add_spin_with_label (vbox, _("height: "), tbo->comic->height);

    spin_args.current_size = tbo->comic->width;
    spin_args.current_size2 = tbo->comic->height;
    spin_args.spin2 = spinh;
    spin_args.scale = &scale;
    g_signal_connect (spinw, "value-changed", G_CALLBACK (export_size_cb), &spin_args);

    spin_args2.current_size = tbo->comic->height;
    spin_args2.current_size2 = tbo->comic->width;
    spin_args2.spin2 = spinw;
    spin_args2.scale = &scale;
    g_signal_connect (spinh, "value-changed", G_CALLBACK (export_size_cb), &spin_args2);

    dropdown = gtk_drop_down_new_from_strings (export_formats);
    gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), 0);
    tbo_widget_add_child (vbox, dropdown);

    actions = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign (actions, GTK_ALIGN_END);

    button = gtk_button_new_with_mnemonic (_("_Cancel"));
    g_object_set_data (G_OBJECT (button), "tbo-response", GINT_TO_POINTER (GTK_RESPONSE_CANCEL));
    g_signal_connect (button, "clicked", G_CALLBACK (export_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    button = gtk_button_new_with_mnemonic (_("_Save"));
    gtk_widget_add_css_class (button, "suggested-action");
    g_object_set_data (G_OBJECT (button), "tbo-response", GINT_TO_POINTER (GTK_RESPONSE_ACCEPT));
    g_signal_connect (button, "clicked", G_CALLBACK (export_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    tbo_widget_add_child (vbox, actions);

    tbo_widget_show_all (GTK_WIDGET (vbox));

    file_args.tbo = tbo;
    file_args.entry = GTK_ENTRY (fileinput);
    g_signal_connect (filebutton, "clicked", G_CALLBACK (filedialog_cb), &file_args);

    data.loop = g_main_loop_new (NULL, FALSE);
    data.response = GTK_RESPONSE_NONE;
    g_object_set_data (G_OBJECT (dialog), "tbo-export-data", &data);
    g_signal_connect (dialog, "close-request", G_CALLBACK (export_close_request_cb), &data);
    gtk_window_present (GTK_WINDOW (dialog));
    g_main_loop_run (data.loop);

    response = data.response;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        width = (gint) (width * scale);
        height = (gint) (height * scale);

        filename = g_strdup (gtk_editable_get_text (GTK_EDITABLE (fileinput)));
        if (filename == NULL || *filename == '\0')
        {
            show_export_error (tbo, _("Please choose a filename to export."));
            g_free (filename);
            gtk_window_destroy (GTK_WINDOW (dialog));
            return FALSE;
        }

        tbo_window_set_export_path (tbo, filename);
        /* 0 guess, 1 png, 2 pdf, 3 svg */
        export_to_index = gtk_drop_down_get_selected (GTK_DROP_DOWN (dropdown));

        switch (export_to_index)
        {
            case 0:
            {
                gchar *dot = strrchr (filename, '.');

                if (dot != NULL && dot[1] != '\0')
                {
                    export_to = g_ascii_strdown (dot + 1, -1);
                    base_filename = g_strndup (filename, dot - filename);
                }
                else
                {
                    base_filename = g_strdup (filename);
                    export_to = g_strdup ("png");
                }
                break;
            }
            case 1:
                export_to = g_strdup ("png");
                base_filename = g_strdup (filename);
                break;
            case 2:
                export_to = g_strdup ("pdf");
                base_filename = g_strdup (filename);
                break;
            case 3:
                export_to = g_strdup ("svg");
                base_filename = g_strdup (filename);
                break;
            default:
                export_to = g_strdup ("png");
                base_filename = g_strdup (filename);
                break;
        }

        if (g_strcmp0 (export_to, "png") != 0 &&
            g_strcmp0 (export_to, "pdf") != 0 &&
            g_strcmp0 (export_to, "svg") != 0)
        {
            g_free (export_to);
            export_to = g_strdup ("png");
        }

        n = g_list_length (tbo->comic->pages);
        n2 = n;
        for (i=0; n; n=n/10, i++);
        format_pages = g_strdup_printf ("%%s%%0%dd.%%s", i);
        for (i=0, page_list = g_list_first (tbo->comic->pages); page_list; i++, page_list = page_list->next)
        {
            gchar *rpath = g_strdup_printf (format_pages, base_filename, i, export_to);
            if (n2 == 1)
            {
                g_free (rpath);
                rpath = g_strdup_printf ("%s.%s", base_filename, export_to);
            }
            // PDF
            if (strcmp (export_to, "pdf") == 0)
            {
                if (!surface)
                {
                    g_free (rpath);
                    rpath = g_strdup_printf ("%s.%s", base_filename, export_to);
                    surface = cairo_pdf_surface_create (rpath, width, height);
                    cr = cairo_create (surface);
                }
            }
            // SVG
            else if (strcmp (export_to, "svg") == 0)
            {
                surface = cairo_svg_surface_create (rpath, width, height);
                cr = cairo_create (surface);
            }
            // PNG or unknown format... default is png
            else
            {
                surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
                cr = cairo_create (surface);
            }

            cairo_scale (cr, scale, scale);

            // drawing the stuff
            tbo_drawing_draw_page (TBO_DRAWING (tbo->drawing), cr, (Page *)page_list->data, width/scale, height/scale);

            if (strcmp (export_to, "pdf") == 0)
                cairo_show_page (cr);
            else if (strcmp (export_to, "png") == 0)
            {
                cairo_status_t status = cairo_surface_write_to_png (surface, rpath);
                if (status != CAIRO_STATUS_SUCCESS)
                    show_export_error (tbo, cairo_status_to_string (status));
            }

            cairo_scale (cr, 1/scale, 1/scale);

            // Not destroying for multipage
            if (strcmp (export_to, "pdf") != 0)
            {
                cairo_surface_destroy (surface);
                cairo_destroy (cr);
                surface = NULL;
            }

            g_free (rpath);
        }

        if (surface)
        {
            cairo_surface_destroy (surface);
            cairo_destroy (cr);
        }
    }

    g_free (format_pages);
    g_free (base_filename);
    g_free (export_to);
    g_free (filename);

    gtk_window_destroy (GTK_WINDOW (dialog));
    g_main_loop_unref (data.loop);

    return FALSE;
}
