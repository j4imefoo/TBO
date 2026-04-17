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
#include "config.h"

#include "tbo-window.h"
#include "comic.h"

static void
present_window (TboWindow *tbo)
{
    GdkSurface *surface;
    GdkToplevelLayout *layout;

    gtk_window_present (GTK_WINDOW (tbo->window));
    surface = gtk_native_get_surface (GTK_NATIVE (tbo->window));
    if (surface != NULL && GDK_IS_TOPLEVEL (surface))
    {
        layout = gdk_toplevel_layout_new ();
        gdk_toplevel_present (GDK_TOPLEVEL (surface), layout);
        gdk_toplevel_focus (GDK_TOPLEVEL (surface), GDK_CURRENT_TIME);
        gdk_toplevel_layout_unref (layout);
    }
}

static void
activate_cb (GtkApplication *app, gpointer user_data)
{
    TboWindow *tbo = tbo_new_tbo (app, 800, 450);
    present_window (tbo);
}

static void
open_cb (GtkApplication *app, GFile **files, gint n_files, const gchar *hint, gpointer user_data)
{
    gint i;

    for (i = 0; i < n_files; i++)
    {
        TboWindow *tbo;
        gchar *path = g_file_get_path (files[i]);

        tbo = tbo_new_tbo (app, 800, 450);
        if (path != NULL)
        {
            tbo_comic_open (tbo, path);
            tbo_window_set_path (tbo, path);
            g_free (path);
        }
        present_window (tbo);
    }
}

int main (int argc, char**argv){
    GtkApplication *app;
    int status;

    g_set_application_name ("TBO");

#ifdef ENABLE_NLS
    /* Initialize the i18n stuff */
    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    app = gtk_application_new ("net.danigm.tbo", G_APPLICATION_HANDLES_OPEN);
    g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);
    g_signal_connect (app, "open", G_CALLBACK (open_cb), NULL);

    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
