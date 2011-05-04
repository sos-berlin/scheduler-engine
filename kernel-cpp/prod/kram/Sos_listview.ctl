VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.UserControl Sos_listview 
   ClientHeight    =   3600
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   4800
   ScaleHeight     =   3600
   ScaleWidth      =   4800
   Begin ComctlLib.ListView ListView1 
      Height          =   3615
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   4815
      _ExtentX        =   8493
      _ExtentY        =   6376
      LabelWrap       =   -1  'True
      HideSelection   =   0   'False
      _Version        =   327682
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   0
   End
End
Attribute VB_Name = "Sos_listview"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Event modified()
Event RowColChange(LastRow As Variant, ByVal LastCol As Integer)

Enum ListView_flags
    listview_not_null = 2               'Das Feld muss ausgefüllt sein
End Enum

Private Type Column_descr
    ctrl                    As control
    bool                    As Boolean
    bool_reverse            As Boolean
    'default                 As Variant
End Type

Private Type Row_descr
    tagg                    As Variant
    error                   As bookmark
End Type

Public ctrls                As Control_object_link
Public coll                 As Collection
Public controls_frame       As frame
Public add_new_allowed      As Boolean


Private empty_record        As hostWare.Dyn_obj     'Muster für neue Einträge mit korrektem Typ und Defaults
Private column_array()      As Column_descr
Private current_row         As Variant              'Long oder Null
Private current_col         As Long
Private in_write_control    As Boolean
Private in_read_control     As Boolean
Private add_mode            As Boolean
'Private col_idx()           As Integer                      'obj_field_index -> subitem-index
Private row_array()         As Row_descr

Private Const bool_width = 200  'Breite einer Bool-Spalte


Public Sub clear()

ListView1.ListItems.clear
ListView1.View = lvwReport
ListView1.LabelEdit = lvwManual
ListView1.Refresh

current_row = Null
current_col = -1

Set coll = Nothing
End Sub

Public Property Get ActiveControl() As control

Set ActiveControl = UserControl.ActiveControl

End Property

Public Property Get listview() As listview

Set listview = ListView1

End Property


Public Property Get row() As Variant

If ListView1.SelectedItem Is Nothing Then
    row = Null
 Else
    row = ListView1.SelectedItem.Index
    If add_new_allowed And row = ListView1.ListItems.count Then row = Null
End If
    
End Property


Public Property Let row(ByVal r As Variant)

If IsNull(r) Then
    Set ListView1.SelectedItem = Nothing
 Else
    Set ListView1.SelectedItem = ListView1.ListItems.item(r)
End If

cell_selected -1

End Property


Public Property Get in_new_row() As Boolean

in_new_row = False
If Not add_new_allowed Then Exit Property
If ListView1.SelectedItem Is Nothing Then Exit Property
If ListView1.SelectedItem.Index < ListView1.ListItems.count Then Exit Property
in_new_row = True

End Property


Public Property Get col_count() As Long

col_count = ListView1.ColumnHeaders.count

End Property


Public Function col_index(ByVal name As Variant) As Long

If IsNumeric(name) Then col_index = name: Exit Function

col_index = empty_record.obj_field_index(name)

End Function


Public Sub load_file(ByVal filename As String)

Dim i       As Long
Dim n       As Long
Dim file    As New hostWare.file
Dim mtrec   As hostWare.Dyn_obj


Set coll = New Collection

file.open "-in -seq " & filename

n = file.field_count

'ReDim col_idx(0 To n - 1)

If ListView1.ColumnHeaders.count = 0 Then
    For i = 0 To n - 1
         ListView1.ColumnHeaders.add , LCase$(file.field_name(i)), file.field_name(i)
    Next i
End If

'For i = 0 To n - 1
'    col_idx(i) = ListView1.ColumnHeaders(LCase$(file.field_name(i))).SubItemIndex
'Next i



Set mtrec = collection_load_file(coll, file)
If empty_record Is Nothing Then Set empty_record = mtrec    'Nur das erste Mal, damit später die Defaults erhalten bleiben. jz 8.7.98

current_row = Null
current_col = -1
Set ListView1.SelectedItem = Nothing

