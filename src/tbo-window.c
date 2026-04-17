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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include "tbo-types.h"
#include "tbo-window.h"
#include "comic.h"
#include "page.h"
#include "ui-menu.h"
#include "tbo-toolbar.h"
#include "tbo-drawing.h"
#include "tbo-tool-selector.h"
#include "tbo-tooltip.h"
#include "tbo-utils.h"
#include "tbo-widget.h"
#include "comic-saveas-dialog.h"

static gboolean KEY_BINDER = TRUE;

static void
apply_theme_preferences (void)
{
    GtkSettings *settings = gtk_settings_get_default ();

    if (settings == NULL)
        return;

    if (g_object_class_find_property (G_OBJECT_GET_CLASS (settings), "gtk-theme-name") != NULL)
    {
        g_object_set (settings, "gtk-theme-name", "Adwaita-dark", NULL);
    }

    if (g_object_class_find_property (G_OBJECT_GET_CLASS (settings), "gtk-application-prefer-dark-theme") != NULL)
    {
        g_object_set (settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }
}

static void
apply_window_icon (GtkWidget *window)
{
    gtk_window_set_default_icon_name ("tbo");
    gtk_window_set_icon_name (GTK_WINDOW (window), "tbo");
}

static GtkWidget *
get_page_widget (TboWindow *tbo, gint nth)
{
    return gtk_notebook_get_nth_page (GTK_NOTEBOOK (tbo->notebook), nth);
}

static GtkWidget *
create_page_tab_label (gint nth)
{
    gchar *text = g_strdup_printf (_("Page %d"), nth + 1);
    GtkWidget *label = gtk_label_new (text);

    g_free (text);
    return label;
}

static void
refresh_page_tab_labels (TboWindow *tbo)
{
    gint i;
    gint count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tbo->notebook));

    for (i = 0; i < count; i++)
    {
        GtkWidget *page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (tbo->notebook), i);
        GtkWidget *label = create_page_tab_label (i);

        gtk_notebook_set_tab_label (GTK_NOTEBOOK (tbo->notebook), page, label);
    }
}

static gboolean
notebook_switch_page_cb (GtkNotebook *notebook,
                         GtkWidget   *page,
                         guint        page_num,
                         TboWindow   *tbo)
{
    tbo_comic_set_current_page_nth (tbo->comic, page_num);
    tbo_window_set_current_tab_page (tbo, FALSE);
    tbo_toolbar_update (tbo->toolbar);
    tbo_window_update_status (tbo, 0, 0);
    tbo_drawing_adjust_scroll (TBO_DRAWING (tbo->drawing));
    return FALSE;
}

static void
destroy_cb (GtkWidget *widget, TboWindow *tbo)
{
    tbo_window_free (tbo);
}

static void
set_window_path (gchar **slot, const gchar *path)
{
    g_free (*slot);
    *slot = path != NULL ? g_strdup (path) : NULL;
}

static gchar *
get_state_file_path (void)
{
    gchar *dir = g_build_filename (g_get_user_config_dir (), "tbo", NULL);
    gchar *path;

    g_mkdir_with_parents (dir, 0755);
    path = g_build_filename (dir, "state.ini", NULL);
    g_free (dir);
    return path;
}

static gchar *
load_persisted_path (const gchar *key)
{
    GKeyFile *kf = g_key_file_new ();
    gchar *state_file = get_state_file_path ();
    gchar *value = NULL;

    if (g_key_file_load_from_file (kf, state_file, G_KEY_FILE_NONE, NULL))
        value = g_key_file_get_string (kf, "paths", key, NULL);

    g_key_file_unref (kf);
    g_free (state_file);
    return value;
}

static void
store_persisted_path (const gchar *key, const gchar *value)
{
    GKeyFile *kf = g_key_file_new ();
    gchar *state_file = get_state_file_path ();
    gchar *content;
    gsize len;

    g_key_file_load_from_file (kf, state_file, G_KEY_FILE_NONE, NULL);
    g_key_file_set_string (kf, "paths", key, value);
    content = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (state_file, content, len, NULL);

    g_free (content);
    g_key_file_unref (kf);
    g_free (state_file);
}

static gchar *
get_dirname_or_home (const gchar *path)
{
    if (path != NULL && *path != '\0')
        return g_path_get_dirname (path);

    return g_strdup (g_get_home_dir ());
}

