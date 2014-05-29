# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstlibyuvscaler.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstlibyuvscaler_la_SOURCES = gstlibyuvscaler.c gstlibyuvscaler.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstlibyuvscaler_la_CFLAGS = $(GST_CFLAGS) $(LIBYUV_CFLAGS)
libgstlibyuvscaler_la_LIBADD = $(GST_LIBS) $(LIBYUV_LIBS)
libgstlibyuvscaler_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(LIBYUV_LDFLAGS)
libgstlibyuvscaler_la_LIBTOOLFLAGS =

# headers we need but don't want installed
noinst_HEADERS = gstlibyuvscaler.h
