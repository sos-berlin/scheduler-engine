// $Id: spooler_common.h,v 1.14 2002/12/02 20:43:31 jz Exp $

#ifndef __SPOOLER_COMMON_H
#define __SPOOLER_COMMON_H

namespace sos {
namespace spooler {

#ifdef SYSTEM_WIN
#   define DIR_SEP "\\"
# else
#   define DIR_SEP "/"
#endif


typedef zschimmer::Thread::Id   Thread_id;                  // _beginthreadex()
typedef DWORD                   Process_id;

//-----------------------------------------------------------------------------------------FOR_EACH

#define        FOR_EACH(             TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator       ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )
#define        FOR_EACH_CONST(       TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (ITERATOR)++ )
//#define LOCKED_FOR_EACH(       LOCK, TYPE, CONTAINER, ITERATOR )  for( TYPE::iterator       ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (LOCK).enter(), (ITERATOR)++, (LOCK).leave() )
//#define LOCKED_FOR_EACH_CONST( LOCK, TYPE, CONTAINER, ITERATOR )  for( TYPE::const_iterator ITERATOR = (CONTAINER).begin(); (ITERATOR) != (CONTAINER).end(); (LOCK).enter(), (ITERATOR)++, (LOCK).leave() )

// Bei LOCKED_FOR_EACHxx() ist der Iterator durch eine Semaphore geschützt. 
// Damit kann einer Liste durchlaufen werden, der ein anderer Thread Elemente hinzufügt.
// Der andere Thread muss dieselbe Semaphore LOCK nutzen.

//-------------------------------------------------------------------------------------------Handle
/*
struct Handle : Sos_self_deleting
{
#   ifdef SYSTEM_WIN
                                Handle                      ( HANDLE h = NULL )             : _handle(h) {}
                                Handle                      ( ulong h )                     : _handle((HANDLE)h) {}     // für _beginthread()
                               ~Handle                      ()                              { close(); }

        void                    operator =                  ( HANDLE h )                    { set_handle( h ); }
        void                    operator =                  ( ulong h )                     { set_handle( (HANDLE)h ); }   // für _beginthread()
                                operator HANDLE             () const                        { return _handle; }
                                operator !                  () const                        { return _handle == 0; }
      //HANDLE*                 operator &                  ()                              { return &_handle; }

        void                    set_handle                  ( HANDLE h )                    { close(); _handle = h; }
        HANDLE                  handle                      () const                        { return _handle; }
        void                    close                       ()                              { if(_handle) { CloseHandle(_handle); _handle=0; } }

        HANDLE                 _handle;

  private:
                                Handle                      ( const Handle& );              // Nicht implementiert
    void                        operator =                  ( const Handle& );              // Nicht implementiert

#   endif
};
*/
//static HANDLE null_handle = NULL;

//-------------------------------------------------------------------------------------------Atomic
/*
template<typename T>
struct Atomic
{
    typedef sos::Thread_semaphore::Guard Guard;


                                Atomic                      ( const T& t = T() )    : _value(t) {}

    Atomic&                     operator =                  ( const T& t )          { Guard g = &_lock; ref() = t; return *this; }
                                operator T                  ()                      { Guard g = &_lock; return ref(); }
    T                           read_and_reset              ()                      { return read_and_set( T() ); }
    T                           read_and_set                ( const T& t )          { Guard g = &_lock; T v = _value; _value = t; return v; }
    T&                          ref                         ()                      { return ref(); }

    volatile T                 _value;
    sos::Thread_semaphore      _lock;
};

//------------------------------------------------------------------------------------Simple_atomic
// Für Typen, die atomar lesbar und schreibbar sind.
// Erforderliche Operationen:
// T&                           operator =                  ( const T& ) atomic
//                              operator T                  () atomic
// bool                         operator ==                 ( const T& ) atomic

template<typename T>
struct Simple_atomic
{
    typedef sos::Thread_semaphore::Guard Guard;


                                Simple_atomic               ( const T& t = T() )    : _value(t) {}

    Simple_atomic&              operator =                  ( const T& t )          { _value = t; return *this; }
                                operator T                  ()                      { return _value; }

    T                           read_and_reset              ()                      { return read_and_set( T() ); }
    T                           read_and_set                ( const T& t )          { if( _value == t )  return _value;  Guard g = &_lock; T v = _value; _value = t; return v; }

    volatile T                 _value;
    sos::Thread_semaphore      _lock;
};
*/
//------------------------------------------------------------------------------------threaded_list
/*
template< class T >
struct threaded_list : list<T>
{
    typedef Thread_semaphore::Guard G;


    explicit                    threaded_list               ()   {}
    explicit                    threaded_list               ( size_type n, const T& v = T(), ) : list(n,v) {}
    iterator                    begin                       ();
    const_iterator              begin                       () const;
    iterator                    end                         ();
    iterator                    end                         () const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    void resize(size_type n, T x = T());
    size_type size() const;
    size_type max_size() const;
    bool empty() const;
    A get_allocator() const;
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;
    void push_front(const T& x);
    void pop_front();
    void push_back(const T& x);
    void pop_back();
    void assign(const_iterator first, const_iterator last);
    void assign(size_type n, const T& x = T());
    iterator insert(iterator it, const T& x = T());
    void insert(iterator it, size_type n, const T& x);
    void insert(iterator it,
        const_iterator first, const_iterator last);
    void insert(iterator it,
        const T *first, const T *last);
    iterator erase(iterator it);
    iterator erase(iterator first, iterator last);
    void clear();
    void swap(threaded_list x);
    void splice(iterator it, threaded_list& x);
    void splice(iterator it, threaded_list& x, iterator first);
    void splice(iterator it, threaded_list& x, iterator first, iterator last);
    void remove(const T& x);
    void remove_if(binder2nd<not_equal_to<T> > pr);
    void unique();
    void unique(not_equal_to<T> pr);
    void merge(threaded_list& x);
    void merge(threaded_list& x, greater<T> pr);
    void sort();
    template<class Pred>
        void sort(greater<T> pr);
    void reverse();


    Thread_semaphore           _lock;

  private:
                                threaded_list               ( const threaded_list& x );
                                threaded_list               ( const_iterator first, const_iterator last, const A& al = A() );
};
*/
/*
template<class T, class A> bool operator== ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> bool operator!= ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> bool operator<  ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> bool operator>  ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> bool operator<= ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> bool operator>= ( const list<T, A>& lhs, const list<T, A>& rhs );
template<class T, class A> void swap       ( const list<T, A>& lhs, const list<T, A>& rhs );
*/

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
