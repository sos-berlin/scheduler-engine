// $Id: log_categories.h 13725 2008-11-02 14:55:43Z jz $

#ifndef ZSCHIMMER_LOG_CATEGORIES_H
#define ZSCHIMMER_LOG_CATEGORIES_H

#include "mutex.h"
#include <iostream>


namespace zschimmer {

struct Log_categories;

//------------------------------------------------------------------------------Cached_log_category

struct Cached_log_category
{
                                Cached_log_category         ( const string& name );
                               ~Cached_log_category         ();

    friend ostream&             operator <<                 ( ostream& s, const Cached_log_category& c )  { s << c._name;  return s; }

  private:
    friend struct               Log_categories;

    string                     _name;
    int                        _last_modified_counter;
    bool                       _is_set;
    bool                       _is_valid;
};

//---------------------------------------------------------------------------Log_categories_content

struct Log_categories_content
{
    struct Entry
    {
        enum Type 
        { 
            e_standard,
            e_implicit,         // z.B. "a.b": a => a.b, a schaltet auch a.b an.
            e_explicit,         // z.B. "a.b": a.* schaltet nicht a.b an. a.b muss explizit gesetzt werden. Ebenso bei "all" (="*")
            e_derived           // Abgeleiteter, automatisch generierter Eintrag, zum schnelleren Nachschlagen und für fehlende Eltern
        };

        static string           type_name                   ( Type );


                                Entry                       ( Type t = e_standard, bool value = false ) : _zero_(this+1), _type(t), _value(value) {}


        Fill_zero              _zero_;
        bool                   _value;
        Type                   _type;

        int64                  _used_counter;
        int64                  _denied_counter;

        bool                   _children_too;               // Benutzer hat gesetzt, z.B. "a.*": alle Kinder, außer e_explicit, setzen
        bool                   _children_too_derived;
        bool                   _has_default;                // Default des Programms 
        bool                   _default_value;
    };

    typedef stdext::hash_map< string, Entry >  Map;


                                Log_categories_content      ()                                      : _zero_(this+1) {}

  protected:

    Fill_zero                  _zero_;
    Map                        _map;
    bool                       _really_all;
};

//-----------------------------------------------------------------------------------Log_categories

struct Log_categories : Log_categories_content
{
    static void                 self_test                   ();


                                Log_categories              ();
                               ~Log_categories              ()                                      {  _valid = false; }

    void                        save_to                     ( Log_categories_content* );
    void                        restore_from                ( const Log_categories_content& );

    void                        set_multiple                ( const string& names );
    void                        set                         ( const string& name );
    string                      set                         ( const string& name, bool value, Entry::Type = Entry::e_standard );
    void                        set_default                 ( const string& name, bool value );         // set_default("misc.java.call")                     => is_set("misc.java.call") == true
    void                        set_local_default           ( const string& name, bool value );         // set_local_default("misc.java.call")  && set("misc.java") => is_set("misc.java.call") == true

    bool                        is_set                      ( const string& name, bool is_derived = false );
    bool                        is_set                      ( const char* name );
    bool                        is_set_cached               ( Cached_log_category* );

    void                    set_all                         ( bool all )                                { _really_all = all;           _modified_counter++; }   // Für asynchrone Aufrufe (Signal-Handler)
    void                        toggle_all                  ()                                          { _really_all = !_really_all;  _modified_counter++; }   // Für asynchrone Aufrufe (Signal-Handler)

    int                         modified_counter            () const                                    { return _modified_counter; }
    bool                        is_set                      ( Cached_log_category* c )                  { if( c->_last_modified_counter != _modified_counter )  is_set_cached( c );  
                                                                                                          return c->_is_set; }
    
    bool                        category_is_relevant        ( const string& name ) const;
    Map                         map_copy                    () const;
    string                      to_string                   () const;
    string                      debug_string                () const;

  private:
    friend struct               Log_categories_content;

    bool                        is_set2                     ( const string& name, bool is_derived );
    void                        modify_children             ( const string& prefix, bool value, bool children_too );
    void                        generate_missing_anchestors_of( const string& );

    Fill_zero                  _zero_;
    bool                       _valid;
    int                        _modified_counter;
    mutable Mutex              _mutex;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