ReDim Preserve column_array(0 To empty_record.obj_field_count - 1)

End Sub


Public Sub add_records(ByVal filename As String)

Dim file    As New hostWare.file
Dim record  As hostWare.Dyn_obj
Dim rec     As hostWare.Dyn_obj
Dim i       As Long

file.open "-in -seq " & filename
While Not file.EOF
    Set record = file.Get()
    Set rec = empty_record.clone()
    
    For i = 0 To record.obj_field_count - 1
        rec(record.obj_field_name(i)) = record(i)
    Next i
    add_record rec
Wend
file.Close

End Sub


Public Sub add_record(ByVal record As hostWare.Dyn_obj)

On Error GoTo FEHLER

add_mode = True

coll.add record
rebind

'If Not tag_coll Is Nothing Then tag_coll.add Nothing
ReDim Preserve row_array(1 To coll.count)

'check_row obj_coll.count

add_mode = False
Exit Sub

FEHLER:
    add_mode = False
    raise_again

End Sub


Public Property Let col_control(ByVal col As Variant, ctrl As control)

Set col_control(col) = ctrl

End Property


Public Property Set col_control(ByVal col As Variant, ctrl As control)
'Hängt ein Detail-Control an eine Spalte

If VarType(col) = vbString Then col = col_index(col)
Set column_array(col).ctrl = ctrl

If TypeName(ctrl) = "CheckBox" Then column_array(col).bool = True

write_controls

End Property


Public Property Let col_title(ByVal col, ByVal title As String)

ListView1.ColumnHeaders(1 + col_index(col)).text = title

End Property


Public Property Let col_width(ByVal col, ByVal twips As Long)

Dim idx As Long
idx = 1 + col_index(col)
LOG "sos_listview.col_width(" & col & "," & twips & "), idx= " & idx
ListView1.ColumnHeaders(idx).Width = twips

End Property


Public Property Let col_visible(ByVal col, ByVal visible As Boolean)

If Not visible Then ListView1.ColumnHeaders(1 + col_index(col)).Width = 0

End Property


Public Property Let col_default(ByVal col, ByVal default_value As Variant)

'column_array(col_index(col)).default = default_value
empty_record(col) = default_value

End Property



Public Property Let col_plausi(ByVal col, ByVal flags As ListView_flags)

'?

End Property


Public Property Let col_bool(ByVal col, ByVal bool As Boolean)

column_array(col_index(col)).bool = bool
'If bool Then col_width(col) = bool_width

End Property


Public Property Get col_bool(ByVal col) As Boolean

col_bool = column_array(col).bool

End Property


Public Property Let col_bool_reverse(ByVal col, ByVal bool As Boolean)

If VarType(col) = vbString Then col = col_index(col)
column_array(col).bool_reverse = bool

End Property


Public Property Get col_bool_reverse(ByVal col) As Boolean

col_bool_reverse = column_array(col).bool_reverse

End Property


Public Property Let HideSelection(ByVal hide As Boolean)

ListView1.HideSelection = hide

End Property


Public Property Let tagg(ByVal row_number As Long, ByVal value As Variant)

row_array(row_number).tagg = value

End Property


Public Property Set tagg(ByVal row_number As Long, value As Variant)

Set row_array(row_number).tagg = value

End Property


Public Property Get tagg(ByVal row_number As Long) As Variant

If IsObject(row_array(row_number).tagg) Then
    Set tagg = row_array(row_number).tagg
 Else
    tagg = row_array(row_number).tagg
End If

End Property


Public Sub read_control(ByVal ctrl As control)

Dim i       As Long
Dim idx     As Long
Dim record  As hostWare.Dyn_obj
Dim row     As Variant

row = Me.row

If in_write_control Then Exit Sub

in_read_control = True

If in_new_row Then
    Set record = empty_record.clone()
    add_record record
    row = ListView1.SelectedItem.Index
    cell_selected row
    
    For i = 0 To record.obj_field_count - 1
        If Not column_array(i).ctrl Is ctrl Then
            'If Not IsEmpty(column_array(i).default) Then record(i) = column_array(i).default
            
            If Not column_array(i).ctrl Is Nothing Then
                in_read_control = False
                read_control column_array(i).ctrl
                in_read_control = True
            End If
        End If
    Next i
