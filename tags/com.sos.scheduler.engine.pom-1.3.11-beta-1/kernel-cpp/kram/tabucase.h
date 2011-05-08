// tabucase.h                   J. Zschimmer,  ©1995 SOS GmbH Berlin

// Für Modul sosctype.cxx

#ifndef __TABUCASE_H
#define __TABUCASE_H

namespace sos
{

extern char tabucase_[];
extern char tablcase_[];

static const char* tabucase = tabucase_;
static const char* tablcase = tablcase_;

inline char sos_tolower ( char c )  { return tablcase[ (unsigned)c ]; }
inline char sos_toupper ( char c )  { return tabucase[ (unsigned)c ]; }


} //namespace sos

#endif
