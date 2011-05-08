Attribute VB_Name = "HOSTAPI"
' Deklarationen der hostAPIL.dll für Visual Basic (Englisch)
' ©1997 SOS GmbH Berlin

Option Explicit
Declare Function sos_init Lib "hostAPIL.dll" () As Long
Declare Function sos_exit Lib "hostAPIL.dll" (ByVal hostapi_handle&) As Long
Declare Function sos_log Lib "hostAPIL.dll" (ByVal hostapi_handle&, ByVal filename$) As Long
Declare Function sos_get_exception_name Lib "hostAPIL.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_error_code Lib "hostAPIL.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_error_text Lib "hostAPIL.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_open Lib "hostAPIL.dll" (ByVal hostapi_handle&, ByVal filename$, ByVal mode&) As Long
Declare Function sos_close Lib "hostAPIL.dll" (ByVal file_handle&) As Long
Declare Function sos_set_key Lib "hostAPIL.dll" (ByVal file_handle&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_get Lib "hostAPIL.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_key Lib "hostAPIL.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_delete_key Lib "hostAPIL.dll" (ByVal file_handle&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_put Lib "hostAPIL.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_store Lib "hostAPIL.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_store_key Lib "hostAPIL.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_insert Lib "hostAPIL.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_update Lib "hostAPIL.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_field_count Lib "hostAPIL.dll" (ByVal file_handle&) As Long
Declare Function sos_get_field_name Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_number&, ByVal name_buffer$, ByVal name_buffer_size&) As Long
Declare Function sos_read_field_as_text Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$, ByVal buffer$, ByVal buffer_size$, ByVal flags&) As Long
Declare Function sos_read_field_as_int Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$) As Long
Declare Function sos_clear_record Lib "hostAPIL.dll" (ByVal file_handle&) As Long
Declare Function sos_write_field_text Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$, ByVal text$) As Long
Declare Function sos_write_field_int Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$, ByVal value&) As Long
Declare Function sos_clear_key Lib "hostAPIL.dll" (ByVal file_handle&) As Long
Declare Function sos_write_key_field_text Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$, ByVal text$) As Long
Declare Function sos_write_key_field_int Lib "hostAPIL.dll" (ByVal file_handle&, ByVal field_name$, ByVal value&) As Long
Declare Function sos_read_fields_as_rtf Lib "hostAPIL.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&, ByVal options$) As Long

Function sos_read_field(file As Long, field_name As String) As Variant

sos_read_field = sos_read_field_as_string(file, field_name)

End Function

Function sos_read_field_as_string$(file As Long, field_name As String)

Dim length As Long
Dim result As String

result = String$(1024, " ")
length = sos_read_field_as_text(file, field_name, result, Len(result), 0)

'if length < 0

If length >= 0 Then sos_read_field_as_string$ = Left$(result, length)

End Function

