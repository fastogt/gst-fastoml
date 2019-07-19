#ifndef __GST_CHILD_INSPECTOR_H__
#define __GST_CHILD_INSPECTOR_H__

#include <gst/gst.h>

G_BEGIN_DECLS

gchar* gst_child_inspector_property_to_string(GObject* object, GParamSpec* param, guint alignment);
gchar* gst_child_inspector_properties_to_string(GObject* object, guint alignment, gchar* title);

G_END_DECLS

#endif /* __GST_CHILD_INSPECTOR_H__ */
