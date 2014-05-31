# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstrgbtoyuv.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstrgbtoyuv_la_SOURCES = gstrgbtoyuv.cpp gstrgbtoyuv.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstrgbtoyuv_la_CXXFLAGS = $(GST_CFLAGS) -I/Users/davidchen/Workspace/Remotium/external/libyuv/include
libgstrgbtoyuv_la_LIBADD = $(GST_LIBS) -lyuv
libgstrgbtoyuv_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) -L/Users/davidchen/Workspace/Remotium/external/prebuilt/macosx/lib
libgstrgbtoyuv_la_LIBTOOLFLAGS =

# headers we need but don't want installed
noinst_HEADERS = gstrgbtoyuv.h
