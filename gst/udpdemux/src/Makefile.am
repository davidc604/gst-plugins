# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstudpdemux.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libgstudpdemux_la_SOURCES                       #
#                            libgstudpdemux_la_CFLAGS                        #
#                            libgstudpdemux_la_LIBADD                        #
#                            libgstudpdemux_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstudpdemux_la_SOURCES = gstudpdemux.c gstudpdemux.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstudpdemux_la_CFLAGS = $(GST_CFLAGS)
libgstudpdemux_la_LIBADD = $(GST_LIBS)
libgstudpdemux_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstudpdemux_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstudpdemux.h
