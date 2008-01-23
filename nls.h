

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

# include <locale.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) (Text)
# define dgettext(Package, String) (String)
#endif

# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif 