static gboolean
confirm_close (TboWindow *tbo)
{
    gint response;
    static const gchar *buttons[] = {
        "_Cancel",
        "_Don't Save",
        "_Save",
        NULL,
    };

    if (!tbo_window_has_unsaved_changes (tbo))
        return TRUE;

    response = tbo_alert_choose (GTK_WINDOW (tbo->window),
                                 _("Do you want to save your work before closing?"),
                                 _("Unsaved changes will be lost if you close this window."),
                                 buttons,
                                 0,
                                 2);

    if (response == 2)
        return tbo_comic_save_dialog (NULL, tbo);

    return response == 1;
}

static void
update_statusbar (TboWindow *tbo, int x, int y)
{
    char buffer[200];
    gboolean in_frame_view = tbo_drawing_get_current_frame (TBO_DRAWING (tbo->drawing)) != NULL;

    if (in_frame_view)
    {
        snprintf (buffer, 200, _("editing frame [ %5d,%5d ] | Esc: back to page"), x, y);
    }
    else
    {
        snprintf (buffer, 200, _("page: %d of %d [ %5d,%5d ] | frames: %d | Enter: frame"),
                  tbo_comic_page_index (tbo->comic),
                  tbo_comic_len (tbo->comic),
                  x, y,
                  tbo_page_len (tbo_comic_get_current_page (tbo->comic)));
    }
    gtk_label_set_text (GTK_LABEL (tbo->status), buffer);
}

static void
load_app_css (void)
{
    static gboolean loaded = FALSE;
    GtkCssProvider *provider;
    const gchar *css;

    if (loaded)
        return;

    css =
        "#tbo-toolbar {"
        "  background: shade(@theme_bg_color, 1.02);"
        "  border-bottom: 1px solid alpha(@theme_fg_color, 0.08);"
        "}"
        ".tbo-toolbar-section button {"
        "  min-width: 36px;"
        "  min-height: 36px;"
        "}"
        "#tbo-sidebar {"
        "  background: shade(@theme_base_color, 0.98);"
        "}"
        "#tbo-status {"
        "  padding: 8px 12px;"
        "  border-top: 1px solid alpha(@theme_fg_color, 0.08);"
        "  background: shade(@theme_bg_color, 1.01);"
        "}"
        "#tbo-toolarea {"
        "  padding: 12px;"
        "}";

    provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_string (provider, css);
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                GTK_STYLE_PROVIDER (provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);
    loaded = TRUE;
}

static gboolean
on_key_cb (GtkEventControllerKey *controller,
           guint                  keyval,
           guint                  keycode,
           GdkModifierType        state,
           TboWindow             *tbo)
{
    TboToolBase *tool;
    TboDrawing *drawing = TBO_DRAWING (tbo->drawing);
    TboKeyEvent event = { .keyval = keyval, .state = state };

    if (tbo->drawing == NULL || !gtk_widget_has_focus (GTK_WIDGET (tbo->drawing)))
        return FALSE;

    tool = tbo_toolbar_get_selected_tool (tbo->toolbar);
    if (tool)
        tool->on_key (tool, GTK_WIDGET (tbo->window), event);

    tbo_window_update_status (tbo, 0, 0);

    if (KEY_BINDER && (state & (GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_META_MASK)) == 0)
    {
        switch (keyval)
        {
            case GDK_KEY_plus:
                tbo_drawing_zoom_in (drawing);
                break;
            case GDK_KEY_minus:
                tbo_drawing_zoom_out (drawing);
                break;
            case GDK_KEY_1:
                tbo_drawing_zoom_100 (drawing);
                break;
            case GDK_KEY_2:
                tbo_drawing_zoom_fit (drawing);
                break;
            case GDK_KEY_s:
                tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_SELECTOR);
                break;
            case GDK_KEY_t:
                tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_TEXT);
                break;
            case GDK_KEY_d:
                tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_DOODLE);
                break;
            case GDK_KEY_b:
                tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_BUBBLE);
                break;
            case GDK_KEY_f:
                tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_FRAME);
                break;
            default:
                break;
        }
    }
    return FALSE;
}

static gboolean
global_key_cb (GtkEventControllerKey *controller,
               guint                  keyval,
               guint                  keycode,
               GdkModifierType        state,
               TboWindow             *tbo)
{
    if (keyval == GDK_KEY_Escape &&
        tbo->drawing != NULL &&
        tbo_drawing_get_current_frame (TBO_DRAWING (tbo->drawing)) != NULL)
    {
        tbo_window_leave_frame (tbo);
        return TRUE;
    }

    return FALSE;
}

static void
on_move_cb (GtkEventControllerMotion *controller,
            gdouble                   x,
            gdouble                   y,
            TboWindow                *tbo)
{
    update_statusbar (tbo, (int)x, (int)y);
}

