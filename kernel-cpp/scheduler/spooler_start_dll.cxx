// $Id: spooler_start_dll.cxx 12116 2006-06-06 18:03:24Z jz $

#include "spooler.h"


//#pragma comment( lib, "scheduler" )


int main( int argc, char** argv )
{
    //sos::_argc = argc;
    //sos::_argv = argv;

    return spooler_program( argc, argv );
}
