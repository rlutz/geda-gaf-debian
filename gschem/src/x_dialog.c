/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2020 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
/*! \todo STILL NEED to clean up line lengths in aa and tr */
#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"


/*********** Start of misc support functions for dialog boxes *******/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_dialog_raise_all(GschemToplevel *w_current)
{
  if(w_current->sowindow) {
    gdk_window_raise(w_current->sowindow->window);
  }
  if(w_current->tiwindow) {
    gdk_window_raise(w_current->tiwindow->window);
  }
  if(w_current->sewindow) {
    gdk_window_raise(w_current->sewindow->window);
  }
  if(w_current->aawindow) {
    gdk_window_raise(w_current->aawindow->window);
  }
  if(w_current->aewindow) {
    gdk_window_raise(w_current->aewindow->window);
  }
  if(w_current->hkwindow) {
    gdk_window_raise(w_current->hkwindow->window);
  }
  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    GtkWidget *window = GSCHEM_DOCKABLE (l->data)->window;
    if (window != NULL && gtk_widget_get_visible (window))
      gdk_window_raise (window->window);
  }
}

/*********** End of misc support functions for dialog boxes *******/

/***************** Start of generic message dialog box *******************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void generic_msg_dialog (const char *msg)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   "%s", msg);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

}

/***************** End of generic message dialog box *********************/

/***************** Start of generic confirm dialog box *******************/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int generic_confirm_dialog (const char *msg)
{
  GtkWidget *dialog;
  gint r;

  dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK_CANCEL,
                                   "%s", msg);

  r = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (r ==  GTK_RESPONSE_OK)
    return 1;
  else
    return 0;
}

/***************** End of generic confirm dialog box *********************/

/***************** Start of generic file select dialog box ***************/
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \warning
 *   Caller must g_free returned character string.
 */
char *generic_filesel_dialog (const char *msg, const char *templ, gint flags)
{
  GtkWidget *dialog;
  gchar *result = NULL;
  char *title;

  /* Default to load if not specified.  Maybe this should cause an error. */
  if (! (flags & (FSB_LOAD | FSB_SAVE))) {
    flags = flags | FSB_LOAD;
  }

  if (flags & FSB_LOAD) {
    title = g_strdup_printf("%s: Open", msg);
    dialog = gtk_file_chooser_dialog_new (title,
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                          NULL);
    /* Since this is a load dialog box, the file must exist! */
    flags = flags | FSB_MUST_EXIST;

  } else {
    title = g_strdup_printf("%s: Save", msg);
    dialog = gtk_file_chooser_dialog_new (title,
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_SAVE,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_OK,
                                          NULL);
  }

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK,
                                          GTK_RESPONSE_CANCEL,
                                          -1);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  /* Pick the current template (*.rc) or default file name */
  if (templ && *templ) {
    if (flags & FSB_SAVE)  {
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), templ);
    } else {
      gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (dialog), templ);
    }
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
    result = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  }
  gtk_widget_destroy (dialog);

  g_free (title);

  return result;

}

/***************** End of generic file select dialog box *****************/

/*********** Start of some Gtk utils  *******/

/*! \brief Selects all text in a TextView widget
 *  \par Function Description
 *  The function selects all the text in a TextView widget.
 */
