// $Id: spooler_start_dll.cxx,v 1.2 2003/11/30 10:35:38 jz Exp $

#include "spooler.h"


int main( int argc, char** argv )
{
    sos::_argc = argc;
    sos::_argv = argv;

    return spooler_program( argc, argv );
}
