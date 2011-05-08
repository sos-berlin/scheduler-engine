// Um Absturz bei delete auf ein StarView-Objekt zu vermeiden:

#if defined __BORLANDC__

#if defined __WIN32__

#if 0
unsigned int __GetPolymorphicDTC( void*, unsigned int )
{
    return 0;
}
#endif

#else

unsigned int __GetPolymorphicDTCfar( void*, unsigned int )
{
    return 0;
}

#endif

#endif
