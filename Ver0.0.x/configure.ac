#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(EvServer, 0.0.15, mail@kabu.direct)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([evs_close.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

AC_CHECK_HEADER(/usr/include/ev.h,
        LIBEV_LIB='-lev',
        AC_MSG_WARN(*** libev header files not found ***))
AC_SUBST(LIBEV_LIB)

AC_CHECK_HEADER(/usr/include/openssl/ssl.h,
        LIBSSL_LIB='-lssl',
        AC_MSG_WARN(*** Openssl header files not found ***))
AC_SUBST(LIBSSL_LIB)

AC_CHECK_HEADER(/usr/include/openssl/crypto.h,
        LIBCRYPTO_LIB='-lcrypto',
        AC_MSG_WARN(*** Openssl/crypto header files not found ***))
AC_SUBST(LIBCRYPTO_LIB)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset socket strchr strerror strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
