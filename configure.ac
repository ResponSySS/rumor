
AC_INIT(Rumor,1.0.4,[Václav Šmilauer <eu@doxos.eu>],rumor)
AM_INIT_AUTOMAKE([dist-bzip2 no-texinfo.tex])
AM_CONFIG_HEADER([config.h])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_PROG_CXX
#AC_CANONICAL_HOST
AC_LANG_CPLUSPLUS

## REQUIRED HEADERS
##############################
AC_CHECK_HEADERS([sstream argp.h],,[AC_MSG_ERROR([This header is needed. Bye.])])

### I18n
##############################
#AM_GNU_GETTEXT([external])

## PTHREADS
##############################
AC_CHECK_HEADER([pthread.h])
AC_CHECK_LIB([pthread],[pthread_create])
if test "$ac_cv_header_pthread_h" != yes -o \
        "$ac_cv_lib_pthread_pthread_create" != yes; then
  AC_MSG_ERROR([POSIX threads (pthreads) not working. Bye.])
else
  # this is apparently needed for some versions of autoconf/automake
  LIBS="-lpthread $LIBS"
  CXXFLAGS="$CXXFLAGS -D_REENTRANT"
fi


rumor_alsa=no
## ALSA
##############################
AC_CHECK_HEADER([alsa/asoundlib.h])
AC_CHECK_LIB([asound],[snd_seq_open])
if test "$ac_cv_header_alsa_asoundlib_h" = "yes" \
      -a "$ac_cv_lib_asound_snd_seq_open" = "yes"; then
  AC_DEFINE(HAVE_ALSA,,[ALSA is usable (for MIDI I/O)])
  LIBS="-lasound $LIBS"
  rumor_alsa=yes
fi

rumor_oss=no
## OSS
##############################
AC_CHECK_HEADER([linux/soundcard.h])
if test "$ac_cv_header_linux_soundcard_h" = yes; then
  AC_DEFINE(HAVE_OSS,,[OSS is usable (for MIDI I/O)])
  rumor_oss=yes
fi

rumor_guile=no
## GUILE
##############################
AC_CHECK_HEADER([guile/gh.h])
AC_CHECK_LIB([guile],[gh_enter])
if test "$ac_cv_header_guile_gh_h" = "yes" \
      -a "$ac_cv_lib_guile_gh_enter" = "yes"; then
  AC_DEFINE(HAVE_GUILE,,[guile is usable (for scripting)])
  LIBS="-lguile $LIBS"
  rumor_guile=yes
fi


## DEBUG (default: enabled)
##############################
AC_ARG_ENABLE(debug,[  --disable-debug         disable debug build])
if test "x$enable_debug" = x; then enable_debug="yes"; fi
if test "$enable_debug" = "yes"; then
  AC_DEFINE([DEBUG],[],[build with verbose runtime logging and debug information])
  CXXFLAGS="$CXXFLAGS -ggdb3 -O0 -Wall -pedantic"  
else
  enable_debug=no
  CXXFLAGS="$CXXFLAGS -g0 -O2"
fi;


AC_MSG_RESULT([
----------------------------------------
      Rumor feature summary
----------------------------------------
ALSA support             $rumor_alsa
OSS support              $rumor_oss
guile scripting support  $rumor_guile
debug build              $enable_debug
----------------------------------------
])

if test "x$rumor_alsa" = xno -a "x$rumor_oss" = xno ; then
  AC_MSG_ERROR([
There is neither ALSA nor OSS interface. Even though you could
play your computer keyboard (see --kbd), you would not hear
anything, not even metronome. For now, this an error.])
fi

AC_OUTPUT
