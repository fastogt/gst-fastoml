#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstyolov2.h"

GST_DEBUG_CATEGORY_STATIC(gst_plugin_template_debug);
#define GST_CAT_DEFAULT gst_plugin_template_debug

/* Filter signals and args */
enum {
  /* FILL ME */
  LAST_SIGNAL
};

enum { PROP_0, PROP_SILENT };

/* pad templates */
#define SINK_CAPS "video/x-raw, format={RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}"
#define SRC_CAPS "video/x-raw, format={RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}"

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS(SINK_CAPS));

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(SRC_CAPS));

#define gst_plugin_template_parent_class parent_class
G_DEFINE_TYPE(GstPluginTemplate, gst_plugin_template, GST_TYPE_ELEMENT);

static void gst_plugin_template_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_plugin_template_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

static gboolean gst_plugin_template_sink_event(GstPad* pad, GstObject* parent, GstEvent* event);
static GstFlowReturn gst_plugin_template_chain(GstPad* pad, GstObject* parent, GstBuffer* buf);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void gst_plugin_template_class_init(GstPluginTemplateClass* klass) {
  GObjectClass* gobject_class;
  GstElementClass* gstelement_class;

  gobject_class = (GObjectClass*)klass;
  gstelement_class = (GstElementClass*)klass;

  gobject_class->set_property = gst_plugin_template_set_property;
  gobject_class->get_property = gst_plugin_template_get_property;

  g_object_class_install_property(
      gobject_class, PROP_SILENT,
      g_param_spec_boolean("silent", "Silent", "Produce verbose output ?", FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class, "Plugin", "FIXME:Generic", "FIXME:Generic Template Element",
                                       "AUTHOR_NAME AUTHOR_EMAIL");

  gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_plugin_template_init(GstPluginTemplate* filter) {
  filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
  gst_pad_set_event_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_plugin_template_sink_event));
  gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_plugin_template_chain));
  GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS(filter->srcpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);

  filter->silent = FALSE;
}

static void gst_plugin_template_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec) {
  GstPluginTemplate* filter = GST_PLUGIN_TEMPLATE(object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void gst_plugin_template_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec) {
  GstPluginTemplate* filter = GST_PLUGIN_TEMPLATE(object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean(value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean gst_plugin_template_sink_event(GstPad* pad, GstObject* parent, GstEvent* event) {
  GstPluginTemplate* filter;
  gboolean ret;

  filter = GST_PLUGIN_TEMPLATE(parent);

  GST_LOG_OBJECT(filter, "Received %s event: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), event);

  switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CAPS: {
      GstCaps* caps;

      gst_event_parse_caps(event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default(pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default(pad, parent, event);
      break;
  }
  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_plugin_template_chain(GstPad* pad, GstObject* parent, GstBuffer* buf) {
  GstPluginTemplate* filter;

  filter = GST_PLUGIN_TEMPLATE(parent);

  if (filter->silent == FALSE)
    g_print("I'm plugged, therefore I'm in.\n");

  /* just push out the incoming buffer without touching it */
  return gst_pad_push(filter->srcpad, buf);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin* plugin) {
  /* debug category for fltering log messages
   *
   * exchange the string 'Template plugin' with your description
   */
  GST_DEBUG_CATEGORY_INIT(gst_plugin_template_debug, "yolov2", 0, "Yolov2 plugin");

  return gst_element_register(plugin, "yolov2", GST_RANK_NONE, GST_TYPE_PLUGIN_TEMPLATE);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  yolov2,
                  "Yolov2 plugin",
                  plugin_init,
                  PACKAGE_VERSION,
                  GST_LICENSE,
                  GST_PACKAGE_NAME,
                  GST_PACKAGE_ORIGIN)
