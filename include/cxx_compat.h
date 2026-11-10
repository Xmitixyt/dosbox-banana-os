#ifndef DOSBOX_BANANA_CXX_COMPAT_H
#define DOSBOX_BANANA_CXX_COMPAT_H
#include <locale.h>
#include <stdio.h>
#include <wchar.h>
typedef locale_t __locale_t;
extern "C" {
char* gets(char*);
char* tmpnam(char*);
int vscanf(const char*, va_list);
}
enum { _ISupper=0x100, _ISlower=0x200, _ISalpha=0x400, _ISdigit=0x800,
       _ISxdigit=0x1000, _ISspace=0x2000, _ISprint=0x4000, _ISgraph=0x8000,
       _ISblank=1, _IScntrl=2, _ISpunct=4, _ISalnum=8 };
#endif
