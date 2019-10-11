/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2019 gEDA Contributors (see ChangeLog for details)
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
/*!
 * \file gschem_find_text_dockable.c
 *
 * \brief Stores state of a find text operation
 */

#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"

#include "../include/gschem_find_text_dockable.h"


enum
{
  COLUMN_FILENAME,
  COLUMN_STRING,
  COLUMN_OBJECT,
  COLUMN_COUNT
};


typedef void (*NotifyFunc)(void*, void*);


static void
assign_store (GschemFindTextDockable *state, GSList *objects);

static void
class_init (GschemFindTextDockableClass *klass);

static void
clear_store (GschemFindTextDockable *state);

static void
dispose (GObject *object);

static void
finalize (GObject *object);

static GSList*
find_objects_using_pattern (GSList *pages, const char *text);

static GSList*
find_objects_using_regex (GSList *pages, const char *text, GError **error);

static GSList*
find_objects_using_substring (GSList *pages, const char *text);

static GSList*
get_pages (GList *pages, gboolean descend);

static void
get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec);

static GList*
get_subpages (PAGE *page);

static void
instance_init (GschemFindTextDockable *state);

static GtkWidget *
create_widget (GschemDockable *parent);

static void
object_weakref_cb (OBJECT *object, GschemFindTextDockable *state);

static void
remove_object (GschemFindTextDockable *state, OBJECT *object);

static void
select_cb (GtkTreeSelection *selection, GschemFindTextDockable *state);

static void
set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);


static GObjectClass *gschem_find_text_dockable_parent_class = NULL;


/*! \brief find instances of a given string
 *
 *  Finds instances of a given string and displays the result inside this
 *  widget.
 *
 *  \param [in] state
 *  \param [in] pages a list of pages to search
 *  \param [in] type the type of find to perform
 *  \param [in] text the text to find
 *  \param [in] descend decend the page heirarchy
 *  \return the number of objects found
 */
int
gschem_find_text_dockable_find (GschemFindTextDockable *state, GList *pages, int type, const char *text, gboolean descend)
{
  int count;
  GSList *objects = NULL;
  GSList *all_pages;

  all_pages = get_pages (pages, descend);

  switch (type) {
    case FIND_TYPE_SUBSTRING:
      objects = find_objects_using_substring (all_pages, text);
      break;

    case FIND_TYPE_PATTERN:
      objects = find_objects_using_pattern (all_pages, text);
      break;

    case FIND_TYPE_REGEX:
      objects = find_objects_using_regex (all_pages, text, NULL);
      break;

    default:
      break;
  }

  g_slist_free (all_pages);

  assign_store (state, objects);

  count = g_slist_length (objects);
  g_slist_free (objects);

  return count;
}


/*! \brief Get/register GschemFindTextDockable type.
 */
GType
gschem_find_text_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GschemFindTextDockableClass),
      NULL,                                /* base_init */
      NULL,                                /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                /* class_finalize */
      NULL,                                /* class_data */
      sizeof(GschemFindTextDockable),
      0,                                   /* n_preallocs */
      (GInstanceInitFunc) instance_init,
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemFindTextDockable",
                                   &info,
                                   0);
  }

  return type;
}








/*! \brief places object in the store so the user can see them
 *
 *  \param [in] state
 *  \param [in] objects the list of objects to put in the store
 */
