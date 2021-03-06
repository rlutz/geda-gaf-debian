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
#include <config.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>

#include "gschem.h"

SCM_SYMBOL (at_sym, "@");
SCM_SYMBOL (gschem_sym, "gschem");
SCM_SYMBOL (core_sym, "core");
SCM_SYMBOL (hook_sym, "hook");
SCM_SYMBOL (run_hook_sym, "run-hook");
SCM_SYMBOL (list_sym, "list");

/*! \brief Gets a Scheme hook object by name.
 * \par Function Description
 * Returns the contents of variable with the given name in the (gschem
 * core hook).  Used for looking up hook objects.
 *
 * \param name name of hook to lookup.
 * \return value found in the (gschem core hook) module.
 */
static SCM
g_get_hook_by_name (const char *name)
{
  SCM exp = scm_list_3 (at_sym,
                        scm_list_3 (gschem_sym, core_sym, hook_sym),
                        scm_from_utf8_symbol (name));
  return g_scm_eval_protected (exp, SCM_UNDEFINED);
}

/*! \brief Runs a object hook for a list of objects.
 * \par Function Description
 * Runs a hook called \a name, which should expect a list of #OBJECT
 * smobs as its argument, with \a obj_lst as the argument list.
 *
 * \see g_run_hook_object()
 *
 * \param name    name of hook to run.
 * \param obj_lst list of #OBJECT smobs as hook argument.
 */
void
g_run_hook_object_list (GschemToplevel *w_current, const char *name,
                        GList *obj_lst)
{
  SCM lst = SCM_EOL;
  GList *iter;

  scm_dynwind_begin (0);
  g_dynwind_window (w_current);

  for (iter = obj_lst; iter != NULL; iter = g_list_next (iter)) {
    lst = scm_cons (edascm_from_object ((OBJECT *) iter->data), lst);
  }
  SCM expr = scm_list_3 (run_hook_sym,
                         g_get_hook_by_name (name),
                         scm_cons (list_sym,
                                   scm_reverse_x (lst, SCM_EOL)));

  g_scm_eval_protected (expr, scm_interaction_environment ());
  scm_dynwind_end ();
  scm_remember_upto_here_1 (expr);
}

/*! \brief Runs a object hook with a single OBJECT.
 * \par Function Description
 * Runs a hook called \a name, which should expect a list of #OBJECT
 * smobs as its argument, with a single-element list containing only \a obj.
 *
 * \see g_run_hook_object_list()
 *
 * \param name name of hook to run.
 * \param obj  #OBJECT argument for hook.
 */
void
g_run_hook_object (GschemToplevel *w_current, const char *name, OBJECT *obj)
{
  scm_dynwind_begin (0);
  g_dynwind_window (w_current);

  SCM expr = scm_list_3 (run_hook_sym,
                         g_get_hook_by_name (name),
                         scm_list_2 (list_sym, edascm_from_object (obj)));

  g_scm_eval_protected (expr, scm_interaction_environment ());
  scm_dynwind_end ();
  scm_remember_upto_here_1 (expr);
}

/*! \brief Runs a page hook.
 * \par Function Description
 * Runs a hook called \a name, which should expect the single #PAGE \a
 * page as its argument.
 *
 * \param name name of hook to run
 * \param page #PAGE argument for hook.
 */
void
g_run_hook_page (GschemToplevel *w_current, const char *name, PAGE *page)
{
  scm_dynwind_begin (0);
  g_dynwind_window (w_current);

  SCM expr = scm_list_3 (run_hook_sym,
                         g_get_hook_by_name (name),
                         edascm_from_page (page));

  g_scm_eval_protected (expr, scm_interaction_environment ());
  scm_dynwind_end ();
  scm_remember_upto_here_1 (expr);
}

/*! \brief Creates an EdascmHookProxy for a named hook.
 * Return a newly-created hook proxy object for the hook called \a
 * name.
 *
 * \param name  name of the hook for which to create a proxy.
 * \return      newly-created EdascmHookProxy instance.
 */
EdascmHookProxy *
g_hook_new_proxy_by_name (const char *name)
{
  SCM hook = g_get_hook_by_name (name);
  return edascm_hook_proxy_new_with_hook (hook);
}

/*! \brief Create the (gschem core hook) Scheme module.
 * \par Function Description
 * Defines some hooks in the (gschem core hook) module.  These hooks
 * allow Scheme callbacks to be triggered on certain gschem actions.
 * For a description of the arguments and behaviour of these hooks,
 * please see ../scheme/gschem/hook.scm.
 */
static void
init_module_gschem_core_hook ()
{

#include "g_hook.x"

#define DEFINE_HOOK(name,arity)                      \
  do { \
    scm_c_define (name, scm_make_hook (scm_from_int (arity)));      \
    scm_c_export (name, NULL); \
  } while (0)

  DEFINE_HOOK ("%add-objects-hook",1);
  DEFINE_HOOK ("%copy-objects-hook",1);
  DEFINE_HOOK ("%remove-objects-hook",1);
  DEFINE_HOOK ("%move-objects-hook",1);
  DEFINE_HOOK ("%mirror-objects-hook",1);
  DEFINE_HOOK ("%rotate-objects-hook",1);
  DEFINE_HOOK ("%paste-objects-hook",1);
  DEFINE_HOOK ("%attach-attribs-hook",1);
  DEFINE_HOOK ("%detach-attribs-hook",1);
  DEFINE_HOOK ("%select-objects-hook",1);
  DEFINE_HOOK ("%deselect-objects-hook",1);
  DEFINE_HOOK ("%new-page-hook",1);
  DEFINE_HOOK ("%action-property-hook",3);
  DEFINE_HOOK ("%bind-keys-hook",3);
}

/*!
 * \brief Initialise the gschem hooks.
 * \par Function Description

 * Registers gschem's Guile hooks for various events.. Should only be
 * called by main_prog().
 */
void
g_init_hook ()
{
  /* Define the (gschem core hook) module */
  scm_c_define_module ("gschem core hook",
                       init_module_gschem_core_hook,
                       NULL);
}