void select_all_text_in_textview(GtkTextView *textview)
{
  GtkTextBuffer *textbuffer;
  GtkTextIter start, end;

  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  gtk_text_buffer_get_bounds (textbuffer, &start, &end);
  gtk_text_buffer_select_range(textbuffer, &start, &end);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int text_view_calculate_real_tab_width(GtkTextView *textview, int tab_size)
{
  PangoLayout *layout;
  gchar *tab_string;
  gint tab_width = 0;

  if (tab_size == 0)
  return -1;

  tab_string = g_strnfill (tab_size, ' ');

  layout = gtk_widget_create_pango_layout (
                                           GTK_WIDGET (textview),
                                           tab_string);
  g_free (tab_string);

  if (layout != NULL) {
    pango_layout_get_pixel_size (layout, &tab_width, NULL);
    g_object_unref (G_OBJECT (layout));
  } else
  tab_width = -1;

  return tab_width;

}

/*********** End of some Gtk utils *******/

/*********** Start of major symbol changed dialog box *******/

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
major_changed_dialog (GschemToplevel* w_current)
{
  GtkListStore *list_store = NULL;
  GtkWidget *dialog = NULL;
  GtkWidget *content_area, *hbox, *vbox, *tree_view, *scroll;
  GtkWidget *image, *label;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  char* tmp;
  GList *curr;

  if (w_current->toplevel->major_changed_refdes == NULL) return;

  /* If the page was opened in the background, raise the main window
     to the front so the user sees what this dialog refers to. */
  x_window_present (w_current);

  list_store = gtk_list_store_new (1, G_TYPE_STRING);

  for (curr = w_current->toplevel->major_changed_refdes;
       curr != NULL;
       curr = g_list_next (curr)) {
    char *value = (char *) curr->data;
    GtkTreeIter iter;

    gtk_list_store_append (list_store, &iter);
    gtk_list_store_set (list_store, &iter,
                        0, value,
                        -1);
  }

  g_list_free_full (w_current->toplevel->major_changed_refdes, g_free);
  w_current->toplevel->major_changed_refdes = NULL;

  /*! \todo this would be much easier using
   * gtk_message_dialog_get_message_area(). */
  dialog = g_object_new (GTK_TYPE_DIALOG,
                         /* GtkWindow */
                         "default-width",  400,
                         "default-height", 300,
                         /* GtkContainer */
                         "border-width", 5,
                         NULL);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  /* This box contains the warning image and the vbox */
  hbox = g_object_new (GTK_TYPE_HBOX,
                       /* GtkContainer */
                       "border-width", 5,
                       /* GtkBox */
                       "homogeneous", FALSE,
                       "spacing", 12,
                       NULL);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);
  /* Warning image */
  image = g_object_new (GTK_TYPE_IMAGE,
                        /* GtkMisc */
                        "xalign", 0.5,
                        "yalign", 0.0,
                        /* GtkImage */
                        "stock", GTK_STOCK_DIALOG_WARNING,
                        "icon-size", GTK_ICON_SIZE_DIALOG,
                        NULL);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  /* This box contains the labels and list of changed symbols */
  vbox = g_object_new (GTK_TYPE_VBOX,
                       /* GtkBox */
                       "homogeneous", FALSE,
                       "spacing", 12,
                       NULL);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  /* Primary label */
  tmp = g_strconcat ("<big><b>",
                     _("Major symbol changes detected."),
                     "</b></big>", NULL);
  label = g_object_new (GTK_TYPE_LABEL,
                        /* GtkMisc */
                        "xalign", 0.0,
                        "yalign", 0.0,
                        "selectable", TRUE,
                        /* GtkLabel */
                        "wrap", TRUE,
                        "use-markup", TRUE,
                        "label", tmp,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  g_free (tmp);
  /* Secondary label */
  label = g_object_new (GTK_TYPE_LABEL,
                        /* GtkMisc */
                        "xalign", 0.0,
                        "yalign", 0.0,
                        "selectable", TRUE,
                        /* GtkLabel */
                        "wrap", TRUE,
                        "use-markup", TRUE,
                        "label",
                        _("Changes have occurred to the symbols shown below.\n\n"
                          "Be sure to verify each of these symbols."),
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  /* List of changed symbols */
  scroll = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                         /* GtkScrolledWindow */
                         "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                         "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                         "shadow-type",       GTK_SHADOW_IN,
                         NULL);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  tree_view = g_object_new (GTK_TYPE_TREE_VIEW,
                            /* GtkTreeView */
                            "enable-search", FALSE,
                            "headers-visible", FALSE,
                            "model", list_store,
                            NULL);
  gtk_container_add (GTK_CONTAINER (scroll), tree_view);
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Symbol"),
                                                     renderer,
                                                     "text", 0,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  gtk_widget_show_all (dialog);

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (w_current->main_window));

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/*********** End of major symbol changed dialog box *******/


/***************** Start of misc helper dialog boxes **************/
/*! \brief Validate the input attribute
 *  \par Function Description
 *  This function validates the attribute and if it isn't valid
 *  pops up an error message box.
 *
 *  \param parent The parent window which spawned this dialog box.
 *  \param attribute The attribute to be validated.
 *  \returns TRUE if the attribute is valid, FALSE otherwise.
 */
int x_dialog_validate_attribute(GtkWindow* parent, char *attribute)
{
  GtkWidget* message_box;

  /* validate the new attribute */
  if (!o_attrib_string_get_name_value (attribute, NULL, NULL)) {
      message_box = gtk_message_dialog_new_with_markup (parent,
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,
                                  _("<span weight=\"bold\" size=\"larger\">The input attribute \"%s\" is invalid\nPlease correct in order to continue</span>\n\nThe name and value must be non-empty.\nThe name cannot end with a space.\nThe value cannot start with a space."),
                                  attribute);
     gtk_window_set_title(GTK_WINDOW(message_box), _("Invalid Attribute"));
     gtk_dialog_run (GTK_DIALOG (message_box));
     gtk_widget_destroy (message_box);
     return FALSE;
  }
  return TRUE;
}
/***************** End of misc helper dialog boxes **************/


/*! \brief Ask the user for confirmation for creating a file.
 *
 * Shows a message dialog and lets the user decide whether to create a
 * file with the given name or not.
 *
 * \param [in] parent    the transient parent window
 * \param [in] message   the message to show, where '%s' is replaced
 *                       with the filename
 * \param [in] filename  the name of the file which would be created
 *
 * \returns \c TRUE if the user confirmed creating the file,
 *          \c FALSE otherwise
 */
gboolean
x_dialog_confirm_create (GtkWindow *parent, const gchar *message,
                                            const gchar *filename)
{
  GtkWidget *dialog, *button;
  gint response_id;

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE,
                                   message, filename);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          NULL);
  button = gtk_button_new_with_mnemonic (_("_Create"));
  gtk_widget_set_can_default (button, TRUE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_NEW,
                                                  GTK_ICON_SIZE_BUTTON));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
                                GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
  response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response_id == GTK_RESPONSE_ACCEPT;
}
