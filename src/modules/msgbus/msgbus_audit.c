#include "e_mod_main.h"

static int _log_dom = -1;
#undef DBG
#undef WARN
#undef INF
#undef ERR
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)
#define WARN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

static EDBus_Message *
cb_audit_timer_dump(const EDBus_Service_Interface *iface __UNUSED__,
                    const EDBus_Message *msg)
{
   EDBus_Message *reply = edbus_message_method_return_new(msg);
   char *tmp;

   tmp = ecore_timer_dump();
   if (!tmp)
     edbus_message_arguments_append(reply, "s",
                                    "Not enable, recompile Ecore with ecore_timer_dump.");
   else
     edbus_message_arguments_append(reply, "s", tmp);

   return reply;
}

static const EDBus_Method methods[] = {
   { "Timers", NULL, EDBUS_ARGS({"s", ""}), cb_audit_timer_dump, 0 },
   { NULL, NULL, NULL, NULL, 0 }
};

static const EDBus_Service_Interface_Desc audit = {
  "org.enlightenment.wm.Audit", methods, NULL, NULL, NULL, NULL
};

void msgbus_audit_init(Eina_Array *ifaces)
{
   EDBus_Service_Interface *iface;

   if (_log_dom == -1)
     {
	_log_dom = eina_log_domain_register("msgbus_audit", EINA_COLOR_BLUE);
	if (_log_dom < 0)
	  EINA_LOG_ERR("could not register msgbus_audit log domain!");
     }

   iface = e_msgbus_interface_attach(&audit);
   if (iface)
     eina_array_push(ifaces, iface);
}
