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


#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "tbo-object-svg.h"
#include "tbo-drawing.h"
#include "frame.h"
#include "doodle-treeview.h"
#include "dnd.h"
#include "tbo-utils.h"
#include "tbo-files.h"
#include "tbo-widget.h"

void free_gstring_array (GArray *arr);

static GArray *TO_FREE = NULL;
static GHashTable *THUMB_CACHE = NULL;
static TboWindow *TBO = NULL;

static void
free_gstring_data (gpointer data, GClosure *closure)
{
    if (data != NULL)
        g_string_free ((GString *) data, TRUE);
}

void
doodle_free_all (void)
{
    int i;
    if (!TO_FREE) return;
    for (i=0; i<TO_FREE->len; i++)
    {
        free_gstring_array (g_array_index (TO_FREE, GArray*, i));
    }
    g_array_free (TO_FREE, TRUE);
    TO_FREE = NULL;

    if (THUMB_CACHE != NULL)
        g_hash_table_remove_all (THUMB_CACHE);
}

static GdkPixbuf *
get_thumbnail_pixbuf (const gchar *path, const gchar *relative_path)
{
    GdkPixbuf *pixbuf;
    gint width;
    gint height;
    gint max_dim;
    gdouble scale;
    gboolean is_body;

    if (THUMB_CACHE == NULL)
        THUMB_CACHE = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

    pixbuf = g_hash_table_lookup (THUMB_CACHE, path);
    if (pixbuf != NULL)
        return g_object_ref (pixbuf);

    pixbuf = gdk_pixbuf_new_from_file (path, NULL);
    if (pixbuf == NULL)
        return NULL;

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    max_dim = MAX (width, height);
    is_body = g_strrstr (relative_path, "/body/") != NULL || g_str_has_prefix (relative_path, "body/");

    if (is_body && max_dim < 128)
        scale = 128.0 / max_dim;
    else if (is_body && max_dim > 160)
        scale = 160.0 / max_dim;
    else
        scale = 1.0;

    if (scale != 1.0)
    {
        GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                     MAX (1, round (width * scale)),
                                                     MAX (1, round (height * scale)),
                                                     GDK_INTERP_BILINEAR);
        g_object_unref (pixbuf);
        pixbuf = scaled;
        if (pixbuf == NULL)
            return NULL;
    }

    g_hash_table_insert (THUMB_CACHE, g_strdup (path), g_object_ref (pixbuf));
    return pixbuf;
}

void doodle_add_to_free (GArray *arr)
{
    if (!TO_FREE)
        TO_FREE = g_array_new (FALSE, FALSE, sizeof(GArray*));

    g_array_append_val (TO_FREE, arr);
}

void
free_gstring_array (GArray *arr)
{
    int i;
    GString *mystr;

    for (i=0; i<arr->len; i++)
    {
        mystr = g_array_index (arr, GString*, i);
        g_string_free (mystr, TRUE);
    }
    g_array_free (arr, TRUE);
}

GArray *
get_files (gchar *base_dir, gboolean isdir, gboolean bubble_mode)
{
    GError *error = NULL;
    const gchar *filename;
    struct stat filestat;
    int st;
    GArray *array = g_array_new (FALSE, FALSE, sizeof(GString*));

    st = stat (base_dir, &filestat);
    if (st)
        return NULL;

    GDir *dir = g_dir_open (base_dir, 0, &error);

    while ((filename = g_dir_read_name (dir)))
    {
        gchar *complete_dir = g_build_filename (base_dir, filename, NULL);
        st = stat (complete_dir, &filestat);

        if (isdir && bubble_mode && strcmp (filename, "bubble"))
        {
            g_free (complete_dir);
            continue;
        }
        if (!strcmp (filename, "bubble") && !bubble_mode)
        {
            g_free (complete_dir);
            continue;
        }

        if (isdir && S_ISDIR (filestat.st_mode))
        {
            GString *dirname_to_append = g_string_new (complete_dir);
            g_array_append_val (array, dirname_to_append);
        }
        else if (!isdir && !S_ISDIR (filestat.st_mode))
        {
            GString *filename_to_append = g_string_new (complete_dir);
            g_array_append_val (array, filename_to_append);
        }

        g_free (complete_dir);
    }

    g_dir_close (dir);

    return array;
}