static void
assign_store (GschemFindTextDockable *state, GSList *objects)
{
  GSList *object_iter;

  g_return_if_fail (state != NULL);
  g_return_if_fail (state->store != NULL);

  clear_store (state);

  object_iter = objects;

  while (object_iter != NULL) {
    char *basename;
    OBJECT *object = (OBJECT*) object_iter->data;
    const char *str;
    GtkTreeIter tree_iter;

    object_iter = g_slist_next (object_iter);

    if (object == NULL) {
      g_warning ("NULL object encountered");
      continue;
    }

    if (object->page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    if (object->type != OBJ_TEXT) {
      g_warning ("expecting a text object");
      continue;
    }

    str = o_text_get_string (object->page->toplevel, object);

    if (str == NULL) {
      g_warning ("NULL string encountered");
      continue;
    }

    s_object_weak_ref (object, (NotifyFunc) object_weakref_cb, state);

    gtk_list_store_append (state->store, &tree_iter);

    if (object->page->is_untitled)
      basename = g_strdup (_("Untitled page"));
    else
      basename = g_path_get_basename (object->page->page_filename);

    gtk_list_store_set (state->store,
                        &tree_iter,
                        COLUMN_FILENAME, basename,
                        COLUMN_STRING, str,
                        COLUMN_OBJECT, object,
                        -1);

    g_free (basename);
  }
}


/*! \brief initialize class
 *
 *  \param [in] klass The class for initialization
 */
static void
class_init (GschemFindTextDockableClass *klass)
{
  gschem_find_text_dockable_parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

  GSCHEM_DOCKABLE_CLASS (klass)->create_widget = create_widget;

  G_OBJECT_CLASS (klass)->dispose  = dispose;
  G_OBJECT_CLASS (klass)->finalize = finalize;

  G_OBJECT_CLASS (klass)->get_property = get_property;
  G_OBJECT_CLASS (klass)->set_property = set_property;

  g_signal_new ("select-object",                     /* signal_name  */
                G_OBJECT_CLASS_TYPE (klass),         /* itype        */
                0,                                   /* signal_flags */
                0,                                   /* class_offset */
                NULL,                                /* accumulator  */
                NULL,                                /* accu_data    */
                g_cclosure_marshal_VOID__POINTER,    /* c_marshaller */
                G_TYPE_NONE,                         /* return_type  */
                1,                                   /* n_params     */
                G_TYPE_POINTER
                );
}


/*! \brief delete all items from the list store
 *
 *  This function deletes all items in the list store and removes all the weak
 *  references to the objects.
 *
 *  \param [in] state
 */
static void
clear_store (GschemFindTextDockable *state)
{
  GtkTreeIter iter;
  gboolean valid;

  g_return_if_fail (state != NULL);
  g_return_if_fail (state->store != NULL);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (state->store), &iter);

  while (valid) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (state->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *object = g_value_get_pointer (&value);

      s_object_weak_unref (object, (NotifyFunc) object_weakref_cb, state);
    }

    g_value_unset (&value);

    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (state->store), &iter);
  }

  gtk_list_store_clear (state->store);
}


/*! \brief Dispose of the object
 */
static void
dispose (GObject *object)
{
  GschemFindTextDockable *state = GSCHEM_FIND_TEXT_DOCKABLE (object);

  if (state->store) {
    clear_store (state);
    g_object_unref (state->store);
    state->store = NULL;
  }

  /* lastly, chain up to the parent dispose */

  g_return_if_fail (gschem_find_text_dockable_parent_class != NULL);
  gschem_find_text_dockable_parent_class->dispose (object);
}


/*! \brief Finalize object
 */
static void
finalize (GObject *object)
{
  /* lastly, chain up to the parent finalize */

  g_return_if_fail (gschem_find_text_dockable_parent_class != NULL);
  gschem_find_text_dockable_parent_class->finalize (object);
}


/*! \brief Find all text objects that match a pattern
 *
 *  \param pages the list of pages to search
 *  \param text the pattern to match
 *  \return a list of objects that match the given pattern
 */
static GSList*
find_objects_using_pattern (GSList *pages, const char *text)
{
  GSList *object_list = NULL;
  GSList *page_iter = pages;
  GPatternSpec *pattern;

  g_return_val_if_fail (text != NULL, NULL);

  pattern = g_pattern_spec_new (text);

  while (page_iter != NULL) {
    const GList *object_iter;
    PAGE *page = (PAGE*) page_iter->data;

    page_iter = g_slist_next (page_iter);

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    object_iter = s_page_objects (page);

    while (object_iter != NULL) {
      OBJECT *object = (OBJECT*) object_iter->data;
      const char *str;

      object_iter = g_list_next (object_iter);

      if (object == NULL) {
        g_warning ("NULL object encountered");
        continue;
      }

      if (object->type != OBJ_TEXT) {
        continue;
      }

      if (!(o_is_visible (object) || page->toplevel->show_hidden_text)) {
        continue;
      }

      str = o_text_get_string (object->page->toplevel, object);

      if (str == NULL) {
        g_warning ("NULL string encountered");
        continue;
      }

      if (g_pattern_match_string (pattern, str)) {
        object_list = g_slist_prepend (object_list, object);
      }
    }
  }

  g_pattern_spec_free (pattern);

  return g_slist_reverse (object_list);
}


/*! \brief Find all text objects that match a regex
 *
 *  \param pages the list of pages to search
 *  \param text the regex to match
 *  \return a list of objects that match the given regex
 */
