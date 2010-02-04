// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

// This is just an intermediate module for making libarccredential library
// persistent in compatible way.

// Adding plugin descriptor to avoid warning messages from loader
Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { NULL, NULL, 0, NULL }
};

