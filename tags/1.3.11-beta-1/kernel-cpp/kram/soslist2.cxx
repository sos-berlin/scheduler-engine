#include <precomp.h>
#define MODULE_NAME "soslist2"
/* soslist.cpp                                          (c) SOS GmbH Berlin
                                                        Joacim Zschimmer
*/

#if 0

#include <sos.h>
#include <xception.h>
#include <soslist.h>

//------------------------------------------------------------Sos_simple_list_0::~Sos_simple_list_0

Sos_simple_list_0::~Sos_simple_list_0()
{
    Sos_simple_list_0_elem* p = elem_ptr();

    while( p ) {
        Sos_simple_list_0_elem* p2 = p->tail().elem_ptr();
        p->_tail._elem_ptr = 0;
        if( --p2->_ref_count ) {
            delete p;
        }    
        p = p2;
    }
}

//------------------------------------------------------------------------Sos_simple_list_0::length

int Sos_simple_list_0::length() const
{
    int                 n = 0;
    Sos_simple_list_0   p = *this;

    while( !p.empty() ) {
        n++;
        p = p->tail();
    }

    return n;
}

//---------------------------------------------------------------------Sos_simple_list_0_elem::last

Sos_simple_list_0 Sos_simple_list_0_elem::last()
{
    Sos_simple_list_0 p = Sos_simple_list_0( this );

    while( !p->tail().empty() ) {
        p = p->tail();
    }

    return p;
}

//------------------------------------------------------------------------Sos_simple_list<T>::store

template< class T >
void Sos_simple_list<T>::store( ostream& s )
{
    uint4 count = length();
    s << (Byte) (count >> 24)
      << (Byte) (count >> 16)
      << (Byte) (count >> 8)
      << (Byte)  count;

    const Sos_simple_list<T> l = this;

    while( !l.empty() ) {
        l->head_ptr()->store( s );
        l = l->tail();
    }
}

//-------------------------------------------------------------------------Sos_simple_list<T>::load

template< class T >
Sos_simple_list<T>& Sos_simple_list<T>::load( istream& s )
{
    uint4 count = ( (uint4)s.get() << 24 )
                | ( (uint4)s.get() << 16 )
                | ( (uint4)s.get() <<  8 )
                |   (uint4)s.get();

    if( s.fail() )  count = 0;

    Sos_simple_list<T> l = *this;

    while( count ) {
        l.append( Sos_simple_list<T>( T().load( s ), 0 ) );
        count--;
    }

    return *this;
}


#endif