GtkWidget *
doodle_add_images (gchar *dir)
{
    int i;
    gchar *dirname;
    GtkWidget *grid;
    GtkWidget *image;
    GtkWidget *button;
    GdkPixbuf *pixbuf;
    int left, top;
    int columns = 2;
    int thumb_width;
    int thumb_height;

    dirname = dir;

    GArray *arr = get_files (dirname, FALSE, FALSE);

    grid = gtk_grid_new ();
    gtk_grid_set_row_spacing (GTK_GRID (grid), 8);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 8);

    GString *mystr;
    for (i=0; i<arr->len; i++)
    {
        const gchar *relative_path;
        int prefix_len;

        top = i / columns;
        left = i % columns;

        mystr = g_array_index (arr, GString*, i);
        prefix_len = tbo_files_prefix_len (mystr->str);
        relative_path = prefix_len > 0 ? mystr->str + prefix_len : mystr->str;

        pixbuf = get_thumbnail_pixbuf (mystr->str, relative_path);
        if (pixbuf == NULL)
            continue;

        thumb_width = gdk_pixbuf_get_width (pixbuf);
        thumb_height = gdk_pixbuf_get_height (pixbuf);

        image = tbo_picture_new_for_pixbuf (pixbuf);
        gtk_picture_set_can_shrink (GTK_PICTURE (image), FALSE);
        gtk_widget_set_size_request (image, thumb_width, thumb_height);
        button = gtk_button_new ();
        gtk_button_set_has_frame (GTK_BUTTON (button), FALSE);
        gtk_widget_set_can_focus (button, FALSE);
        gtk_widget_set_size_request (button, thumb_width + 12, thumb_height + 12);

        tbo_dnd_setup_asset_source (button, mystr->str, relative_path);

        tbo_widget_add_child (button, image);
        gtk_grid_attach (GTK_GRID (grid), button, left, top, 1, 1);
        g_object_unref (pixbuf);
    }

    doodle_add_to_free (arr);

    tbo_widget_show_all (GTK_WIDGET (grid));
    return grid;
}

void
doodle_add_dir_images (gchar *dir, GtkWidget *box)
{
    char base_name[255];
    get_base_name (dir, base_name, 255);
    GtkWidget *expander = gtk_expander_new (base_name);
    GtkWidget *grid = doodle_add_images (dir);
    tbo_widget_add_child (expander, grid);
    gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
    tbo_widget_add_child (box, expander);
}

void
on_expand_cb (GtkExpander *expander, GParamSpec *pspec, GString *str)
{
    GString *mystr2;
    int i;
    GtkWidget *vbox = gtk_expander_get_child (expander);
    int numofchilds = 0;
    if (vbox == NULL || !gtk_expander_get_expanded (expander))
        return;

    numofchilds = tbo_widget_get_child_count (vbox);

    if (numofchilds == 0)
    {
        GArray *subdir = get_files (str->str, TRUE, FALSE);

        if (subdir != NULL && subdir->len > 0)
        {
            for (i=0; i<subdir->len; i++)
            {
                mystr2 = g_array_index (subdir, GString*, i);
                doodle_add_dir_images (mystr2->str, vbox);
            }
        }
        else
        {
            GtkWidget *grid = doodle_add_images (str->str);
            tbo_widget_add_child (vbox, grid);
        }

        if (subdir != NULL)
            free_gstring_array (subdir);
    }
    tbo_widget_show_all (GTK_WIDGET (vbox));
}

GtkWidget *
doodle_setup_tree (TboWindow *tbo, gboolean bubble_mode)
{
    GtkWidget *expander;
    GtkWidget *vbox;
    GtkWidget *vbox2;
    gchar *dirname;

    TBO = tbo;

    dirname = malloc (255*sizeof(char));
    char label_format[255];
    int i, k;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);

    GArray *arr = NULL;
    GString *mystr, *mystr2;

    char **possible_dirs = tbo_files_get_dirs ();
    for (k=0; possible_dirs[k]; k++)
    {
        arr = get_files (possible_dirs[k], TRUE, bubble_mode);
        if (!arr) continue;

        for (i=0; i<arr->len; i++)
        {
            mystr = g_array_index (arr, GString*, i);

            vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
            get_base_name (mystr->str, dirname, 255);
            snprintf (label_format, 255, "<span underline=\"single\" size=\"large\" weight=\"ultrabold\">%s</span>", dirname);
            expander = gtk_expander_new (label_format);
            gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
            tbo_box_pack_start (vbox, expander, FALSE, FALSE, 5);
            tbo_widget_add_child (expander, vbox2);

            mystr2 = g_string_new (mystr->str);
            g_signal_connect_data (GTK_EXPANDER (expander),
                                   "notify::expanded",
                                   G_CALLBACK (on_expand_cb),
                                   mystr2,
                                   free_gstring_data,
                                   0);

            if (bubble_mode)
            {
                gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
                on_expand_cb (GTK_EXPANDER (expander), NULL, mystr2);
            }
        }
        free_gstring_array (arr);
    }
    tbo_files_free (possible_dirs);

    free (dirname);

    return vbox;
}