static GSList*
find_objects_using_regex (GSList *pages, const char *text, GError **error)
{
  GError *ierror = NULL;
  GSList *object_list = NULL;
  GSList *page_iter = pages;
  GRegex *regex;

  g_return_val_if_fail (text != NULL, NULL);

  regex = g_regex_new (text,
                       0,
                       0,
                       &ierror);

  if (ierror != NULL) {
    g_propagate_error (error, ierror);
    return NULL;
  }

  while (page_iter != NULL) {
    const GList *object_iter;
    PAGE *page = (PAGE*) page_iter->data;

    page_iter = g_slist_next (page_iter);

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    object_iter = s_page_objects (page);

    while (object_iter != NULL) {
      OBJECT *object = (OBJECT*) object_iter->data;
      const char *str;

      object_iter = g_list_next (object_iter);

      if (object == NULL) {
        g_warning ("NULL object encountered");
        continue;
      }

      if (object->type != OBJ_TEXT) {
        continue;
      }

      if (!(o_is_visible (object) || page->toplevel->show_hidden_text)) {
        continue;
      }

      str = o_text_get_string (object->page->toplevel, object);

      if (str == NULL) {
        g_warning ("NULL string encountered");
        continue;
      }

      if (g_regex_match (regex, str, 0, NULL)) {
        object_list = g_slist_prepend (object_list, object);
      }
    }
  }

  g_regex_unref (regex);

  return g_slist_reverse (object_list);
}


/*! \brief Find all text objects that contain a substring
 *
 *  \param pages the list of pages to search
 *  \param text the substring to find
 *  \return a list of objects that contain the given substring
 */
static GSList*
find_objects_using_substring (GSList *pages, const char *text)
{
  GSList *object_list = NULL;
  GSList *page_iter = pages;

  g_return_val_if_fail (text != NULL, NULL);

  while (page_iter != NULL) {
    const GList *object_iter;
    PAGE *page = (PAGE*) page_iter->data;

    page_iter = g_slist_next (page_iter);

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    object_iter = s_page_objects (page);

    while (object_iter != NULL) {
      OBJECT *object = (OBJECT*) object_iter->data;
      const char *str;

      object_iter = g_list_next (object_iter);

      if (object == NULL) {
        g_warning ("NULL object encountered");
        continue;
      }

      if (object->type != OBJ_TEXT) {
        continue;
      }

      if (!(o_is_visible (object) || page->toplevel->show_hidden_text)) {
        continue;
      }

      str = o_text_get_string (object->page->toplevel, object);

      if (str == NULL) {
        g_warning ("NULL string encountered");
        continue;
      }

      if (strstr (str, text) != NULL) {
        object_list = g_slist_prepend (object_list, object);
      }
    }
  }

  return g_slist_reverse (object_list);
}



/*! \brief obtain a list of pages for an operation
 *
 *  Descends the heirarchy of pages, if selected, and removes duplicate pages.
 *
 *  \param [in] pages the list of pages to begin search
 *  \param [in] descend alose locates subpages
 *  \return a list of all the pages
 */
static GSList*
get_pages (GList *pages, gboolean descend)
{
  GList *input_list = g_list_copy (pages);
  GSList *output_list = NULL;
  GHashTable *visit_list = g_hash_table_new (NULL, NULL);

  while (input_list != NULL) {
    PAGE *page = (PAGE*) input_list->data;

    input_list = g_list_delete_link (input_list, input_list);

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    /** \todo the following function becomes available in glib 2.32 */
    /* if (g_hash_table_contains (visit_list, page)) { */

    if (g_hash_table_lookup_extended (visit_list, page, NULL, NULL)) {
      continue;
    }

    output_list = g_slist_prepend (output_list, page);
    g_hash_table_insert (visit_list, page, NULL);

    if (descend) {
      input_list = g_list_concat (input_list, get_subpages (page));
    }
  }

  g_hash_table_destroy (visit_list);

  return g_slist_reverse (output_list);
}


/*! \brief Get a property
 *
 *  \param [in]     object
 *  \param [in]     param_id
 *  \param [in,out] value
 *  \param [in]     pspec
 */
static void
get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
  /* GschemFindTextDockable *state = GSCHEM_FIND_TEXT_DOCKABLE (object); */

  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
  }
}


/*! \brief get the subpages of a schematic page
 *
 *  if any subpages are not loaded, this function will load them.
 *
 *  \param [in] page the parent page
 *  \return a list of all the subpages
 */
