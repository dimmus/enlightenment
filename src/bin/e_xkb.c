#include "e.h"

static void _e_xkb_update_event(int);

static void _e_xkb_type_reconfig(E_Pixmap_Type comp_type);
static void _e_xkb_type_update(E_Pixmap_Type comp_type, int cur_group);

static int _e_xkb_cur_group = -1;

static Ecore_Exe *cur_exe;

#ifndef HAVE_WAYLAND_ONLY
static Ecore_Event_Handler *xkb_state_handler = NULL;
static Ecore_Event_Handler *xkb_new_keyboard_handler = NULL;
static Ecore_Event_Handler *xkb_keymap_handler = NULL;
static int skip_new_keyboard = 0;
static Ecore_Timer *save_group = NULL;
#endif

E_API int E_EVENT_XKB_CHANGED = 0;

static void
_eval_cur_group(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl2, *cl = NULL;
   int cur_group = -1;

   cl = e_config->xkb.current_layout;
   if (!cl) cl = e_config->xkb.sel_layout;

   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl2)
     {
        cur_group++;
        if (!cl2->name) continue;
        if (!cl || e_config_xkb_layout_eq(cl, cl2))
          {
             INF("Setting keyboard layout: %s|%s|%s", cl2->name, cl2->model, cl2->variant);
             e_config->xkb.cur_group = cur_group;
             return;
          }
     }
   e_config->xkb.cur_group = 0;
}

#ifndef HAVE_WAYLAND_ONLY
static Eina_Bool
_e_xkb_init_timer(void *data EINA_UNUSED)
{
   if (!e_comp->root) return EINA_FALSE;

   _eval_cur_group();

   e_xkb_update(e_config->xkb.cur_group);
   ecore_x_xkb_track_state();
   return EINA_FALSE;
}
#endif

#ifndef HAVE_WAYLAND_ONLY

