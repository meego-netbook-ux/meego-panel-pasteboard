AC_PREREQ(2.53)
AC_INIT(moblin-panel-pasteboard, 0.0.4)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=moblin-panel-pasteboard
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

CFLAGS="$CFLAGS -Wall"

PKG_CHECK_MODULES(MPL, moblin-panel >= 0.50.0)

MOBLIN_PANELS_DIR=`$PKG_CONFIG --variable=moblin_panel_panels_dir moblin-panel`
AC_SUBST(MOBLIN_PANELS_DIR)

PKG_CHECK_MODULES(PASTEBOARD,
                  clutter-x11-1.0
                  clutter-1.0
                  gtk+-2.0
                  nbtk-1.2 >= 1.1.3)

AC_ARG_ENABLE([cache],
              [AC_HELP_STRING([--enable-cache],
                              [Enable Nbtk image cache generation])],
              [],
              [enable_cache=no])

AS_IF([test "x$enable_cache" = "xyes"],
      [
      AC_CHECK_PROG([NBTK_CREATE_IMAGE_CACHE],
                [nbtk-create-image-cache],
                [$(which nbtk-create-image-cache)])
])

AC_ARG_ENABLE([moblin-desktop-file],
              [AC_HELP_STRING([--disable-moblin-desktop-file],
                              [Disable installation of moblin desktop file])],
              [enable_mdesktop=$enableval],
              [enable_mdesktop=yes])

AM_CONDITIONAL([ENABLE_MOBLIN_DESKTOP_FILE], [test "x$enable_mdesktop" = "xyes"])

# glib-genmarshal
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

AM_CONDITIONAL([ENABLE_CACHE],   [test "x$enable_cache" = "xyes"])

AC_OUTPUT([
        Makefile
        src/Makefile
        data/Makefile
        data/theme/Makefile
        po/Makefile.in
])
