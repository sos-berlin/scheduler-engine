/* rotbar.h					(c) SOS GmbH Berlin
						Joacim Zschimmer
*/


#if !defined( SYSTEM_WIN )

static Byte _rotbar = 0;
static Byte _rotbar_chars [4] = {'|','/','-','\\'};

#define _rotate_bar()  {                                                      \
                        putc (_rotbar_chars [_rotbar++ & 3], stderr);         \
                        putc (8, stderr);                                     \
                       }

#else

inline void _rotate_bar() {}

#endif