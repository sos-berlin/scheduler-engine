#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//-----------------------------------------------------------------------Dependencies::Dependencies

Dependencies::Dependencies( File_based_subsystem* subsystem )
:
    _zero_(this+1),
    _subsystem( subsystem )
{
}

//----------------------------------------------------------------------Dependencies::~Dependencies
    
Dependencies::~Dependencies()
{
}

//----------------------------------------------------------------------Dependencies::add_requisite

void Dependencies::add_requisite( Dependant* dependant, const string& missings_path )
{
    Z_LOG2("JS-644", obj_name() << "::" << Z_FUNCTION << " " << dependant->obj_name() << " '" << missings_path << "'\n");
    _path_requestors_map[ _subsystem->normalized_path( missings_path ) ].insert( dependant );
}

//-------------------------------------------------------------------Dependencies::remove_requisite

void Dependencies::remove_requisite( Dependant* dependant, const string& missings_path )
{
    Z_LOG2("JS-644", obj_name() << "::" << Z_FUNCTION << " '" << missings_path << "'\n");
    Requestor_set&  requestors_set = _path_requestors_map[ missings_path ];
    
    requestors_set.erase( dependant );
    if( requestors_set.empty() )  _path_requestors_map.erase( _subsystem->normalized_path( missings_path ) );
}

//----------------------------------------------------------Dependencies::announce_requisite_loaded

void Dependencies::announce_requisite_loaded( File_based* found_missing )
{
    Z_LOG2("JS-644", obj_name() << "::" << Z_FUNCTION << " " << found_missing->obj_name() << "\n");
    assert( found_missing->subsystem() == _subsystem );

    Path_requestors_map::iterator it = _path_requestors_map.find( found_missing->normalized_path() );

    if( it != _path_requestors_map.end() ) {
        Requestor_set& requestor_set = it->second;

        Z_FOR_EACH( Requestor_set, requestor_set, it2 ) {
            Requestor_set::iterator next_it2  = it2;  next_it2++;
            Dependant*              dependant = *it2;
        
            try {
                Z_LOG2("JS-644", Z_FUNCTION << " " << dependant->obj_name() << "->on_requisite_loaded()\n");
                dependant->on_requisite_loaded( found_missing );
            }
            catch( exception& x ) {
                string m = message_string("SCHEDULER-459", found_missing->obj_name(), x.what());
                Z_LOG2("JS-644", "ERROR " << Z_FUNCTION << " " << m << "\n");
                dependant->log()->error(m);
                found_missing->log()->error(m);
            }
        }
    }
}

//---------------------------------------------------Dependencies::announce_requisite_to_be_removed

bool Dependencies::announce_requisite_to_be_removed( File_based* to_be_removed )
{
    assert( to_be_removed->subsystem() == _subsystem );
    Z_LOG2("JS-644", obj_name() << "::" << Z_FUNCTION << " " << to_be_removed->obj_name() << "\n");

    bool result = true;

    Path_requestors_map::iterator it = _path_requestors_map.find( to_be_removed->normalized_path() );

    if( it != _path_requestors_map.end() ) {
        Requestor_set& requestor_set = it->second;

        Z_FOR_EACH( Requestor_set, requestor_set, it2 ) {
            Requestor_set::iterator next_it2  = it2;  next_it2++;
            Dependant*              dependant = *it2;
        
            Z_DEBUG_ONLY(Z_LOG2("JS-644", Z_FUNCTION << " " << dependant->obj_name() << "->on_requisite_to_be_removed( " << to_be_removed->obj_name() << " )\n" ));
            result = dependant->on_requisite_to_be_removed( to_be_removed );
            if( !result )  break;
        }
    }

    return result;
}

//---------------------------------------------------------Dependencies::announce_requisite_removed

void Dependencies::announce_requisite_removed( File_based* to_be_removed )
{
    assert( to_be_removed->subsystem() == _subsystem );

    Path_requestors_map::iterator it = _path_requestors_map.find( to_be_removed->normalized_path() );

    if( it != _path_requestors_map.end() )
    {
        Requestor_set& requestor_set = it->second;

        Z_FOR_EACH( Requestor_set, requestor_set, it2 )
        {
            Requestor_set::iterator next_it2  = it2;  next_it2++;
            Dependant*              dependant = *it2;
        
            Z_DEBUG_ONLY(Z_LOG2("JS-644", obj_name() << "::" << Z_FUNCTION << " " << dependant->obj_name() << " on_requisite_to_be_removed( " << to_be_removed->obj_name() << " )\n"));
            dependant->on_requisite_removed( to_be_removed );
        }
    }
}

//---------------------------------------------------------------------------Dependencies::obj_name

string Dependencies::obj_name() const {
    return S() << "Dependencies(" << _subsystem->obj_name() << ")";
}

}}} //namespace sos::scheduler::folder