TboWindow *
tbo_window_new (GtkWidget *window, GtkWidget *dw_scroll,
                GtkWidget *scroll2,
                GtkWidget *notebook, GtkWidget *toolarea,
                GtkWidget *status, GtkWidget *vbox, Comic *comic)
{
    TboWindow *tbo;

    tbo = malloc (sizeof (TboWindow));
    tbo->window = window;
    tbo->dw_scroll = dw_scroll;
    tbo->scroll2 = scroll2;
    tbo->drawing = tbo_scrolled_window_get_child (dw_scroll);
    tbo->status = status;
    tbo->vbox = vbox;
    tbo->comic = comic;
    tbo->toolarea = toolarea;
    tbo->notebook = notebook;

    tbo->undo_stack = tbo_undo_stack_new ();
    tbo->path = NULL;
    tbo->browse_path = NULL;
    tbo->export_path = NULL;
    tbo->dirty = FALSE;
    tbo->destroying = FALSE;

    return tbo;
}

void 
tbo_window_free (TboWindow *tbo)
{
    tbo_comic_free (tbo->comic);
    if (tbo->toolbar)
        g_object_unref (tbo->toolbar);
    g_free (tbo->path);
    g_free (tbo->browse_path);
    g_free (tbo->export_path);
    tbo_undo_stack_del (tbo->undo_stack);
    free (tbo);
}

void
tbo_window_set_path (TboWindow *tbo, const gchar *path)
{
    set_window_path (&tbo->path, path);
    tbo_window_set_browse_path (tbo, path);
}

void
tbo_window_set_browse_path (TboWindow *tbo, const gchar *path)
{
    set_window_path (&tbo->browse_path, path);
    if (path != NULL)
        store_persisted_path ("browse_path", path);
}

void
tbo_window_set_export_path (TboWindow *tbo, const gchar *path)
{
    set_window_path (&tbo->export_path, path);
    if (path != NULL)
        store_persisted_path ("export_path", path);
}

gchar *
tbo_window_get_open_dir (TboWindow *tbo)
{
    if (tbo->browse_path == NULL)
        tbo->browse_path = load_persisted_path ("browse_path");

    if (tbo->browse_path != NULL)
        return get_dirname_or_home (tbo->browse_path);

    return get_dirname_or_home (tbo->path);
}

gchar *
tbo_window_get_export_dir (TboWindow *tbo)
{
    if (tbo->export_path == NULL)
        tbo->export_path = load_persisted_path ("export_path");

    if (tbo->export_path != NULL)
        return g_path_get_dirname (tbo->export_path);

    return tbo_window_get_open_dir (tbo);
}

void
tbo_window_mark_dirty (TboWindow *tbo)
{
    tbo->dirty = TRUE;
}

void
tbo_window_mark_clean (TboWindow *tbo)
{
    tbo->dirty = FALSE;
}

gboolean
tbo_window_has_unsaved_changes (TboWindow *tbo)
{
    return tbo->dirty;
}

void
tbo_window_add_page_widget (TboWindow *tbo, GtkWidget *page)
{
    gint count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (tbo->notebook));
    gtk_notebook_append_page (GTK_NOTEBOOK (tbo->notebook), page, create_page_tab_label (count));
}

void
tbo_window_remove_page_widget (TboWindow *tbo, gint nth)
{
    GtkWidget *page = get_page_widget (tbo, nth);

    if (page != NULL)
    {
        gtk_notebook_remove_page (GTK_NOTEBOOK (tbo->notebook), nth);
        refresh_page_tab_labels (tbo);
    }
}

gint
tbo_window_get_page_count (TboWindow *tbo)
{
    return gtk_notebook_get_n_pages (GTK_NOTEBOOK (tbo->notebook));
}

gboolean 
tbo_window_free_cb (GtkWidget *widget, GdkEvent *event,
                    TboWindow *tbo)
{
    return !confirm_close (tbo);
}

gboolean
tbo_window_close_request_cb (GtkWindow *window, TboWindow *tbo)
{
    if (confirm_close (tbo))
    {
        tbo->destroying = TRUE;
        return FALSE;
    }

    return TRUE;
}


GtkWidget *
create_darea (TboWindow *tbo)
{
    GtkEventController *key;
    GtkEventController *motion;
    GtkWidget *scrolled;
    GtkWidget *darea;

    scrolled = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    darea = tbo_drawing_new_with_params (tbo->comic);
    tbo_scrolled_window_set_child (scrolled, darea);
    tbo_drawing_init_dnd (TBO_DRAWING (darea), tbo);

    motion = gtk_event_controller_motion_new ();
    g_signal_connect (motion, "motion", G_CALLBACK (on_move_cb), tbo);
    gtk_widget_add_controller (darea, motion);

    key = gtk_event_controller_key_new ();
    g_signal_connect (key, "key-pressed", G_CALLBACK (on_key_cb), tbo);
    gtk_widget_add_controller (darea, key);
    tbo_widget_show_all (scrolled);

    return scrolled;
}

