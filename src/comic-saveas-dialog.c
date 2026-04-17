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
#include <glib/gi18n.h>
#include "comic-saveas-dialog.h"
#include "tbo-file-dialog.h"
#include "tbo-window.h"
#include "comic.h"

gboolean
tbo_comic_save_dialog (GtkWidget *widget, TboWindow *window)
{
    if (window->path)
        return tbo_comic_save (window, window->path);
    else
        return tbo_comic_saveas_dialog (widget, window);
}

gboolean
tbo_comic_saveas_dialog (GtkWidget *widget, TboWindow *window)
{
    gchar *filename;
    char buffer[260];

    g_strlcpy (buffer, window->comic->title, sizeof (buffer));
    if (!g_str_has_suffix ((window->comic->title), ".tbo"))
        strcat (buffer, ".tbo");
    filename = tbo_file_dialog_save_project (window, buffer);

    if (filename != NULL)
    {
        tbo_window_set_browse_path (window, filename);
        if (tbo_comic_save (window, filename))
        {
            tbo_window_set_path (window, filename);
            g_free (filename);
            return TRUE;
        }
        g_free (filename);
    }

    return FALSE;
}
