// zschimmer.h                                      ©2000 Joacim Zschimmer
// $Id: not_in_recursion.h 13691 2008-09-30 20:42:20Z jz $

#ifndef __ZSCHIMMER_NOT_IN_RECURSION_H
#define __ZSCHIMMER_NOT_IN_RECURSION_H

#include "log.h"

namespace zschimmer {
          
//--------------------------------------------------------------------------------Recursion_count
    
//struct Recursion_count
//{
//                                Prefix_log_deny_recursion   ( int* count_ptr )                    : _count_ptr( count_ptr ) { (*count_ptr)++; }
//                               ~Prefix_log_deny_recursion   ()                                      { _count_ptr--; }
//
//                                operator const int          ()                                      { return *_count_ptr; }
//
//    int* const                 _count_ptr;
//};

//-------------------------------------------------------------------------------------In_recursion
    
struct Not_in_recursion : Non_cloneable
{
    Not_in_recursion( int* recursion_count_ptr )
    :
        _previous_recursion_count( (*recursion_count_ptr)++ ),
        _recursion_count_ptr( recursion_count_ptr )
    { 
        if( _previous_recursion_count )  Z_LOG2( "zschimmer", "***" << Z_FUNCTION << "  In recursion ***\n" );
    }

    Not_in_recursion( const Not_in_recursion& r )
    :
        _previous_recursion_count( r._previous_recursion_count ),
        _recursion_count_ptr( NULL )
    {
    }

    ~Not_in_recursion()
    { 
        release();
    }

    void release()
    {
        if( _recursion_count_ptr )  (*_recursion_count_ptr)--, _recursion_count_ptr = NULL;
    }

    operator bool()
    { 
        return _previous_recursion_count == 0;
    }


  private:
    int  const _previous_recursion_count;
    int*       _recursion_count_ptr;
};

//
// Funktioniert nicht mit gcc 3.4.3, weil der einen Kopierkonstruktor verlangt: if( Not_in_recursion n = &x )   

//struct Not_in_recursion : Non_cloneable
//{
//    Not_in_recursion( int* recursion_count_ptr )
//    :
//        _recursion_count_ptr( recursion_count_ptr ) 
//    { 
//        (*recursion_count_ptr)++;
//    }
//
//    ~Not_in_recursion()
//    { 
//        (*_recursion_count_ptr)--;
//    }
//
//    operator bool()
//    { 
//        return *_recursion_count_ptr == 1;
//    }
//
//
//  private:
//    int* const _recursion_count_ptr;
//};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif


