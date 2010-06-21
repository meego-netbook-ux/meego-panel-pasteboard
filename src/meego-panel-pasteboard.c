/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <string.h>

#include <glib/gi18n.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>
#include <meego-panel/mpl-entry.h>

#include "mnb-clipboard-store.h"
#include "mnb-clipboard-view.h"

#define WIDGET_SPACING 5
#define ICON_SIZE 48
#define PADDING 8
#define N_COLS 4
#define LAUNCHER_WIDTH  235
#define LAUNCHER_HEIGHT 64

typedef struct _SearchClosure   SearchClosure;

static guint search_timeout_id = 0;
static MnbClipboardStore *store = NULL;

static void
on_dropdown_show (MplPanelClient  *client,
                  ClutterActor    *filter_entry)
{
  /* give focus to the actor */
  clutter_actor_grab_key_focus (filter_entry);
}

static void
on_dropdown_hide (MplPanelClient  *client,
                  ClutterActor    *filter_entry)
{
  /* Reset search. */
  mpl_entry_set_text (MPL_ENTRY (filter_entry), "");
}

struct _SearchClosure
{
  MnbClipboardView *view;

  gchar *filter;
};

static gboolean
search_timeout (gpointer data)
{
  SearchClosure *closure = data;

  g_debug ("%s: filter = '%s'", G_STRLOC, closure->filter);
  mnb_clipboard_view_filter (closure->view, closure->filter);

  return FALSE;
}

static void
search_cleanup (gpointer data)
{
  SearchClosure *closure = data;

  search_timeout_id = 0;

  g_object_unref (closure->view);
  g_free (closure->filter);

  g_slice_free (SearchClosure, closure);
}

static void
on_search_activated (MplEntry *entry,
                     gpointer  data)
{
  MnbClipboardView *view = data;
  SearchClosure *closure;
  const gchar *text;

  if (search_timeout_id != 0)
    {
      g_source_remove (search_timeout_id);
      search_timeout_id = 0;
    }

  text = mpl_entry_get_text (entry);

  closure = g_slice_new (SearchClosure);
  closure->view = g_object_ref (view);
  closure->filter = (text == NULL || *text == '\0') ? NULL : g_strdup (text);

  search_timeout_id = g_timeout_add_full (CLUTTER_PRIORITY_REDRAW - 5, 250,
                                          search_timeout,
                                          closure, search_cleanup);
}

static void
on_clear_clicked (MxButton *button,
                  gpointer    dummy G_GNUC_UNUSED)
{
  mnb_clipboard_store_clear (MNB_CLIPBOARD_STORE (store));
}

static void
on_selection_changed (MnbClipboardStore *store,
                      const gchar       *current_selection,
                      MxLabel         *label)
{
  if (current_selection == NULL || *current_selection == '\0')
    mx_label_set_text (label, _("the current selection to pasteboard"));
  else
    {
      gchar *text;

      text = g_strconcat (_("your selection"),
                          " \"", current_selection, "\"",
                          NULL);
      mx_label_set_text (label, text);

      g_free (text);
    }
}

static void
on_selection_copy_clicked (MxButton *button,
                           gpointer    dummy G_GNUC_UNUSED)
{
  mnb_clipboard_store_save_selection (store);
}

static void on_item_added (MnbClipboardStore    *store,
                           MnbClipboardItemType  item_type,
                           ClutterActor         *bin);

static void
on_item_added (MnbClipboardStore    *store,
               MnbClipboardItemType  item_type,
               ClutterActor         *bin)
{
  g_signal_handlers_disconnect_by_func (store,
                                        G_CALLBACK (on_item_added),
                                        bin);

  clutter_actor_destroy (bin);
}

