ACLOCAL_AMFLAGS = -I m4

# Compiler flags
AM_CFLAGS = -Wall -Werror -fPIC

# 1. Header Files
# These will be installed to $(prefix)/include (e.g., /usr/local/include/memarena.h)
include_HEADERS = src/include/memarena.h

# 2. Shared Library
# Libtool will build 'libmemarena.la' and the corresponding .so/.dll/.dylib files.
lib_LTLIBRARIES = libmemarena.la

libmemarena_la_SOURCES = src/memarena.c

# LDFLAGS configuration:
# -avoid-version:  Prevents version numbers (like .so.0.0.1). 
#                  Useful if this is internal or you always want the naked .so file.
# -no-undefined:   CRITICAL for your future Windows port. 
#                  It declares that the library has no unresolved symbols, allowing DLL creation.
# -export-dynamic: Ensures symbols are visible for dlopen().
libmemarena_la_LDFLAGS = -avoid-version -no-undefined -export-dynamic

# 3. Pkg-config
# Installs the .pc file so other projects can find your library with `pkg-config --libs libmemarena`
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = memarena.pc
