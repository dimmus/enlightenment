/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_shelf_free(E_Shelf *es);
static void _e_shelf_position_calc(E_Shelf *es);
static void _e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static Evas_Object *_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style);

static Evas_List *shelves = NULL;
static int shelf_id = 0;

/* FIXME: shelves need to do this:
 * 1. allow them to be moved, resized etc. etc.
 * 2. have a configuration panel per shelf to select if its inline, layer etc.
 * 3. catch all right clicks not on modules for right click context menu
 * 4. a global config dialog that lists shelves u have to configure them
 */

/* externally accessible functions */
EAPI int
e_shelf_init(void)
{
   return 1;
}

EAPI int
e_shelf_shutdown(void)
{
   return 1;
}

EAPI void
e_shelf_config_init(void)
{
   Evas_List *l, *l2;
   
   for (l = e_config->shelves; l; l = l->next)
     {
	E_Config_Shelf *cf_es;
	E_Zone *zone;
	int closeness;
	
	cf_es = l->data;
	zone = e_util_container_zone_number_get(cf_es->container, cf_es->zone);
	if (zone)
	  {
	     E_Shelf *es;
	     
	     es = e_shelf_zone_new(zone, cf_es->name, cf_es->style,
				   cf_es->popup, cf_es->layer);
	     if (es)
	       {
		  es->cfg = cf_es;
		  e_shelf_orient(es, cf_es->orient);
		  _e_shelf_position_calc(es);
		  e_shelf_populate(es);
		  e_shelf_show(es);
	       }
	  }
     }
}

EAPI E_Shelf *
e_shelf_zone_new(E_Zone *zone, const char *name, const char *style, int popup, int layer)
{
   E_Shelf *es;
   char buf[1024];
   
   es = E_OBJECT_ALLOC(E_Shelf, E_SHELF_TYPE, _e_shelf_free);
   if (!es) return NULL;

   es->x = 0;
   es->y = 0;
   es->w = 32;
   es->h = 32;
   if (popup)
     {
	es->popup = e_popup_new(zone, es->x, es->y, es->w, es->h);
	e_popup_layer_set(es->popup, layer);
	es->ee = es->popup->ecore_evas;
	es->evas = es->popup->evas;
     }
   else
     {
	es->ee = zone->container->bg_ecore_evas;
	es->evas = zone->container->bg_evas;
     }
   es->fit_along = 1;
   es->layer = layer;
   es->zone = zone;
   es->style = evas_stringshare_add(style);
   
   es->o_base = edje_object_add(es->evas);
   es->name = evas_stringshare_add(name);
   snprintf(buf, sizeof(buf), "shelf/%s/base", es->style);
   evas_object_resize(es->o_base, es->w, es->h);
   if (!e_theme_edje_object_set(es->o_base, "base/theme/shelf", buf))
     e_theme_edje_object_set(es->o_base, "base/theme/shelf",
			     "shelf/default/base");
   if (es->popup)
     {
	evas_object_show(es->o_base);
	e_popup_edje_bg_object_set(es->popup, es->o_base);
     }
   else
     {
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_layer_set(es->o_base, layer);
     }

   snprintf(buf, sizeof(buf), "%i", shelf_id);
   shelf_id++;
   es->gadcon = e_gadcon_swallowed_new(es->name, buf, es->o_base, "items");
   e_gadcon_size_request_callback_set(es->gadcon,
				      _e_shelf_gadcon_size_request,
				      es);
   e_gadcon_frame_request_callback_set(es->gadcon,
				       _e_shelf_gadcon_frame_request,
				       es);
   e_gadcon_orient(es->gadcon, E_GADCON_ORIENT_TOP);
   edje_object_signal_emit(es->o_base, "set_orientation", "top");
   e_gadcon_zone_set(es->gadcon, zone);
   e_gadcon_ecore_evas_set(es->gadcon, es->ee);
   
   shelves = evas_list_append(shelves, es);
   return es;
}

EAPI void
e_shelf_populate(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   e_gadcon_populate(es->gadcon);
}

EAPI void
e_shelf_show(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->popup)
     e_popup_show(es->popup);
   else
     evas_object_show(es->o_base);
}

EAPI void
e_shelf_hide(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->popup)
     e_popup_hide(es->popup);
   else
     evas_object_hide(es->o_base);
}

