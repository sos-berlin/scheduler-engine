#define MODULE_NAME "sosid"
#define COPYRIGHT   "© SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#if defined JZ_TEST

/* Ein eigener Zeiger-Typ Sos_id wie in Rapid.
   Für automatische Garbage Collection.

   Kann für persistente Objekte erweitert werden:
      Sos_id_as<T>::store( Sos_storage_location* )

      mit Sos_storage_location::Sos_storage_location( Sos_file*, Key* );

      Das Sos_register verwaltet dann den Ort und lädt das Objekt automatisch bei Bedarf.


   Ein Objekt kann als verwerfbar gekennzeichnet werden. Es wird dann bei Speichermangel gelöscht
   und wieder automatisch angelegt. Für Puffer (eine Eigenschaft von Area? Klasse Discardable).
*/

typedef int Sos_id_index;


struct Sos_id
{
                                 Sos_id                 ( void* );
                                 Sos_id                 ( const Sos_id& );
                                ~Sos_id                 ();

    T*                           ptr                    () const;

  private:
    void                         operator =             ( int );            // Nicht implementiert

    Sos_id_index                _index;
};


template< class T >
struct Sos_id_as<T> : Sos_id
{
                                 Sos_id                 ( T* );
    T*                           operator ->            () const;
    T&                           operator *             () const;
    T*                           ptr                    () const;
};


struct Sos_register
{
    void*                        ptr                    ( const Sos_id& );

  private:
    friend                       Sos_id::Sos_id         ( void* );

    int                          register               ( void* );
    void                         incr_ref               ( Sos_id_index );
    void                         decr_ref               ( Sos_id_index );   // Löscht bei 0
};

Sos_register sos_register;


inline Sos_id::Sos_id( void* ptr )
{
    _index = sos_register.register( ptr );
}

inline Sos_id::Sos_id( const Sos_id& id )
{
    _index = id._index;
    sos_register.incr_ref( _index );
}

inline Sos_id::~Sos_id()
{
    sos_register.decr_ref( _index );
}

inline void* Sos_id::ptr()
{
    return sos_register.ptr( _index );
}

template< class T >
inline Sos_id_as<T>::Sos_id_as( T* object_ptr )
:
    Sos_id( object_ptr );
{
}

template< class T >
inline Sos_id_as<T>::operator-> ()
{
    return (T*)ptr();
}

template< class T >
inline T& Sos_id_as<T>::operator* ()
{
    return *(T*)ptr();
}



#endif
