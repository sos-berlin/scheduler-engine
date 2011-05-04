// sostypcd.h                                   (c) 1995 SOS GmbH Berlin

#ifndef __SOSTYPCD_H
#define __SOSTYPCD_H

namespace sos
{

/* Jede von Sos_object_base abgeleitete Klasse kann hier eingetragen werden.
*/

enum Sos_type_code
{
    tc_Sos_object_base          = 1,
    tc_Sos_self_deleting        = 2,
    tc_Sos_object               = 3,
    tc_Sos_msg_filter           = 4,
    tc_Sos_file                 = 5,
    tc_Field_type               = 6,
    tc_Has_name_file            = 7,
    tc_Method_type              = 8,
    tc_Void_method_type         = 9,
    tc_Record_type              = 10,
    tc_Array_type               = 11,
    tc_Field_descr              = 12,
    tc_Array_field_descr        = 13,
    tc_Oracle_date_type         = 14,
    tc_Sql_select               = 15,
    tc_Filter_file              = 16,
    tc_String0_type             = 17,
    tc_Tabbed_field_type        = 18,
    tc_Sql_expr                 = 19,
    tc_Abs_file                 = 20,
    tc_Sos_database_file        = 21,
    tc_factory_Text_parser      = 22,
    tc_factory_Rtf_parser       = 24,
    tc_Sql_expr_with_operands   = 25,
    tc_Field_subtype            = 26,

    tc_dummy_fuer_short__       = 0x7FFF
};


} //namespace sos

#endif