TboWindow *
tbo_new_tbo (GtkApplication *app, int width, int height)
{
    const int sidebar_width = 300;
    const int window_width = MAX (width + sidebar_width + 80, 1100);
    const int window_height = MAX (height + 180, 720);
    TboWindow *tbo;
    Comic *comic;
    GtkWidget *window;
    GtkWidget *container;
    GtkWidget *tool_paned;
    GtkWidget *menu;
    GtkWidget *headerbar;
    TboToolbar *toolbar;
    GtkWidget *scrolled;
    GtkWidget *scrolled2;
    GtkWidget *darea;
    GtkWidget *status;
    GtkWidget *hpaned;
    GtkWidget *notebook;
    GtkEventController *global_key;

    window = app != NULL ? gtk_application_window_new (app) : gtk_window_new ();
    gtk_window_set_default_size (GTK_WINDOW (window), window_width, window_height);
    gchar *icon_path = tbo_get_data_path ("icon.png");
    g_free (icon_path);

    apply_theme_preferences ();
    load_app_css ();

    headerbar = gtk_header_bar_new ();
    gtk_header_bar_set_show_title_buttons (GTK_HEADER_BAR (headerbar), TRUE);
    gtk_window_set_titlebar (GTK_WINDOW (window), headerbar);
    gtk_widget_add_css_class (window, "dark");

    container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class (container, "dark");
    tbo_widget_add_child (window, container);

    comic = tbo_comic_new (_("Untitled"), width, height);
    gtk_window_set_title (GTK_WINDOW (window), comic->title);
    scrolled = gtk_scrolled_window_new ();
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    darea = tbo_drawing_new_with_params (comic);
    tbo_scrolled_window_set_child (scrolled, darea);
    notebook = gtk_notebook_new ();
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
    gtk_widget_set_hexpand (notebook, TRUE);
    gtk_widget_set_vexpand (notebook, TRUE);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrolled, create_page_tab_label (0));

    hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand (hpaned, TRUE);
    gtk_widget_set_vexpand (hpaned, TRUE);
    tool_paned = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name (tool_paned, "tbo-toolarea");
    scrolled2 = gtk_scrolled_window_new ();
    gtk_widget_set_name (scrolled2, "tbo-sidebar");
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled2), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    tbo_scrolled_window_set_child (scrolled2, tool_paned);
    gtk_widget_set_size_request (scrolled2, sidebar_width, -1);
    gtk_widget_set_vexpand (scrolled2, TRUE);

    gtk_paned_set_position (GTK_PANED (hpaned), window_width - sidebar_width);
    tbo_paned_pack_start (hpaned, notebook, TRUE, FALSE);
    tbo_paned_pack_end (hpaned, scrolled2, FALSE, FALSE);

    status = gtk_label_new (NULL);
    gtk_widget_set_name (status, "tbo-status");
    gtk_label_set_xalign (GTK_LABEL (status), 0.0);
    gtk_label_set_ellipsize (GTK_LABEL (status), PANGO_ELLIPSIZE_END);

    tbo = tbo_window_new (window, scrolled, scrolled2, notebook, tool_paned, status, container, comic);

    // Generando la barra de herramientas de la aplicacion
    toolbar = TBO_TOOLBAR (tbo_toolbar_new_with_params (tbo));
    tbo->toolbar = toolbar;

    // drag & drop
    tbo_drawing_init_dnd (TBO_DRAWING (darea), tbo);

    // key press event
    g_signal_connect (tbo->notebook, "switch-page", G_CALLBACK (notebook_switch_page_cb), tbo);
    global_key = gtk_event_controller_key_new ();
    gtk_event_controller_set_propagation_phase (global_key, GTK_PHASE_CAPTURE);
    g_signal_connect (global_key, "key-pressed", G_CALLBACK (global_key_cb), tbo);
    gtk_widget_add_controller (window, global_key);
    g_signal_connect (window, "close-request", G_CALLBACK (tbo_window_close_request_cb), tbo);
    g_signal_connect (window, "destroy", G_CALLBACK (destroy_cb), tbo);

    menu = generate_menu (tbo);
    gtk_header_bar_pack_end (GTK_HEADER_BAR (headerbar), menu);
    tbo_box_pack_start (container, toolbar->toolbar, FALSE, FALSE, 0);

    tbo_widget_add_child (container, hpaned);

    tbo_box_pack_start (container, status, FALSE, FALSE, 0);

    tbo_widget_show_all (window);
    apply_window_icon (window);
    tbo_toolbar_set_selected_tool (toolbar, TBO_TOOLBAR_SELECTOR);

    tbo_window_update_status (tbo, 0, 0);
    return tbo;
}

