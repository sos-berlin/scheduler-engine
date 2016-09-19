#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {


//-----------------------------------------------------------------------------Dependant::Dependant

Dependant::Dependant()
{
}

//----------------------------------------------------------------------------Dependant::~Dependant

Dependant::~Dependant()
{
    try
    {
        remove_requisites();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//---------------------------------------------------------------------Dependant::remove_requisites

void Dependant::remove_requisites()
{
    Z_FOR_EACH( Requisite_sets, _requisite_sets, it )
    {
        File_based_subsystem* subsystem      = it->first;
        Requisite_set&        requisite_path = it->second;

        Z_FOR_EACH( Requisite_set, requisite_path, it2 )
        {
            subsystem->dependencies()->remove_requisite( this, *it2 );
        }
    }
}

//-------------------------------------------------------------------------Dependant::add_requisite

void Dependant::add_requisite( const Requisite_path& r )
{
    if (const File_based* f = dynamic_cast<const File_based*>(this)) {
        if (f->subsystem() == r._subsystem && f->normalized_path() == f->subsystem()->normalized_path(r._path))
            z::throw_xc("SCHEDULER-486", f->obj_name());
    }

    _requisite_sets[ r._subsystem ].insert( r._subsystem->normalized_path( r._path ) );
    r._subsystem->dependencies()->add_requisite( this, r._path );
}

//----------------------------------------------------------------------Dependant::remove_requisite

void Dependant::remove_requisite( const Requisite_path& r )
{
    _requisite_sets[ r._subsystem ].erase( r._subsystem->normalized_path( r._path ) );
    r._subsystem->dependencies()->remove_requisite( this, r._path );
}

//---------------------------------------------------------------Dependant::requisite_is_registered

bool Dependant::requisite_is_registered( const Requisite_path& r )
{
    bool result = false;

    const Requisite_sets::const_iterator it = _requisite_sets.find( r._subsystem );
    if( it != _requisite_sets.end() )  
    {
        result = it->second.find(r._subsystem->normalized_path(r._path)) != it->second.end();
    }
    
    return result;
}

//------------------------------------------------------------Dependant::on_requisite_to_be_removed

bool Dependant::on_requisite_to_be_removed( File_based* )
{
    return false;
}

//------------------------------------------------------------------Dependant::on_requisite_removed

void Dependant::on_requisite_removed( File_based* )
{
    // Noch nicht verallgemeinern:  if( _state == s_active )  set_file_based_state( s_incomplete );    
}

//--------------------------------------------------------------------Dependant::missing_requisites

vector<string> Dependant::missing_requisites_java() {
    list<Requisite_path> requisite_paths = missing_requisites();
    vector<string> result;
    result.reserve(requisite_paths.size());
    Z_FOR_EACH_CONST(list<Requisite_path>, requisite_paths, i) {
        string s;
        s.reserve(30 + i->_path.size());
        s.append(i->_subsystem->xml_element_name());
        s.append(":");
        s.append(i->_path);
        result.push_back(s);
    }
    return result;
}

list<Requisite_path> Dependant::missing_requisites()
{
    list<Requisite_path> result;

    Z_FOR_EACH_CONST( Requisite_sets, _requisite_sets, ds )
    {
        File_based_subsystem* subsystem = ds->first;

        Z_FOR_EACH_CONST(Requisite_set, ds->second, d) {
            Absolute_path path = *d;
            File_based* requisite = subsystem->file_based_or_null(path);
            if (requisite) {
                result.splice(result.end(), requisite->missing_requisites());
            }
            if (!requisite || !requisite->is_active_and_not_to_be_removed()) {
                result.push_back(Requisite_path(subsystem, path));
            }
        }
    }

    Z_FOR_EACH_CONST( stdext::hash_set<Dependant*>, _accompanying_dependants, p )
    {
        result.splice(result.end(), (*p)->missing_requisites());
    }

    return result;
}

//------------------------------------------------------------Dependant::add_accompanying_dependant

void Dependant::add_accompanying_dependant( Dependant* d )
{ 
    _accompanying_dependants.insert( d ); 
}

//---------------------------------------------------------Depandant::remove_accompanying_dependant

void Dependant::remove_accompanying_dependant( Dependant* d )
{ 
    _accompanying_dependants.erase( d ); 
}

//-----------------------------------------------------------------Dependant::requistes_dom_element

//xml::Element_ptr Dependant::requistes_dom_element( const xml::Document_ptr& document, const Show_what& )
//{
//    xml::Element_ptr result = document.create_element( "requisites" );
//
//    append_requisite_dom_elements( result );
//
//    return result;
//}

//---------------------------------------------------------Dependant::append_requisite_dom_elements

int Dependant::append_requisite_dom_elements( const xml::Element_ptr& element, int nesting)
{
    int result = 0;

    if (nesting < 10) {  // Einfache Sperre gegen zikuläre Abhängigkeit
        Z_FOR_EACH_CONST( Requisite_sets, _requisite_sets, ds )  
        {
            File_based_subsystem* subsystem = ds->first;  // ->first = Key ist der Zeiger auf das Subsystem

            Z_FOR_EACH_CONST( Requisite_set, ds->second, d ) // ->second = Values = Liste der Pfade zu den Requisiten??
            {
                Absolute_path path      ( *d );
                File_based*   requisite = subsystem->file_based_or_null( path );

                // JS-775: default process_class should not displayed
                if (requisite && requisite->is_visible_requisite()) {
                    xml::Element_ptr e = element.append_new_element( "requisite" );
                    e.setAttribute( "type", subsystem->object_type_name() );
                    e.setAttribute( "path", path );
                    result++;
                    // Auch indirekte Requisiten (möglicherweise doppelt, könnten identifiziert werden)
                    result += requisite->append_requisite_dom_elements(element, nesting + 1);
                }
                else
                if (!requisite || !requisite->is_active_and_not_to_be_removed()) {
                    xml::Element_ptr e = element.append_new_element("requisite");
                    e.setAttribute("type", subsystem->object_type_name());
                    e.setAttribute("path", path);
                    e.setAttribute("is_missing", "yes");
                    result++;
                }
            }
        }

        Z_FOR_EACH_CONST( stdext::hash_set<Dependant*>, _accompanying_dependants, p )
        {
            result += (*p)->append_requisite_dom_elements(element, nesting + 1);
        }
    }

    return result;
}

}}} //namespace sos::scheduler::folder