static ClutterActor *
make_pasteboard (gint           width,
                 ClutterActor **entry_out)
{
  ClutterActor *vbox, *hbox, *label, *entry, *bin, *button;
  ClutterActor *view, *scroll;
  ClutterText *text;

  /* the object proxying the Clipboard changes and storing them */
  store = mnb_clipboard_store_new ();

  vbox = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (vbox), 12);
  mx_table_set_row_spacing (MX_TABLE (vbox), 6);
  clutter_actor_set_name (CLUTTER_ACTOR (vbox), "pasteboard-vbox");

  /* Filter row. */
  hbox = mx_table_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (hbox), "pasteboard-search");
  mx_table_set_column_spacing (MX_TABLE (hbox), 20);
  mx_table_add_actor_with_properties (MX_TABLE (vbox),
                                        CLUTTER_ACTOR (hbox),
                                        0, 0,
                                        "row-span", 1,
                                        "column-span", 2,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);

  label = mx_label_new_with_text (_("Pasteboard"));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "pasteboard-search-label");
  mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        NULL);

  entry = (ClutterActor *)mpl_entry_new (_("Search"));
  clutter_actor_set_name (CLUTTER_ACTOR (entry), "pasteboard-search-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (entry), 600);
  mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                        CLUTTER_ACTOR (entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        NULL);
  if (entry_out)
    *entry_out = CLUTTER_ACTOR (entry);

  /* bin for the the "pasteboard is empty" notice */
  bin = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (bin), "pasteboard-empty-bin");
  mx_bin_set_alignment (MX_BIN (bin),
                          MX_ALIGN_START,
                          MX_ALIGN_MIDDLE);
  mx_bin_set_fill (MX_BIN (bin), TRUE, FALSE);
  mx_table_add_actor_with_properties (MX_TABLE (vbox), CLUTTER_ACTOR (bin),
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        "row-span", 1,
                                        "column-span", 2,
                                        NULL);

  label = mx_label_new_with_text (_("You need to copy some text to use Pasteboard"));
  mx_stylable_set_style_class (MX_STYLABLE (label), "pasteboard-empty-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (bin),
                               CLUTTER_ACTOR (label));

  g_signal_connect (store, "item-added",
                    G_CALLBACK (on_item_added),
                    bin);

  /* the actual view */
  view = CLUTTER_ACTOR (mnb_clipboard_view_new (store));

  /* the scroll view is bigger to avoid the horizontal scroll bar */
  scroll = CLUTTER_ACTOR (mx_scroll_view_new ());
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll), view);

  bin = mx_frame_new ();
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-items-list");
  g_object_set (bin, "y-fill", TRUE, "x-fill", TRUE, NULL);
  clutter_container_add_actor (CLUTTER_CONTAINER (bin), scroll);
  mx_table_add_actor_with_properties (MX_TABLE (vbox),
                                        CLUTTER_ACTOR (bin),
                                        2, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);

  /* hook up the search entry to the view */
  g_signal_connect (entry, "button-clicked",
                    G_CALLBACK (on_search_activated), view);
  g_signal_connect (entry, "text-changed",
                    G_CALLBACK (on_search_activated), view);


  /* side controls */
  bin = mx_table_new ();
  mx_table_set_row_spacing (MX_TABLE (bin), 12);
  clutter_actor_set_width (CLUTTER_ACTOR (bin), 300);
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "pasteboard-controls");
  mx_table_add_actor_with_properties (MX_TABLE (vbox),
                                        CLUTTER_ACTOR (bin),
                                        2, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);

  button = mx_button_new_with_label (_("Clear pasteboard"));
  mx_table_add_actor_with_properties (MX_TABLE (bin),
                                        CLUTTER_ACTOR (button),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_clear_clicked),
                    NULL);

  hbox = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (hbox), 6);
  mx_table_add_actor_with_properties (MX_TABLE (bin),
                                        CLUTTER_ACTOR (hbox),
                                        1, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);

  button = mx_button_new_with_label (_("Copy"));
  mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                        CLUTTER_ACTOR (button),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_START,
                                        NULL);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_selection_copy_clicked),
                    store);

  label = mx_label_new_with_text (_("the current selection to pasteboard"));
  text = CLUTTER_TEXT (mx_label_get_clutter_text (MX_LABEL (label)));
  clutter_text_set_single_line_mode (text, FALSE);
  clutter_text_set_line_wrap (text, TRUE);
  clutter_text_set_line_wrap_mode (text, PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (text, PANGO_ELLIPSIZE_END);
  mx_table_add_actor_with_properties (MX_TABLE (hbox),
                                        CLUTTER_ACTOR (label),
                                        0, 1,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", TRUE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        NULL);

  g_signal_connect (store, "selection-changed",
                    G_CALLBACK (on_selection_changed),
                    label);

  return CLUTTER_ACTOR (vbox);
}

static void
_client_set_size_cb (MplPanelClient *client,
                     guint           width,
                     guint           height,
                     gpointer        userdata)
{
  clutter_actor_set_size ((ClutterActor *)userdata,
                          width,
                          height);

  g_debug (G_STRLOC ": Dimensions for grid view: %d x %d",
           width,
           height);
}

static gboolean standalone = FALSE;

static GOptionEntry entries[] = {
  {
    "standalone", 's',
    0,
    G_OPTION_ARG_NONE, &standalone,
    "Do not embed into the mutter-meego panel", NULL
  },

  { NULL }
};

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;
  ClutterActor *stage, *pasteboard;
  GOptionContext *context;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- mutter-meego pasteboard panel");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  g_option_context_add_group (context, gtk_get_option_group (FALSE));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical (G_STRLOC ": Error parsing option: %s", error->message);
      g_clear_error (&error);
    }

  g_option_context_free (context);

  mpl_panel_clutter_init_with_gtk (&argc, &argv);

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
                                 MX_CACHE);
  mx_style_load_from_file (mx_style_get_default (),
                             THEMEDIR "/panel.css", NULL);

  if (!standalone)
    {
      ClutterActor *entry = NULL;
      client = mpl_panel_clutter_new ("pasteboard",
                                      _("pasteboard"),
                                      NULL,
                                      "pasteboard-button",
                                      TRUE);

      mpl_panel_clutter_setup_events_with_gtk (client);

      mpl_panel_client_set_height_request (client, 400);

      stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (client));

      pasteboard = make_pasteboard (800, &entry);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), pasteboard);
      g_signal_connect (client,
                        "set-size", G_CALLBACK (_client_set_size_cb),
                        pasteboard);
      g_signal_connect (client,
                        "show-begin", G_CALLBACK (on_dropdown_show),
                        entry);
      g_signal_connect (client,
                        "hide-end", G_CALLBACK (on_dropdown_hide),
                        entry);
    }
  else
    {
      Window xwin;

      stage = clutter_stage_get_default ();
      clutter_actor_realize (stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      mpl_panel_clutter_setup_events_with_gtk_for_xid (xwin);

      pasteboard = make_pasteboard (800, NULL);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), pasteboard);
      clutter_actor_set_size (pasteboard, 1016, 504);
      clutter_actor_set_size (stage, 1016, 504);
      clutter_actor_show_all (stage);
    }

  clutter_main ();

  return 0;
}