EAPI void
e_shelf_move(E_Shelf *es, int x, int y)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->x = x;
   es->y = y;
   if (es->popup)
     e_popup_move(es->popup, es->x, es->y);
   else
     evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
}

EAPI void
e_shelf_resize(E_Shelf *es, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->w = w;
   es->h = h;
   if (es->popup)
     {
	e_popup_resize(es->popup, es->w, es->h);
	evas_object_resize(es->o_base, es->w, es->h);
     }
   else
     evas_object_resize(es->o_base, es->w, es->h);
}

EAPI void
e_shelf_move_resize(E_Shelf *es, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->x = x;
   es->y = y;
   es->w = w;
   es->h = h;
   if (es->popup)
     {
	e_popup_move_resize(es->popup, es->x, es->y, es->w, es->h);
	evas_object_resize(es->o_base, es->w, es->h);
     }
   else
     {
	evas_object_move(es->o_base, es->zone->x + es->x, es->zone->y + es->y);
	evas_object_resize(es->o_base, es->w, es->h);
     }
}

EAPI void
e_shelf_layer_set(E_Shelf *es, int layer)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   es->layer = layer;
   if (es->popup)
     e_popup_layer_set(es->popup, es->layer);
   else
     evas_object_layer_set(es->o_base, es->layer);
}

EAPI void
e_shelf_save(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->cfg)
     {
	es->cfg->orient = es->gadcon->orient;
	if (es->cfg->style) evas_stringshare_del(es->cfg->style);
	es->cfg->style = evas_stringshare_add(es->style);
     }
   else
     {
	E_Config_Shelf *cf_es;
	
	cf_es = E_NEW(E_Config_Shelf, 1);
	cf_es->name = evas_stringshare_add(es->name);
	cf_es->container = es->zone->container->num;
	cf_es->zone = es->zone->num;
	if (es->popup) cf_es->popup = 1;
	cf_es->layer = es->layer;
	e_config->shelves = evas_list_append(e_config->shelves, cf_es);
	cf_es->orient = es->gadcon->orient;
	cf_es->style = evas_stringshare_add(es->style);
	cf_es->fit_along = es->fit_along;
	cf_es->fit_size = es->fit_size;
	es->cfg = cf_es;
     }
   e_config_save_queue();
}

EAPI void
e_shelf_unsave(E_Shelf *es)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   if (es->cfg)
     {
	e_config->shelves = evas_list_remove(e_config->shelves, es->cfg);
	evas_stringshare_del(es->cfg->name);
	if (es->cfg->style) evas_stringshare_del(es->cfg->style);
	free(es->cfg);
     }
}

EAPI void
e_shelf_orient(E_Shelf *es, E_Gadcon_Orient orient)
{
   E_OBJECT_CHECK(es);
   E_OBJECT_TYPE_CHECK(es, E_GADMAN_SHELF_TYPE);
   e_gadcon_orient(es->gadcon, orient);
   switch (es->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	edje_object_signal_emit(es->o_base, "set_orientation", "float");
	break;
      case E_GADCON_ORIENT_HORIZ:
	edje_object_signal_emit(es->o_base, "set_orientation", "horizontal");
	break;
      case E_GADCON_ORIENT_VERT:
	edje_object_signal_emit(es->o_base, "set_orientation", "vertical");
	break;
      case E_GADCON_ORIENT_LEFT:
	edje_object_signal_emit(es->o_base, "set_orientation", "left");
	break;
      case E_GADCON_ORIENT_RIGHT:
	edje_object_signal_emit(es->o_base, "set_orientation", "right");
	break;
      case E_GADCON_ORIENT_TOP:
	edje_object_signal_emit(es->o_base, "set_orientation", "top");
	break;
      case E_GADCON_ORIENT_BOTTOM:
	edje_object_signal_emit(es->o_base, "set_orientation", "bottom");
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	edje_object_signal_emit(es->o_base, "set_orientation", "top_left");
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	edje_object_signal_emit(es->o_base, "set_orientation", "top_right");
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	edje_object_signal_emit(es->o_base, "set_orientation", "bottom_left");
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	edje_object_signal_emit(es->o_base, "set_orientation", "bottom_right");
	break;
      default:
	break;
     }
}

/* local subsystem functions */
static void
_e_shelf_free(E_Shelf *es)
{
   shelves = evas_list_remove(shelves, es);
   e_object_del(E_OBJECT(es->gadcon));
   evas_stringshare_del(es->name);
   evas_stringshare_del(es->style);
   evas_object_del(es->o_base);
   if (es->popup) e_object_del(E_OBJECT(es->popup));
   free(es);
}

static void
_e_shelf_position_calc(E_Shelf *es)
{
   E_Gadcon_Orient orient = E_GADCON_ORIENT_FLOAT;
   int size = 40;
   
   if (es->cfg)
     {
	orient = es->cfg->orient;
	size = es->cfg->size;
     }
   else
     orient = es->gadcon->orient;
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	break;
      case E_GADCON_ORIENT_HORIZ:
	break;
      case E_GADCON_ORIENT_VERT:
	break;
      case E_GADCON_ORIENT_LEFT:
	break;
      case E_GADCON_ORIENT_RIGHT:
	break;
      case E_GADCON_ORIENT_TOP:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = (es->zone->w - es->w) / 2;
	es->y = 0;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	if (!es->fit_along) es->w = es->zone->w;
	if (!es->fit_size) es->h = size;
	es->x = (es->zone->w - es->w) / 2;
	es->y = es->zone->h - es->h;
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	break;
      default:
	break;
     }
   e_shelf_move_resize(es, es->x, es->y, es->w, es->h);
}

static void
_e_shelf_gadcon_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h)
{
   E_Shelf *es;
   Evas_Coord nx, ny, nw, nh, ww, hh;
   
   es = data;
   nx = es->x;
   ny = es->y;
   nw = es->w;
   nh = es->h;
   ww = hh = 0;
   printf("req min = %i %i\n", w, h);
   evas_object_geometry_get(gc->o_container, NULL, NULL, &ww, &hh);
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	break;
      case E_GADCON_ORIENT_HORIZ:
	break;
      case E_GADCON_ORIENT_VERT:
	break;
      case E_GADCON_ORIENT_LEFT:
	break;
      case E_GADCON_ORIENT_RIGHT:
	break;
      case E_GADCON_ORIENT_TOP:
	if (!es->fit_along) w = ww;
	if (!es->fit_size) h = hh;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	if (!es->fit_along) w = ww;
	if (!es->fit_size) h = hh;
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	break;
      default:
	break;
     }
   printf("adj min = %i %i\n", w, h);
   e_gadcon_swallowed_min_size_set(gc, w, h);
   edje_object_size_min_calc(es->o_base, &nw, &nh);
   printf("new w, h = %i %i\n", nw, nh);
   switch (gc->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
	break;
      case E_GADCON_ORIENT_HORIZ:
	break;
      case E_GADCON_ORIENT_VERT:
	break;
      case E_GADCON_ORIENT_LEFT:
	break;
      case E_GADCON_ORIENT_RIGHT:
	break;
      case E_GADCON_ORIENT_TOP:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = es->x + ((es->w - nw) / 2);
	ny = 0;
	break;
      case E_GADCON_ORIENT_BOTTOM:
	if (!es->fit_along) nw = es->w;
	if (!es->fit_size) nh = es->h;
	if (nw > es->zone->w) nw = es->zone->w;
	if (nh > es->zone->h) nh = es->zone->h;
	if (nw != es->w) nx = es->x + ((es->w - nw) / 2);
	ny = es->zone->h - nh;
	break;
      case E_GADCON_ORIENT_CORNER_TL:
	break;
      case E_GADCON_ORIENT_CORNER_TR:
	break;
      case E_GADCON_ORIENT_CORNER_BL:
	break;
      case E_GADCON_ORIENT_CORNER_BR:
	break;
      default:
	break;
     }
   e_shelf_move_resize(es, nx, ny, nw, nh);
}

static Evas_Object *
_e_shelf_gadcon_frame_request(void *data, E_Gadcon_Client *gcc, const char *style)
{
   E_Shelf *es;
   Evas_Object *o;
   char buf[4096];
   
   es = data;
   o = edje_object_add(gcc->gadcon->evas);
   snprintf(buf, sizeof(buf), "shelf/%s/%s", es->style, style);
   if (!e_theme_edje_object_set(o, "base/theme/shelf", buf))
     {
	evas_object_del(o);
	return NULL;
     }
   return o;
}