void
tbo_window_update_status (TboWindow *tbo, int x, int y)
{
    if (tbo == NULL || tbo->destroying)
        return;

    update_statusbar (tbo, x, y);
    tbo_toolbar_update (tbo->toolbar);
}

void
tbo_empty_tool_area (TboWindow *tbo)
{
    tbo_widget_destroy_all_children (tbo->toolarea);
}

void
tbo_window_set_key_binder (TboWindow *tbo, gboolean keyb)
{
    KEY_BINDER = keyb;
    if (keyb)
        tbo_menu_enable_accel_keys (tbo);
    else
        tbo_menu_disable_accel_keys (tbo);
}

void
tbo_window_set_current_tab_page (TboWindow *tbo, gboolean setit)
{
    int nth;

    nth = tbo_comic_page_index (tbo->comic);
    if (setit)
        gtk_notebook_set_current_page (GTK_NOTEBOOK (tbo->notebook), nth);
    tbo->dw_scroll = get_page_widget (tbo, nth);
    tbo->drawing = tbo_scrolled_window_get_child (tbo->dw_scroll);
    TBO_DRAWING (tbo->drawing)->tool = tbo->toolbar->selected_tool;

    tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_SELECTOR);
    tbo_tool_selector_set_selected (TBO_TOOL_SELECTOR (tbo->toolbar->selected_tool), NULL);
    tbo_tool_selector_set_selected_obj (TBO_TOOL_SELECTOR (tbo->toolbar->selected_tool), NULL);
}

void
tbo_window_enter_frame (TboWindow *tbo, Frame *frame)
{
    TboDrawing *drawing;
    TboToolSelector *selector;

    if (frame == NULL)
        return;

    drawing = TBO_DRAWING (tbo->drawing);
    selector = TBO_TOOL_SELECTOR (tbo->toolbar->tools[TBO_TOOLBAR_SELECTOR]);

    tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_SELECTOR);
    tbo_tool_selector_set_selected (selector, frame);
    tbo_tool_selector_set_selected_obj (selector, NULL);
    tbo_page_set_current_frame (tbo_comic_get_current_page (tbo->comic), frame);
    tbo_drawing_set_current_frame (drawing, frame);
    gtk_widget_grab_focus (tbo->drawing);
    tbo_tooltip_set (NULL, 0, 0, tbo);
    tbo_tooltip_set_center_timeout (_("press Esc to go back"), 3000, tbo);
    tbo_window_update_status (tbo, 0, 0);
    tbo_drawing_adjust_scroll (drawing);
}

void
tbo_window_leave_frame (TboWindow *tbo)
{
    TboDrawing *drawing;
    TboToolSelector *selector;
    Frame *frame;

    drawing = TBO_DRAWING (tbo->drawing);
    frame = tbo_drawing_get_current_frame (drawing);
    if (frame == NULL)
        return;

    selector = TBO_TOOL_SELECTOR (tbo->toolbar->tools[TBO_TOOLBAR_SELECTOR]);
    tbo_toolbar_set_selected_tool (tbo->toolbar, TBO_TOOLBAR_SELECTOR);
    tbo_drawing_set_current_frame (drawing, NULL);
    tbo_tool_selector_set_selected (selector, frame);
    tbo_tool_selector_set_selected_obj (selector, NULL);
    gtk_widget_grab_focus (tbo->drawing);
    tbo_tooltip_set (NULL, 0, 0, tbo);
    tbo_window_update_status (tbo, 0, 0);
    tbo_drawing_adjust_scroll (drawing);
}

gboolean
tbo_window_undo_cb (GtkWidget *widget, TboWindow *tbo) {
    tbo_undo_stack_undo (tbo->undo_stack);

    tbo_window_mark_dirty (tbo);
    tbo_drawing_update (TBO_DRAWING (tbo->drawing));
    tbo_toolbar_update (tbo->toolbar);
    return FALSE;
}

gboolean
tbo_window_redo_cb (GtkWidget *widget, TboWindow *tbo) {
    tbo_undo_stack_redo (tbo->undo_stack);

    tbo_window_mark_dirty (tbo);
    tbo_drawing_update (TBO_DRAWING (tbo->drawing));
    tbo_toolbar_update (tbo->toolbar);
    return FALSE;
}
