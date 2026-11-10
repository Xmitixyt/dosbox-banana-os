#ifndef BANANA_DOSBOX_LIBINTL_H
#define BANANA_DOSBOX_LIBINTL_H
extern "C" {
char *gettext(const char *);
char *dgettext(const char *, const char *);
char *dcgettext(const char *, const char *, int);
char *bindtextdomain(const char *, const char *);
char *textdomain(const char *);
}
#endif