Else
    If IsNull(row) Then Exit Sub      'Kann bei rekursivem Aufruf über write_control() passieren. jz 22.4.98
End If

For i = 0 To UBound(column_array)
    If column_array(i).ctrl Is ctrl Then Exit For
Next i

If i > UBound(column_array) Then err_raise "Sos_listview", "Unbekanntes Control in Sos_listview"

If TypeName(ctrl) = "CheckBox" Then
    coll(row)(i) = IIf((ctrl.value <> 0) <> column_array(i).bool_reverse, 1, 0)   'Nicht True/False, denn nicht jedes DBMS kennt Boolean und VB-True = -1 ist kein guter DB-Wert
 Else
    coll(row)(i) = ctrl.text
End If

set_list_cell row, i, coll(row)(i)
set_modified

in_read_control = False
    
End Sub


Private Sub write_control(ByVal ctrl As control, ByVal col As Long, ByVal value As Variant)

If ctrl Is Nothing Then Exit Sub
If in_read_control Then Exit Sub

in_write_control = True

If Not ctrls Is Nothing Then
    ctrls.write_control ctrl, value
 Else
    Select Case TypeName(ctrl)
        
        Case "CheckBox"
            If IsNull(as_bool(value)) Then
                ctrl.value = 0  '2
             Else
                If col_bool(col) Then
                    ctrl.value = IIf(col_bool_reverse(col) <> as_bool(value), 1, 0)
                 Else
                    ctrl.value = IIf(as_bool(value), 1, 0)
                End If
            End If
            
        Case Else
            ctrl = strng(value)
    End Select
End If

in_write_control = False

End Sub


Public Sub write_controls()

Dim i           As Long
Dim ctrl        As control

If Not IsNull(row) Then

    'If Not controls_frame Is Nothing Then controls_frame.visible = True
    
    For i = 0 To UBound(column_array)
        Set ctrl = column_array(i).ctrl
        If Not ctrl Is Nothing Then
            'ctrl.BackColor = vbWhite
            ctrl.Enabled = True
            If Not ctrls Is Nothing Then ctrls.ignore_error(ctrl) = False
            write_control ctrl, i, coll(row)(i) 'text
        End If
    Next i
    
ElseIf in_new_row Then

    set_status ""
    
    'If Not controls_frame Is Nothing Then controls_frame.visible = true
    
    For i = 0 To UBound(column_array)
        Set ctrl = column_array(i).ctrl
        If Not ctrl Is Nothing Then
            ctrl.Enabled = True
            If Not ctrls Is Nothing Then ctrls.ignore_error(ctrl) = True
            'write_control ctrl, i, column_array(i).default
            write_control ctrl, i, empty_record(i)
            'ctrl.BackColor = vbWhite
        End If
    Next i
    
Else
 
    set_status ""
    
    'If Not controls_frame Is Nothing Then controls_frame.visible = False
    
    For i = 0 To UBound(column_array)
        Set ctrl = column_array(i).ctrl
        If Not ctrl Is Nothing Then
            ctrl.Enabled = False
            write_control ctrl, i, ""
            'ctrl.BackColor = vbButtonFace
        End If
    Next i
    
End If

End Sub


#If 0 Then  'column_array().bool_reverse beachten!
Public Sub set_text(ByVal row As Long, ByVal col As Long, ByVal text As String)

coll(row)(col) = text

set_list_cell row, col, coll(row)(col)

If Not listview.SelectedItem Is Nothing Then
    If row = listview.SelectedItem.Index Then write_control column_array(col).ctrl, col, text
End If

set_modified

End Sub
#End If


Public Sub set_list_cell(ByVal row As Long, ByVal col As Long, ByVal value As Variant)

If IsNull(value) Then value = ""

If column_array(col).bool Then
    value = IIf(as_bool(value) <> column_array(col).bool_reverse, "X", "")
End If

If col = 0 Then
    ListView1.ListItems(row).text = value
 Else
    ListView1.ListItems(row).SubItems(col) = value
End If