static Eina_Bool
_e_xkb_save_group(void *data)
{
   int group = (intptr_t)data;

   if (e_config->xkb.cur_group != group)
     {
        e_config->xkb.cur_group = group;
        e_config_save_queue();
        e_xkb_update(e_config->xkb.cur_group);
     }
   save_group = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_xkb_new_keyboard(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Ecore_X_Event_Xkb *ev = event;

   if (skip_new_keyboard > 0)
     {
        skip_new_keyboard --;
        return ECORE_CALLBACK_PASS_ON;
     }

   // skip just "xmodmap" changes
   if ((ev) && (ev->map_notify)) return ECORE_CALLBACK_PASS_ON;
   printf("XKB: EV: _xkb_new_keyboard\n");
   //we have to restore our settings here
   e_xkb_reconfig();
   e_xkb_update(e_config->xkb.cur_group);
   e_deskenv_xmodmap_run();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_xkb_new_state(void* data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Xkb *ev = event;

   if (save_group) ecore_timer_del(save_group);
   save_group = ecore_timer_loop_add(0.5, _e_xkb_save_group, (void *)(intptr_t)ev->group);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_xkb_keymap(void* data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   printf("XKB: EV: _xkb_keymap\n");
   return ECORE_CALLBACK_PASS_ON;
}
#endif

static Eina_Bool
kb_exe_del(void *d EINA_UNUSED, int t EINA_UNUSED, Ecore_Exe_Event_Del *ev)
{
   if (ev->exe == cur_exe)
     cur_exe = NULL;
   return ECORE_CALLBACK_RENEW;
}

/* externally accessible functions */
E_API int
e_xkb_init(E_Pixmap_Type comp_type)
{
   if (!E_EVENT_XKB_CHANGED)
     {
        E_EVENT_XKB_CHANGED = ecore_event_type_new();
        ecore_event_handler_add(ECORE_EXE_EVENT_DEL, (Ecore_Event_Handler_Cb)kb_exe_del, NULL);
     }
#ifndef HAVE_WAYLAND_ONLY
   if (comp_type == E_PIXMAP_TYPE_X)
     {
        xkb_state_handler = ecore_event_handler_add(ECORE_X_EVENT_XKB_STATE_NOTIFY, _xkb_new_state, NULL);
        xkb_new_keyboard_handler = ecore_event_handler_add(ECORE_X_EVENT_XKB_NEWKBD_NOTIFY, _xkb_new_keyboard, NULL);
        xkb_keymap_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_KEYMAP, _xkb_keymap, NULL);
     }
#endif
   if (e_config->xkb.dont_touch_my_damn_keyboard) return 1;
   printf("XKB: e_xkb_init\n");

   _e_xkb_type_reconfig(comp_type);

#ifndef HAVE_WAYLAND_ONLY
   if (comp_type == E_PIXMAP_TYPE_X)
     ecore_timer_loop_add(1.5, _e_xkb_init_timer, NULL);
   else
#endif
   if (comp_type == E_PIXMAP_TYPE_WL)
     {
        _eval_cur_group();
        _e_xkb_type_update(comp_type, e_config->xkb.cur_group);
     }

   return 1;
}

E_API int
e_xkb_shutdown(void)
{
#ifndef HAVE_WAYLAND_ONLY
   E_FREE_FUNC(xkb_keymap_handler, ecore_event_handler_del);
   E_FREE_FUNC(xkb_state_handler, ecore_event_handler_del);
   E_FREE_FUNC(xkb_new_keyboard_handler, ecore_event_handler_del);
   ecore_timer_del(save_group);
   save_group = NULL;
#endif
   return 1;
}

static void
_e_x_xkb_reconfig(void)
{
   E_Config_XKB_Layout *cl;
   E_Config_XKB_Option *op;
   Eina_List *l;
   Eina_Strbuf *buf;

   if (e_config->xkb.dont_touch_my_damn_keyboard) return;
   printf("XKB: _e_x_xkb_reconfig\n");
   if ((!e_config->xkb.used_layouts) && (!e_config->xkb.used_options) && (!e_config->xkb.default_model)) return;
   if (!getenv("DISPLAY")) return;

   buf = eina_strbuf_new();
   eina_strbuf_append(buf, "setxkbmap ");

   if (e_config->xkb.used_layouts)
     {
        eina_strbuf_append(buf, "-layout '");
        EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
          {
             if (cl->name)
               {
                  eina_strbuf_append(buf, cl->name);
                  eina_strbuf_append(buf, ",");
               }
          }

        eina_strbuf_append(buf, "' -variant '");
        EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
          {
             if ((cl->variant) && (strcmp(cl->variant, "basic")))
               {
                  eina_strbuf_append(buf, cl->variant);
                  eina_strbuf_append(buf, ",");
               }
             else
               eina_strbuf_append(buf, ",");
          }
        eina_strbuf_append(buf, "'");

        /* use first entry in used layouts */
        cl = e_config->xkb.used_layouts->data;

        if (cl->model)
          {
             eina_strbuf_append(buf, " -model '");
             if (strcmp(cl->model, "default"))
               eina_strbuf_append(buf, cl->model);
             else if ((e_config->xkb.default_model) &&
                      (strcmp(e_config->xkb.default_model, "default")))
               eina_strbuf_append(buf, e_config->xkb.default_model);
             else
               eina_strbuf_append(buf, "default");
             eina_strbuf_append(buf, "'");
          }
     }
   else if (e_config->xkb.default_model)
     {
        eina_strbuf_append(buf, " -model '");
        eina_strbuf_append(buf, e_config->xkb.default_model);
        eina_strbuf_append(buf, "'");
     }

   if (e_config->xkb.used_options)
     {
        /* clear options */
        eina_strbuf_append(buf, " -option ");

        /* add in selected options */
        EINA_LIST_FOREACH(e_config->xkb.used_options, l, op)
          {
             if (op->name)
               {
                  eina_strbuf_append(buf, " -option '");
                  eina_strbuf_append(buf, op->name);
                  eina_strbuf_append(buf, "'");
               }
          }
     }
#ifndef HAVE_WAYLAND_ONLY
   skip_new_keyboard ++;
#endif
   INF("SET XKB RUN: %s", eina_strbuf_string_get(buf));
   E_FREE_FUNC(cur_exe, ecore_exe_kill);
   cur_exe = ecore_exe_run(eina_strbuf_string_get(buf), NULL);
   eina_strbuf_free(buf);
}

static void
_e_x_xkb_update(int cur_group)
{
   if (e_config->xkb.dont_touch_my_damn_keyboard) return;
   printf("XKB: _e_x_xkb_update\n");
   if ((!e_config->xkb.used_layouts) && (!e_config->xkb.used_options) && (!e_config->xkb.default_model)) return;
   if (!getenv("DISPLAY")) return;
   if (cur_group != _e_xkb_cur_group)
     {
        _e_xkb_cur_group = cur_group;
#ifndef HAVE_WAYLAND_ONLY
        if (e_comp->root)
          {
             e_comp_canvas_keys_ungrab();
             ecore_x_xkb_select_group(cur_group);
             e_comp_canvas_keys_grab();
          }
#endif
        e_deskenv_xmodmap_run();
        _e_xkb_update_event(cur_group);
        return;
     }
}


static void
_e_wl_xkb_update(int cur_group)
{
#ifdef HAVE_WAYLAND
   e_comp_wl_input_keymap_index_set(cur_group);
   _e_xkb_update_event(cur_group);
#else
   (void)cur_group;
#endif
}


static void
_e_wl_xkb_reconfig(void)
{
#ifdef HAVE_WAYLAND
   E_Config_XKB_Option *op;
   E_Config_XKB_Layout *cl;
   Eina_Strbuf *options, *layouts, *variants;
   Eina_List *l;


   options = eina_strbuf_new();

    /* create options */
   EINA_LIST_FOREACH(e_config->xkb.used_options, l, op)
     {
        if (op->name)
          {
             eina_strbuf_append(options, op->name);
             eina_strbuf_append_char(options, ',');
          }
     }

   layouts = eina_strbuf_new();
   variants = eina_strbuf_new();

   //search for the correct layout

   /* create layouts */
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl)
     {
        if (cl->name)
          {
             eina_strbuf_append(layouts, cl->name);
             eina_strbuf_append_char(layouts, ',');
          }
        else
          eina_strbuf_append_char(layouts, ',');

        if ((cl->variant) && (strcmp(cl->variant, "basic")))
          {
             eina_strbuf_append(variants, cl->variant);
             eina_strbuf_append_char(variants, ',');
          }
        else
          eina_strbuf_append_char(variants, ',');
     }

   /* collect model to use */

   /* set keymap to the compositor */
   e_comp_wl_input_keymap_set(NULL,
                              e_config->xkb.default_model,
                              eina_strbuf_string_get(layouts), //pool of layouts to use
                              eina_strbuf_string_get(variants),  //pool of variants to use
                              eina_strbuf_string_get(options) //list of options
                             );

   eina_strbuf_free(variants);
   eina_strbuf_free(layouts);
   eina_strbuf_free(options);
#endif
}

static void
_e_xkb_type_reconfig(E_Pixmap_Type comp_type)
{
   printf("XKB: _e_xkb_type_reconfig\n");
   if (comp_type == E_PIXMAP_TYPE_X)
     _e_x_xkb_reconfig();
   else if (comp_type == E_PIXMAP_TYPE_WL)
     _e_wl_xkb_reconfig();
}

E_API void
e_xkb_reconfig(void)
{
   _e_xkb_type_reconfig(e_comp->comp_type);
}

static void
_e_xkb_type_update(E_Pixmap_Type comp_type, int cur_group)
{
   e_config->xkb.cur_group = cur_group;
   e_config_save_queue();

   if (comp_type == E_PIXMAP_TYPE_WL)
     _e_wl_xkb_update(cur_group);
   else
     _e_x_xkb_update(cur_group);
}

E_API void
e_xkb_update(int cur_group)
{
   _e_xkb_type_update(e_comp->comp_type, cur_group);
}

E_API void
e_xkb_layout_next(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl;

   if (e_config->xkb.dont_touch_my_damn_keyboard) return;
   printf("XKB: e_xkb_layout_next\n");
   if (!e_config->xkb.used_layouts) return;
   l = eina_list_nth_list(e_config->xkb.used_layouts, e_config->xkb.cur_group);
   l = eina_list_next(l);
   if (!l) l = e_config->xkb.used_layouts;

   e_config->xkb.cur_group = (e_config->xkb.cur_group + 1) % eina_list_count(e_config->xkb.used_layouts);
   cl = eina_list_data_get(l);
   eina_stringshare_replace(&e_config->xkb.cur_layout, cl->name);
   eina_stringshare_replace(&e_config->xkb.selected_layout, cl->name);
   INF("Setting keyboard layout: %s|%s|%s", cl->name, cl->model, cl->variant);
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

E_API void
e_xkb_layout_prev(void)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl;

   if (e_config->xkb.dont_touch_my_damn_keyboard) return;
   printf("XKB: e_xkb_layout_prev\n");
   if (!e_config->xkb.used_layouts) return;
   l = eina_list_nth_list(e_config->xkb.used_layouts, e_config->xkb.cur_group);
   l = eina_list_prev(l);
   if (!l) l = eina_list_last(e_config->xkb.used_layouts);

   e_config->xkb.cur_group = (e_config->xkb.cur_group == 0) ?
     ((int)eina_list_count(e_config->xkb.used_layouts) - 1) : (e_config->xkb.cur_group - 1);
   cl = eina_list_data_get(l);
   eina_stringshare_replace(&e_config->xkb.cur_layout, cl->name);
   eina_stringshare_replace(&e_config->xkb.selected_layout, cl->name);
   INF("Setting keyboard layout: %s|%s|%s", cl->name, cl->model, cl->variant);
   e_xkb_update(e_config->xkb.cur_group);
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

/* always use this function to get the current layout's name
 * to ensure the most accurate results!!!
 */
E_API E_Config_XKB_Layout *
e_xkb_layout_get(void)
{
   unsigned int n = 0;

   if (e_config->xkb.dont_touch_my_damn_keyboard) return NULL;
   printf("XKB: e_xkb_layout_get\n");
   if (e_config->xkb.current_layout) return e_config->xkb.current_layout;
   if (_e_xkb_cur_group >= 0)
     n = _e_xkb_cur_group;
   return eina_list_nth(e_config->xkb.used_layouts, n);
}

E_API void
e_xkb_layout_set(const E_Config_XKB_Layout *cl)
{
   Eina_List *l;
   E_Config_XKB_Layout *cl2;
   int cur_group = -1;

   EINA_SAFETY_ON_NULL_RETURN(cl);
   if (e_config->xkb.dont_touch_my_damn_keyboard) return;
   printf("XKB: e_xkb_layout_set\n");
   if (e_config_xkb_layout_eq(e_config->xkb.current_layout, cl)) return;
   cl2 = e_config_xkb_layout_dup(e_config->xkb.current_layout);
   e_config_xkb_layout_free(e_config->xkb.current_layout);
   e_config->xkb.current_layout = cl2;
   EINA_LIST_FOREACH(e_config->xkb.used_layouts, l, cl2)
     {
        cur_group++;
        if (!cl2->name) continue;
        if (e_config_xkb_layout_eq(cl, cl2))
          {
             INF("Setting keyboard layout: %s|%s|%s", cl2->name, cl2->model, cl2->variant);
             eina_stringshare_replace(&e_config->xkb.cur_layout, cl->name);
             eina_stringshare_replace(&e_config->xkb.selected_layout, cl->name);
             e_xkb_update(cur_group);
             break;
          }
     }
   _e_xkb_update_event(e_config->xkb.cur_group);
   e_config_save_queue();
}

E_API const char *
e_xkb_layout_name_reduce(const char *name)
{
   const char *s;

   if (!name) return NULL;
   s = strchr(name, '/');
   if (s) s++;
   else s = name;
   return s;
}

E_API void
e_xkb_e_icon_flag_setup(Evas_Object *eicon, const char *name)
{
   int w, h;
   char buf[PATH_MAX];

   e_xkb_flag_file_get(buf, sizeof(buf), name);
   e_icon_preload_set(eicon, EINA_FALSE);
   e_icon_file_set(eicon, buf);
   e_icon_size_get(eicon, &w, &h);
   evas_object_size_hint_aspect_set(eicon, EVAS_ASPECT_CONTROL_BOTH, w, h);
}

E_API void
e_xkb_flag_file_get(char *buf, size_t bufsize, const char *name)
{
   name = e_xkb_layout_name_reduce(name);
   snprintf(buf, bufsize, "%s/data/flags/%s_flag.png",
            e_prefix_data_get(), name ? name : "unknown");
   if (!ecore_file_exists(buf))
     snprintf(buf, bufsize, "%s/data/flags/unknown_flag.png",
              e_prefix_data_get());
}

E_API Eina_Bool
e_config_xkb_layout_eq(const E_Config_XKB_Layout *a, const E_Config_XKB_Layout *b)
{
   if (a == b) return EINA_TRUE;
   if ((!a) || (!b)) return EINA_FALSE;
   return ((a->name == b->name) && (a->model == b->model) && (a->variant == b->variant));
}

E_API void
e_config_xkb_layout_free(E_Config_XKB_Layout *cl)
{
   if (!cl) return;

   eina_stringshare_del(cl->name);
   eina_stringshare_del(cl->model);
   eina_stringshare_del(cl->variant);
   free(cl);
}

E_API E_Config_XKB_Layout *
e_config_xkb_layout_dup(const E_Config_XKB_Layout *cl)
{
   E_Config_XKB_Layout *cl2;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cl, NULL);
   cl2 = E_NEW(E_Config_XKB_Layout, 1);
   cl2->name = eina_stringshare_ref(cl->name);
   cl2->model = eina_stringshare_ref(cl->model);
   cl2->variant = eina_stringshare_ref(cl->variant);
   return cl2;
}

static void
_e_xkb_update_event(int cur_group)
{
   ecore_event_add(E_EVENT_XKB_CHANGED, NULL, NULL, (intptr_t *)(long)cur_group);
}