static GList*
get_subpages (PAGE *page)
{
  const GList *object_iter;
  GList *page_list = NULL;

  g_return_val_if_fail (page != NULL, NULL);

  object_iter = s_page_objects (page);

  while (object_iter != NULL) {
    char *attrib;
    char **filenames;
    char **iter;
    OBJECT *object = (OBJECT*) object_iter->data;

    object_iter = g_list_next (object_iter);

    if (object == NULL) {
      g_warning ("NULL object encountered");
      continue;
    }

    if (object->type != OBJ_COMPLEX) {
      continue;
    }

    attrib = o_attrib_search_attached_attribs_by_name (object,
                                                       "source",
                                                       0);

    if (attrib == NULL) {
      attrib = o_attrib_search_inherited_attribs_by_name (object,
                                                          "source",
                                                          0);
    }

    if (attrib == NULL) {
      continue;
    }

    filenames = g_strsplit (attrib, ",", 0);

    if (filenames == NULL) {
      continue;
    }

    for (iter = filenames; *iter != NULL; iter++) {
      PAGE *subpage = s_hierarchy_load_subpage (page, *iter, NULL);

      if (subpage != NULL) {
        page_list = g_list_prepend (page_list, subpage);
      }
    }

    g_strfreev (filenames);
  }

  return g_list_reverse (page_list);
}


/*! \brief initialize a new instance
 *
 *  \param [in] state the new instance
 */
static void
instance_init (GschemFindTextDockable *state)
{
  state->store = gtk_list_store_new(COLUMN_COUNT,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_POINTER);
}

static GtkWidget *
create_widget (GschemDockable *parent)
{
  GschemFindTextDockable *state = GSCHEM_FIND_TEXT_DOCKABLE (parent);

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled;
  GtkTreeSelection *selection;
  GtkWidget *tree_widget;

  scrolled = gtk_scrolled_window_new (NULL, NULL);

  tree_widget = gtk_tree_view_new_with_model (GTK_TREE_MODEL (state->store));
  gtk_container_add (GTK_CONTAINER (scrolled), tree_widget);

  /* filename column */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Filename"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer, "text", 0);

  /* text column */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Text"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer, "text", 1);

  /* attach signal to detect user selection */

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_widget));
  g_signal_connect (selection, "changed", G_CALLBACK (select_cb), state);

  gtk_widget_show_all (scrolled);
  return scrolled;
}


/*! \brief callback for an object that has been destroyed
 *
 *  \param [in] object the object that has been destroyed
 *  \param [in] state
 */
static void
object_weakref_cb (OBJECT *object, GschemFindTextDockable *state)
{
  g_return_if_fail (state != NULL);

  remove_object (state, object);
}


/*! \brief remove an object from the store
 *
 *  This function gets called in response to the object deletion. And, doesn't
 *  dereference the object.
 *
 *  This function doesn't remove the weak reference, under the assumption that
 *  the object is being destroyed.
 *
 *  \param [in] state
 *  \param [in] object the object to remove from the store
 */
static void
remove_object (GschemFindTextDockable *state, OBJECT *object)
{
  GtkTreeIter iter;
  gboolean valid;

  g_return_if_fail (object != NULL);
  g_return_if_fail (state != NULL);
  g_return_if_fail (state->store != NULL);

  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (state->store), &iter);

  while (valid) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (state->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *other = g_value_get_pointer (&value);

      if (object == other) {
        g_value_unset (&value);
        valid = gtk_list_store_remove (state->store, &iter);
        continue;
      }
    }

    g_value_unset (&value);
    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (state->store), &iter);
  }
}


/*! \brief callback for user selecting an item
 *
 *  \param [in] selection
 *  \param [in] state
 */
static void
select_cb (GtkTreeSelection *selection, GschemFindTextDockable *state)
{
  GtkTreeIter iter;
  gboolean success;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (state != NULL);

  success = gtk_tree_selection_get_selected (selection, NULL, &iter);

  if (success) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (state->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *object = g_value_get_pointer (&value);

      if (object != NULL) {
        g_signal_emit_by_name (state, "select-object", object);
      } else {
        g_warning ("NULL object encountered");
      }
    }

    g_value_unset (&value);
  }
}


/*! \brief Set a gobject property
 *
 *  \param [in]     object
 *  \param [in]     param_id
 *  \param [in,out] value
 *  \param [in]     pspec
 */
static void
set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
  /* GschemFindTextDockable *state = GSCHEM_FIND_TEXT_DOCKABLE (object); */

  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
  }
}