End Sub


Public Property Let cell(ByVal row As Long, ByVal col As Long, value As Variant)

coll(row)(col) = value
set_list_cell row, col, value

End Property


Public Sub rebind()

Dim i           As Long
Dim j           As Long
Dim v           As Variant
Dim r           As hostWare.Dyn_obj

ReDim Preserve row_array(1 To max(1, coll.count))

For i = 1 To min(coll.count, ListView1.ListItems.count)
    Set r = coll(i)
    For j = 0 To ListView1.ColumnHeaders.count - 1
        set_list_cell i, j, r(j)
    Next j
Next i

For i = i To coll.count
    Set r = coll(i)
    ListView1.ListItems.add
    For j = 0 To r.obj_field_count - 1
        set_list_cell i, j, r(j)
    Next j
Next i

While i <= ListView1.ListItems.count
    ListView1.ListItems.remove i
Wend

If add_new_allowed Then
    ListView1.ListItems.add , , "(Neu ...)"
End If

'ListView1.Refresh

write_controls

End Sub


Public Sub remove_row(ByVal row As Long)

coll.remove row
ListView1.ListItems.remove row
cell_selected -1
If coll.count > 0 Then listview.ListItems(row).selected = True
write_controls

End Sub


Public Sub move_row(ByVal source_row As Long, ByVal dest_row As Long)

Dim i   As Long
Dim o   As hostWare.Dyn_obj
Dim rd  As Row_descr

ReDim Preserve row_array(1 To coll.count)
rd = row_array(source_row)

If dest_row > source_row Then
    coll.add coll(source_row), , , dest_row
    coll.remove source_row
    
    For i = source_row To dest_row - 1
        row_array(i) = row_array(i + 1)
    Next i
 Else
    coll.add coll(source_row), , dest_row
    coll.remove source_row + 1
    
    For i = source_row To dest_row + 1 Step -1
        row_array(i) = row_array(i - 1)
    Next i
End If

row_array(dest_row) = rd
If Me.row = source_row Then Me.row = dest_row
rebind

End Sub


Private Sub ListView1_ItemClick(ByVal item As ComctlLib.ListItem)

select_item item

End Sub


'Private Sub ListView1_ColumnClick(ByVal ColumnHeader As ComctlLib.ColumnHeader)
'
'End Sub


'Private Sub doc_ListView_ItemClick(ByVal item As ComctlLib.ListItem)
'
'End Sub

Private Sub ListView1_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)

Dim item    As ListItem

Set item = ListView1.HitTest(X, Y)
If item Is Nothing Then Set item = ListView1.HitTest(30, Y)
select_item item

End Sub


Public Property Get error() As Boolean

Dim i As Long

If IsNull(current_row) Then error = False: Exit Property
If ctrls Is Nothing Then error = False: Exit Property

For i = 0 To UBound(column_array)
    With column_array(i)
        If Not .ctrl Is Nothing Then
            If ctrls.error(.ctrl) Then error = True: Exit Property
        End If
    End With
Next

error = False

End Property


Private Sub select_item(ByVal item As ComctlLib.ListItem)

Dim i As Long

If Me.error Then
    Set ListView1.SelectedItem = ListView1.ListItems(current_row)
 Else
    Set ListView1.SelectedItem = item
    cell_selected -1
End If

End Sub


Private Sub cell_selected(ByVal col As Long)

Dim last_row    As Variant
Dim last_col    As Long

last_row = current_row
last_col = current_col

current_row = row
current_col = col

If ifnull(current_row, -1) <> ifnull(last_row, -1) Or current_col <> last_col Then
    write_controls
    RaiseEvent RowColChange(last_row, last_col)
End If

End Sub


Private Sub set_modified()

If add_mode Then Exit Sub

RaiseEvent modified
If Not ctrls Is Nothing Then ctrls.set_modified ListView1

End Sub


Private Sub UserControl_Initialize()

ListView1.HideSelection = False
current_row = Null
current_col = -1

ReDim row_array(1 To 1)

End Sub


Private Sub UserControl_Resize()

ListView1.Width = UserControl.Width
ListView1.Height = UserControl.Height

End Sub

