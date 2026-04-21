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
#include "comic.h"
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

static gchar *
strip_matching_extension (const gchar *filename, const gchar *extension)
{
    const gchar *dot;

    if (filename == NULL || extension == NULL)
        return g_strdup (filename);

    dot = strrchr (filename, '.');
    if (dot != NULL && g_ascii_strcasecmp (dot + 1, extension) == 0)
        return g_strndup (filename, dot - filename);

    return g_strdup (filename);
}

static void
show_export_error (TboWindow *tbo, const gchar *message)
{
    tbo_alert_show (GTK_WINDOW (tbo->window), message, NULL);
}

gboolean
tbo_export_file (TboWindow *tbo,
                 const gchar *filename,
                 const gchar *format_hint,
                 gint width,
                 gint height)
{
    cairo_surface_t *surface = NULL;
    cairo_t *cr = NULL;
    gchar *base_filename = NULL;
    gchar *format_pages = NULL;
    GList *page_list;
    gchar *export_to = NULL;
    gint i, n, n2;
    gboolean exported = FALSE;
    gboolean success = TRUE;

    if (filename == NULL || *filename == '\0' || width <= 0 || height <= 0)
        return FALSE;

    if (format_hint != NULL && *format_hint != '\0')
    {
        export_to = g_ascii_strdown (format_hint, -1);
        base_filename = strip_matching_extension (filename, export_to);
    }
    else
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
    }

    if (g_strcmp0 (export_to, "png") != 0 &&
        g_strcmp0 (export_to, "pdf") != 0 &&
        g_strcmp0 (export_to, "svg") != 0)
    {
        g_free (export_to);
        export_to = g_strdup ("png");
    }

    n = tbo_comic_len (tbo->comic);
    n2 = n;
    for (i = 0; n; n = n / 10, i++);
    format_pages = g_strdup_printf ("%%s%%0%dd.%%s", i);

    for (i = 0, page_list = tbo_comic_get_pages (tbo->comic); page_list; i++, page_list = page_list->next)
    {
        gchar *rpath = g_strdup_printf (format_pages, base_filename, i, export_to);

        if (n2 == 1)
        {
            g_free (rpath);
            rpath = g_strdup_printf ("%s.%s", base_filename, export_to);
        }

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
        else if (strcmp (export_to, "svg") == 0)
        {
            surface = cairo_svg_surface_create (rpath, width, height);
            cr = cairo_create (surface);
        }
        else
        {
            surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
            cr = cairo_create (surface);
        }

        if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS || cairo_status (cr) != CAIRO_STATUS_SUCCESS)
        {
            show_export_error (tbo, cairo_status_to_string (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS ?
                                                            cairo_surface_status (surface) :
                                                            cairo_status (cr)));
            success = FALSE;
            g_free (rpath);
            break;
        }

        tbo_drawing_draw_page (TBO_DRAWING (tbo->drawing), cr, (Page *) page_list->data, width, height);

        if (strcmp (export_to, "pdf") == 0)
            cairo_show_page (cr);
        else if (strcmp (export_to, "png") == 0)
        {
            cairo_status_t status = cairo_surface_write_to_png (surface, rpath);
            if (status != CAIRO_STATUS_SUCCESS)
            {
                show_export_error (tbo, cairo_status_to_string (status));
                success = FALSE;
            }
        }

        if (success)
            exported = TRUE;

        if (strcmp (export_to, "pdf") != 0)
        {
            cairo_surface_destroy (surface);
            cairo_destroy (cr);
            surface = NULL;
            cr = NULL;
        }

        g_free (rpath);

        if (!success)
            break;
    }

    if (surface != NULL)
    {
        cairo_surface_destroy (surface);
        cairo_destroy (cr);
    }

    g_free (format_pages);
    g_free (base_filename);
    g_free (export_to);

    return success && exported;
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
    gint width = tbo_comic_get_width (tbo->comic);
    gint height = tbo_comic_get_height (tbo->comic);
    gchar *filename = NULL;
    gint response;
    gdouble scale = 1.0;
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

    TboDialogRunData data;
    const gchar *format_hint = NULL;

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
        gtk_editable_set_text (GTK_EDITABLE (fileinput), tbo_comic_get_title (tbo->comic));
    }
    tbo_widget_add_child (hbox, filelabel);
    tbo_widget_add_child (hbox, fileinput);
    tbo_widget_add_child (hbox, filebutton);
    tbo_widget_add_child (vbox, hbox);

    spinw = add_spin_with_label (vbox, _("width: "), tbo_comic_get_width (tbo->comic));
    spinh = add_spin_with_label (vbox, _("height: "), tbo_comic_get_height (tbo->comic));

    spin_args.current_size = tbo_comic_get_width (tbo->comic);
    spin_args.current_size2 = tbo_comic_get_height (tbo->comic);
    spin_args.spin2 = spinh;
    spin_args.scale = &scale;
    g_signal_connect (spinw, "value-changed", G_CALLBACK (export_size_cb), &spin_args);

    spin_args2.current_size = tbo_comic_get_height (tbo->comic);
    spin_args2.current_size2 = tbo_comic_get_width (tbo->comic);
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
    g_signal_connect (button, "clicked", G_CALLBACK (tbo_dialog_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    button = gtk_button_new_with_mnemonic (_("_Save"));
    gtk_widget_add_css_class (button, "suggested-action");
    g_object_set_data (G_OBJECT (button), "tbo-response", GINT_TO_POINTER (GTK_RESPONSE_ACCEPT));
    g_signal_connect (button, "clicked", G_CALLBACK (tbo_dialog_button_cb), dialog);
    tbo_widget_add_child (actions, button);

    tbo_widget_add_child (vbox, actions);

    tbo_widget_show_all (GTK_WIDGET (vbox));

    file_args.tbo = tbo;
    file_args.entry = GTK_ENTRY (fileinput);
    g_signal_connect (filebutton, "clicked", G_CALLBACK (filedialog_cb), &file_args);

    tbo_dialog_run_data_init (&data, GTK_RESPONSE_CANCEL);
    g_signal_connect (dialog, "close-request", G_CALLBACK (tbo_dialog_close_request_cb), &data);
    tbo_dialog_run (GTK_WINDOW (dialog), &data);

    response = data.response;

    if (response == GTK_RESPONSE_ACCEPT)
    {
        width = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinw));
        height = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinh));

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

        if (export_to_index == 1)
            format_hint = "png";
        else if (export_to_index == 2)
            format_hint = "pdf";
        else if (export_to_index == 3)
            format_hint = "svg";

        if (!tbo_export_file (tbo, filename, format_hint, width, height))
        {
            gtk_window_destroy (GTK_WINDOW (dialog));
            tbo_dialog_run_data_clear (&data);
            g_free (filename);
            return FALSE;
        }
    }

    g_free (filename);

    gtk_window_destroy (GTK_WINDOW (dialog));
    tbo_dialog_run_data_clear (&data);

    return response == GTK_RESPONSE_ACCEPT;
}
