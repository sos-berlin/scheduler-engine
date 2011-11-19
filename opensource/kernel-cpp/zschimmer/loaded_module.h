// $Id: loaded_module.h 11394 2005-04-03 08:30:29Z jz $

#ifndef __ZSCHIMMER_LOADED_MODULE
#define __ZSCHIMMER_LOADED_MODULE

namespace zschimmer {

//------------------------------------------------------------------------------------Loaded_module

struct Loaded_module : Object, Non_cloneable
{
/*
    struct Entry
    {
            Entry( const char* name );
            Entry( const string& name );
            Entry( const void* = NULL );

        Loaded_module          _loaded_module;
        string                 _name;
        void*                  _addr;
    };

    Loaded_module( const Entry[] entries )
    {
        for( const Entry& e = entries[0]; e->_addr | e->_name != ""; e++ )  add_entry( e );
    }
*/



                                Loaded_module               ( const string& filename = "" )         { set_filename( filename ); }
                               ~Loaded_module               ();


    void                    set_title                       ( const string& title )                 { _title = title; }

    void                        load                        ();
    void                        load                        ( const char* filename )                { set_filename( filename ); load(); }
    void                        load                        ( const string& filename )              { load( filename.c_str() ); }

    void                    set_filename                    ( const string& );
    string                      filename                    () const                                { return _filename; }

    void*                       addr_of                     ( const char* entry );
    void*                       addr_of                     ( const string& entry )                 { return addr_of( entry.c_str() ); } 

    void                        dont_unload                 ()                                      { _dont_unload = true; }

    friend ostream&             operator <<                 ( ostream& s, const Loaded_module& m )  { s << "Modul " << m._filename;  return s; }


  private:
    string                     _title;
    string                     _filename;
    bool                       _dont_unload;

#   ifdef Z_WINDOWS
                                operator HINSTANCE          ()                                      { return _handle; }
        HINSTANCE              _handle;
#    else
                                operator void*              ()                                      { return _handle; }
        void*                  _handle;
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
