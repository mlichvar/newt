

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

# include <locale.h>

# include <libintl.h>
# define _(Text) gettext (Text)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif 

