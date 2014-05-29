# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstyuvtorgb.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstyuvtorgb_la_SOURCES = gstyuvtorgb.cpp gstyuvtorgb.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstyuvtorgb_la_CXXFLAGS = $(GST_CFLAGS) $(LIBYUV_CFLAGS)
libgstyuvtorgb_la_LIBADD = $(GST_LIBS) $(LIBYUV_LIBADD)
libgstyuvtorgb_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(LIBYUV_LDFLAGS)
libgstyuvtorgb_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstyuvtorgb.h
