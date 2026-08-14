#ifndef RAGE_MSVC_COMPAT_H
#define RAGE_MSVC_COMPAT_H

#ifdef _MSC_VER
/* Generated retail-state declarations use GNU alignment annotations.  The
 * host does not require a PS1 linker layout, so MSVC can safely omit them. */
#define __attribute__(x)
#endif

#endif
