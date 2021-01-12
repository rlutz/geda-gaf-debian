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


#ifndef GSCHEM_COMPSELECT_DOCKABLE_H
#define GSCHEM_COMPSELECT_DOCKABLE_H


/*
 * GschemCompselectDockable
 */

#define GSCHEM_TYPE_COMPSELECT_DOCKABLE         (gschem_compselect_dockable_get_type())
#define GSCHEM_COMPSELECT_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_COMPSELECT_DOCKABLE, GschemCompselectDockable))
#define GSCHEM_COMPSELECT_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_COMPSELECT_DOCKABLE, GschemCompselectDockableClass))
#define GSCHEM_IS_COMPSELECT_DOCKABLE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_COMPSELECT_DOCKABLE))
#define GSCHEM_COMPSELECT_DOCKABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_COMPSELECT_DOCKABLE, GschemCompselectDockableClass))

typedef struct _GschemCompselectDockableClass GschemCompselectDockableClass;
typedef struct _GschemCompselectDockable      GschemCompselectDockable;


struct _GschemCompselectDockableClass {
  GschemDockableClass parent_class;
};

struct _GschemCompselectDockable {
  GschemDockable parent_instance;

  GtkWidget   *top, *vbox, *hpaned, *vpaned;
  GtkTreeView *libtreeview, *inusetreeview, *attrtreeview;
  GtkNotebook *viewtabs;
  GschemPreview *preview;
  GtkEntry    *entry_filter;
  GtkButton   *button_clear;
  guint        filter_timeout;
  GtkComboBox *combobox_behaviors;

  GtkWidget *preview_content,               *attribs_content;
  GtkWidget *preview_frame,                 *attribs_frame;
  GtkWidget *preview_box,                   *attribs_box;
  GtkWidget *preview_paned,                 *attribs_paned;
  GtkWidget *preview_expander,              *attribs_expander;
  GdkWindow *preview_expander_event_window, *attribs_expander_event_window;
  gboolean   preview_size_allocated,         attribs_size_allocated;

  CLibSymbol *selected_symbol;
  gchar *selected_filename;
  gboolean is_selected;
};


GType gschem_compselect_dockable_get_type (void);

/* Response IDs for special dialog buttons */
typedef enum {
  COMPSELECT_RESPONSE_PLACE = 1,
  COMPSELECT_RESPONSE_HIDE = 2,
  COMPSELECT_RESPONSE_REFRESH = 3
} CompselectResponseType;

#endif /* GSCHEM_COMPSELECT_DOCKABLE_H */
