# Ocelot Configure script
# Generate ./configure using autoreconf (or autoconf for the brave)

AC_INIT(ocelot, 1.1)
AC_CONFIG_SRCDIR([src/ocelot.h])
AC_CONFIG_MACRO_DIRS([m4])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_CHECK_FUNCS([ftruncate memset socket strerror])

AM_INIT_AUTOMAKE([1.11 no-define foreign])
AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile])
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX(11, noext, mandatory)
AX_BOOST_BASE([1.37], [], [AC_MSG_ERROR(Need boost >= 1.37)])
AX_BOOST_IOSTREAMS
AX_BOOST_SYSTEM
AX_PTHREAD([], AC_MSG_FAILURE([pthread library is required]))
EV_DEVEL
MYSQL_C_API_LOCATION
MYSQLPP_DEVEL
TCMALLOC

AC_OUTPUT
