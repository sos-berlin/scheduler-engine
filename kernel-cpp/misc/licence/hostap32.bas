Attribute VB_Name = "HOSTAP32"
Option Explicit

' Deklarationen der hostap32l.dll für Visual Basic (Englisch)
' ©1997 SOS GmbH Berlin

Declare Function sos_init Lib "hostap32l.dll" () As Long
Declare Function sos_exit Lib "hostap32l.dll" (ByVal hostapi_handle&) As Long
Declare Function sos_log Lib "hostap32l.dll" (ByVal hostapi_handle&, ByVal filename$) As Long
Declare Function sos_get_exception_name Lib "hostap32l.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_error_code Lib "hostap32l.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_error_text Lib "hostap32l.dll" (ByVal hostapi_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_open Lib "hostap32l.dll" (ByVal hostapi_handle&, ByVal filename$, ByVal mode&) As Long
Declare Function sos_close Lib "hostap32l.dll" (ByVal file_handle&) As Long
Declare Function sos_set_key Lib "hostap32l.dll" (ByVal file_handle&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_get Lib "hostap32l.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&) As Long
Declare Function sos_get_key Lib "hostap32l.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_delete_key Lib "hostap32l.dll" (ByVal file_handle&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_put Lib "hostap32l.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_store Lib "hostap32l.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_store_key Lib "hostap32l.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&, ByVal key$, ByVal key_length&) As Long
Declare Function sos_insert Lib "hostap32l.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_update Lib "hostap32l.dll" (ByVal file_handle&, ByVal record$, ByVal record_length&) As Long
Declare Function sos_field_count Lib "hostap32l.dll" (ByVal file_handle&) As Long
Declare Function sos_get_field_name Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_number&, ByVal name_buffer$, ByVal name_buffer_size&) As Long
Declare Function sos_read_field_as_text Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$, ByVal buffer$, ByVal buffer_size$, ByVal flags&) As Long
Declare Function sos_read_field_as_int Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$) As Long
Declare Function sos_clear_record Lib "hostap32l.dll" (ByVal file_handle&) As Long
Declare Function sos_write_field_text Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$, ByVal text$) As Long
Declare Function sos_write_field_int Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$, ByVal value&) As Long
Declare Function sos_clear_key Lib "hostap32l.dll" (ByVal file_handle&) As Long
Declare Function sos_write_key_field_text Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$, ByVal text$) As Long
Declare Function sos_write_key_field_int Lib "hostap32l.dll" (ByVal file_handle&, ByVal field_name$, ByVal value&) As Long
Declare Function sos_read_fields_as_rtf Lib "hostap32l.dll" (ByVal file_handle&, ByVal buffer$, ByVal buffer_size&, ByVal options$) As Long


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

