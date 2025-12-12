ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Werror -fPIC

include_HEADERS = src/include/memarena.h
lib_LTLIBRARIES = libmemarena.la
libmemarena_la_SOURCES = src/memarena.c
libmemarena_la_LDFLAGS = -avoid-version -module -shared -export-dynamic

all-local: memarena.so

memarena.so: .libs/libmemarena.so
	cp -f $< $@

# Install the real .so as memarena.so (not libmemarena.so.x.y.z)
install-exec-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)
	$(INSTALL_PROGRAM) .libs/libmemarena.so $(DESTDIR)$(libdir)/memarena.so

uninstall-local:
	rm -f $(DESTDIR)$(libdir)/memarena.so

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = memarena.pc

# Clean generated .pc file
CLEANFILES = memarena.so memarena.pc

install-exec-hook:
	if [ -z "$(DESTDIR)" ] && [ "$(libdir)" != "$(prefix)/lib" -o "$(prefix)" = "/usr" -o "$(prefix)" = "/usr/local" ]; then \
	    echo "Running ldconfig..." ; \
	    ldconfig || true ; \
	fi

