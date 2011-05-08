// allowbrk.h

#if defined( SYSTEM_DOS )

inline void allow_break()
{
    struct date dummy_date;  // FÅr Break-Gelegenheit
    getdate(&dummy_date);
}

#endif
