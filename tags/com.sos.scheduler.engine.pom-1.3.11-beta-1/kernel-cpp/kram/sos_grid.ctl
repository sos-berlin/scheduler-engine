VERSION 5.00
Object = "{00028C4A-0000-0000-0000-000000000046}#5.0#0"; "TDBG5.OCX"
Begin VB.UserControl Sos_grid 
   ClientHeight    =   3600
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   4800
   ScaleHeight     =   3600
   ScaleWidth      =   4800
   Begin TrueDBGrid50.TDBGrid TDBGrid1 
      Height          =   3255
      Left            =   0
      OleObjectBlob   =   "sos_grid.ctx":0000
      TabIndex        =   0
      Top             =   0
      Width           =   4575
   End
End
Attribute VB_Name = "Sos_grid"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Event Click(row As Long, col As Long)
Event ButtonClick(ByVal ColIndex As Integer)
Event RowColChange(LastRow As Variant, ByVal LastCol As Integer)
Event PostEvent(ByVal MsgId As Integer)
Event modified()
Event error(error As Boolean)
Event FetchCellTips(ByVal SplitIndex As Integer, ByVal ColIndex As Integer, ByVal RowIndex As Long, CellTip As String, ByVal FullyDisplayed As Boolean, ByVal TipStyle As TrueDBGrid50.StyleDisp)
Event FetchCellStyle(ByVal Condition As Integer, ByVal Split As Integer, ByVal RowIndex As Long, ByVal col As Integer, ByVal CellStyle As TrueDBGrid50.StyleDisp)
Event FetchRowStyle(ByVal Split As Integer, ByVal RowIndex As Long, ByVal RowStyle As TrueDBGrid50.StyleDisp)
Event OnAddNew()
Event AfterCellChange(ByVal row As Long, ByVal col As Integer)

Enum Grid_flags
    grid_not_null = 2               'Das Feld muss ausgefüllt sein
End Enum


Enum Grid_cell_status
    grid_error = 1
End Enum

Private Type Wrong_value
    row     As Long
    col     As Integer
    value   As Variant
    error_text As String
End Type

Public ctrls            As Control_object_link
Public row_record       As hostWare.Dyn_obj             ' Record-Typ der Zeilen, wird von collection_load_file gesetzt
'Public AfterCellChange_allowed As Boolean

'Public tag_coll            As New Collection
Private tag_array()    As Variant

Private add_mode            As Boolean                      'True: Ereignis modified nicht auslösen
Private err_flag            As Boolean
Private obj_coll            As Collection
Private plausi_array()      As Long                         'Enthält die Plausis für jede Spalte
Private status_array()      As String                       'Jedes Zeichen entspricht einer Tabellenspalte. Enthölt Grind_cell_status
Private wrong_value_array(1 To 100)  As Wrong_value         'Die letzten paar falschen Eingaben vorhalten (werden gelb markiert)
Private wrong_value_q             As Long
Private defaultMarqueeStyle As Integer
Private control_array()     As control                      'Jeder Spalte kann eine externes Control für die Detail-Ansicht zugeordnet werden

Public Property Get ActiveControl() As control

Set ActiveControl = UserControl.ActiveControl

End Property


Public Property Get error() As Boolean

error = err_flag

End Property


Public Property Set coll(c As Collection)

Dim i As Long

Set obj_coll = c
ReDim status_array(0 To 0)

For i = 1 To UBound(wrong_value_array)
    wrong_value_array(i).row = 0
Next i

End Property


Public Property Get coll() As Collection

add_mode = True
Set coll = obj_coll
add_mode = False

End Property


Public Property Get row() As Variant

If TDBGrid1.AddNewMode <> dbgNoAddNew Then 'Bediener gibt gerade neue Zeile ein?
    row = Null
ElseIf IsNull(TDBGrid1.bookmark) Then
    row = Null
Else
    row = TDBGrid1.bookmark
End If

End Property


Public Property Let row(ByVal row_number As Variant)

TDBGrid1.bookmark = row_number

End Property


Public Property Let tagg(ByVal row_number As Long, ByVal value As Variant)

tag_array(row_number) = value

End Property


Public Property Set tagg(ByVal row_number As Long, value As Variant)

Set tag_array(row_number) = value

End Property

Public Property Get tagg(ByVal row_number As Long) As Variant

If IsObject(tag_array(row_number)) Then
    Set tagg = tag_array(row_number)
 Else
    tagg = tag_array(row_number)
End If

End Property


Public Property Get columns() As TrueDBGrid50.columns

Set columns = TDBGrid1.columns

End Property


Public Property Get col_index(ByVal name As String) As Long

col_index = TDBGrid1.columns(name).ColIndex

End Property


Public Property Get grid() As TrueDBGrid50.TDBGrid

Set grid = TDBGrid1

End Property


Public Property Let plausi(ByVal ColIndex As Variant, ByVal flags As Grid_flags)

Dim idx As Long

idx = TDBGrid1.columns(ColIndex).ColIndex

If UBound(plausi_array) < idx Then ReDim Preserve plausi_array(0 To idx)

plausi_array(idx) = CLng(flags)

TDBGrid1.columns(ColIndex).FetchStyle = flags <> 0

End Property


Public Sub clear()

Set obj_coll = Nothing

TDBGrid1.ClearFields

On Error Resume Next
TDBGrid1.bookmark = Null

End Sub


Public Sub load_file(ByVal filename As String)

Dim file    As New hostWare.file
Dim j       As Long
Dim n       As Long
Dim X       As New XArray
Dim col     As TrueDBGrid50.Column

'Ein paar Standard-Einstellungen:
'grid.CellTips = dbgAnchored  'Lässt zu langen Zelleninhalt als Tip anzeigen
TDBGrid1.AllowColSelect = False
TDBGrid1.AllowRowSizing = False
TDBGrid1.ExtendRightColumn = True
TDBGrid1.TabAction = dbgColumnNavigation

'While tdbgrid1.Splits.count > 1: tdbgrid1.Splits.remove 0: Wend

'??? Folgendes .remove lieferte am 2.3.98 in factory, Neue Abfrage: Aufruf der Methode remove fehlgeschlagen.
On Error GoTo GRID_MIST
While TDBGrid1.columns.count > 0: TDBGrid1.columns.remove 0: Wend
On Error GoTo 0

Set row_record = Nothing
Set coll = New Collection

file.open "-in -seq " & filename

n = file.field_count

For j = 0 To n - 1
    Set col = TDBGrid1.columns.add(j)
    col.DataField = file.field_name(j)
    col.caption = file.field_name(j)
    col.visible = True
Next j

sos_grid_load_file Me, file

add_mode = True
check_all_cells
add_mode = False

file.Close

If TDBGrid1.DataMode <> 3 Then MsgBox "tdbgrid.datamode <> 3"

TDBGrid1.bookmark = Null
reopen

'If obj_coll.count > 0 Then ReDim status_array(1 To obj_coll.count, 0 To n - 1)

Exit Sub

GRID_MIST:
    ' "Aufruf der Methode remove fehlgeschlagen."
    'Im Debugger kann nach diesem Fehler das Programm fortgesetzt werden, als wäre nix geschehen.
    'Vielleicht hilft eine MsgBox:
    Dim grid_err As Long
    If grid_err >= 3 Then raise_again
    grid_err = grid_err + 1
    show_error
    Resume Next

End Sub

Public Sub add_record(ByVal record As hostWare.Dyn_obj)

On Error GoTo FEHLER

add_mode = True

obj_coll.add record
'If Not tag_coll Is Nothing Then tag_coll.add Nothing
ReDim Preserve tag_array(1 To obj_coll.count)

check_row obj_coll.count
TDBGrid1.ApproxCount = obj_coll.count

add_mode = False
Exit Sub

FEHLER:
    add_mode = False
    raise_again

End Sub

Public Sub set_column_bool(ByVal name As String, Optional ByVal reverse As Boolean)

tdbgrid_helper.set_column_bool TDBGrid1, name, reverse
TDBGrid1.columns(name).Width = tdbgrid_bool_width

End Sub


Public Sub fix_columns(ByVal columns As Long)

tdbgrid_fix_columns TDBGrid1, columns

End Sub

Public Property Let visible(ByVal b As Boolean)

TDBGrid1.visible = b

End Property


Public Property Let ExtendRightColumn(ByVal b As Boolean)

TDBGrid1.ExtendRightColumn = b

End Property


Public Sub reopen()

Dim i As Long
Dim n As Long

n = obj_coll.count

On Error Resume Next
TDBGrid1.bookmark = Null
If Err Then LOG ("tdbgrid1.bookmark=Null  " & Err.Description)
On Error GoTo 0

TDBGrid1.reopen

If n > 0 Then TDBGrid1.bookmark = 1

ReDim status_array(0 To n)

ReDim Preserve control_array(0 To TDBGrid1.columns.count - 1)

For i = 0 To TDBGrid1.columns.count - 1
    If IsEmpty(control_array(i)) Then Set control_array(i) = Nothing
Next i

If n > 0 Then ReDim tag_array(1 To n)

End Sub


Public Sub rebind()

TDBGrid1.rebind

End Sub


Public Sub set_text(ByVal row As Long, ByVal col As Variant, ByVal text As String)

Dim saved_firstrow  As Variant
Dim saved_col       As Variant
Dim saved_leftcol   As Variant
Dim saved_row       As Variant

If VarType(col) = vbString Then col = TDBGrid1.columns(col).ColIndex

'jz 4.5.98 obj_coll(row)(col) = text

saved_col = TDBGrid1.col
saved_row = TDBGrid1.bookmark
saved_firstrow = TDBGrid1.FirstRow
saved_leftcol = TDBGrid1.LeftCol

'? If grid.AddNewMode = dbgNoAddNew Then
TDBGrid1.bookmark = row
TDBGrid1.col = col
TDBGrid1.text = strng(text)
'TDBGrid1.Update     'Ruft ClassicWrite
cell_to_obj row, col, text

TDBGrid1.col = saved_col
TDBGrid1.bookmark = saved_row
TDBGrid1.FirstRow = saved_firstrow
TDBGrid1.LeftCol = saved_leftcol

End Sub


Public Sub set_control(ByVal col As Variant, ByVal ctrl As control)
'Hängt ein Detail-Control an eine Spalte

If VarType(col) = vbString Then col = TDBGrid1.columns(col).ColIndex
Set control_array(col) = ctrl

End Sub


Public Property Get DataChanged() As Boolean

DataChanged = TDBGrid1.DataChanged

End Property


Public Sub Update()

TDBGrid1.Update

End Sub


Private Sub TDBGrid1_ComboSelect(ByVal ColIndex As Integer)

Dim row As Long

If IsNull(TDBGrid1.bookmark) Then
    'Wenn die Tabelle leer ist, ist tdbgrid1.bookmark null. jz 11.5.98
    row = coll.count
 Else
    row = TDBGrid1.bookmark
End If


cell_to_obj row, ColIndex, TDBGrid1.text
'RaiseEvent Click(TDBGrid1.bookmark, ColIndex)

End Sub


Private Sub TDBGrid1_Error(ByVal DataError As Integer, Response As Integer)

LOG "Sos_grid TDBGrid1_Error " & CStr(DataError)
'Fehler von ClassicWrite nicht melden!
Response = 0

End Sub

Private Sub TDBGrid1_FetchCellStyle(ByVal Condition As Integer, ByVal Split As Integer, bookmark As Variant, ByVal col As Integer, ByVal CellStyle As TrueDBGrid50.StyleDisp)

'Debug.Print "TDBGrid1_FetchCellStyle begin " & bookmark & ", " & col

If cell_status(bookmark, col) And grid_error Then
    CellStyle.BackColor = vbYellow
Else
    CellStyle.BackColor = vbWhite
    If IsNull(bookmark) Then Exit Sub
    ' Event wird nur im Normalfall nach oben gereicht
    RaiseEvent FetchCellStyle(Condition, Split, CLng(bookmark), col, CellStyle)
End If

'Debug.Print "TDBGrid1_FetchCellStyle end"

End Sub


Private Sub TDBGrid1_FetchRowStyle(ByVal Split As Integer, bookmark As Variant, ByVal RowStyle As TrueDBGrid50.StyleDisp)

If IsNull(bookmark) Then Exit Sub
RaiseEvent FetchRowStyle(Split, CLng(bookmark), RowStyle)

End Sub

Private Sub tdbgrid1_UnboundGetRelativeBookmark(StartLocation As Variant, ByVal offset As Long, NewLocation As Variant, ApproximatePosition As Long)

Dim row As Long

If obj_coll Is Nothing Then
    StartLocation = Null
    Exit Sub
End If
    
If IsNull(StartLocation) Then
    If offset < 0 Then
        row = obj_coll.count + 1 + offset
     Else
        row = offset
    End If
 Else
    row = StartLocation + offset
End If

If row >= 1 And row <= obj_coll.count Then
    NewLocation = row
    ApproximatePosition = row
    'Debug.Print "UnboundGetRelativeBookmark row=" & CStr(row)
 Else
    NewLocation = Null
End If

End Sub

Private Function cell_status(ByVal row As Long, ByVal col As Long) As Grid_cell_status

If row > UBound(status_array) Then
    cell_status = 0
ElseIf 1 + col > Len(status_array(row)) Then
    cell_status = 0
Else
    cell_status = Asc(Mid$(status_array(row), 1 + col, 1)) - 64
End If

End Function


Private Sub set_cell_status(ByVal row As Long, ByVal col As Long, status As Grid_cell_status)

Dim s As String

If row > UBound(status_array) Then ReDim Preserve status_array(0 To row)

s = status_array(row)

If 1 + col > Len(s) Then s = s & String$(col + 1 - Len(s), " ")
Mid$(s, 1 + col, 1) = Chr$(64 + status)

status_array(row) = s

End Sub


Private Function wrong_value_idx(ByVal row As Long, ByVal col As Long) As Variant

Dim i As Long

For i = 1 To UBound(wrong_value_array)
    With wrong_value_array(i)
        If .row = row And .col = col Then wrong_value_idx = i: Exit Function
    End With
Next i

wrong_value_idx = Null

End Function


Private Sub tdbgrid1_ClassicRead(bookmark As Variant, ByVal col As Integer, value As Variant)

Dim i           As Long
Dim err_flag    As Boolean
Dim w           As Variant

If col >= TDBGrid1.columns.count Then Exit Sub

If Not obj_coll Is Nothing Then
    If bookmark >= 1 And bookmark <= obj_coll.count Then
        If cell_status(bookmark, col) And grid_error Then
            w = wrong_value_idx(bookmark, col)
            If Not IsNull(w) Then value = wrong_value_array(w).value
         Else
            value = obj_coll(bookmark)(col)
            If VarType(value) = vbBoolean Then value = IIf(value, 1, 0)
            'Debug.Print "ClassicRead " & CStr(bookmark) & ", " & CStr(col) & " = " & strng(value)
        End If
    End If
End If

End Sub


Private Sub TDBGrid1_ClassicWrite(bookmark As Variant, ByVal col As Integer, value As Variant)

'ClassicWrite für Texteingaben überflüssig: TDBGrid1_change hat den Wert schon gespeichert.
'Nicht aber: CycleonClick und Zuweisungen an tdbgrid1.text

If IsNull(bookmark) Then Exit Sub  'Was soll denn das?
If obj_coll Is Nothing Then Exit Sub
If bookmark > obj_coll.count Then Exit Sub

cell_to_obj bookmark, col, value

End Sub


Private Sub TDBGrid1_ClassicAdd(NewRowBookmark As Variant, ByVal col As Integer, value As Variant)

If obj_coll Is Nothing Then Exit Sub
NewRowBookmark = obj_coll.count

cell_to_obj NewRowBookmark, col, value

End Sub

Private Sub TDBGrid1_ClassicDelete(bookmark As Variant)

obj_coll.remove bookmark

End Sub

Public Sub set_defaults(ByVal row As Long)

Dim i   As Long
Dim col As TrueDBGrid50.Column
Dim obj As hostWare.Dyn_obj

If row = 0 Then err_raise "set_defaults", "Interner Fehler: obj_coll beginnt bei 1"


Set obj = obj_coll(row)

For i = 0 To TDBGrid1.columns.count - 1
    Set col = TDBGrid1.columns(i)
    If IsNull(obj(i)) Then
        col.value = col.DefaultValue
        obj(i) = col.DefaultValue
        clear_error row, i
    End If
Next i

End Sub


Private Sub TDBGrid1_BeforeColEdit(ByVal ColIndex As Integer, ByVal KeyAscii As Integer, cancel As Integer)
Dim old_coll_count As Long
Dim new_record As hostWare.Dyn_obj

If TDBGrid1.AddNewMode = dbgAddNewCurrent Then

'Dim text    As String

'    text = TDBGrid1.text
    old_coll_count = obj_coll.count
    RaiseEvent OnAddNew
    ' Überprüfen ob OnAddNew geggriffen hat, d.h. einen neuen Satz eingefügt hat
    If obj_coll.count = old_coll_count Then
        ' Event-Methode OnAddNew nicht implementiert?
        If row_record Is Nothing Then
            If obj_coll.count > 0 Then ' Existiert eine Zeile?
                Set row_record = obj_coll(1)
            Else
                err_raise "sos_grid", "Fatal: neuer Record für die neue Zeile konnte nicht angelegt werden"
            End If
        End If
        Set new_record = row_record.clone()
        add_record new_record
    End If
    
    On Error Resume Next
    TDBGrid1.Update
    On Error GoTo 0
    
    ' Constraint: coll_count = old_coll_count + 1
    set_defaults obj_coll.count
    'TDBGrid1.ReBind
'    TDBGrid1.text = text
    'TDBGrid1.SelLength = 1   funktioniert nicht
    'TDBGrid1.selstart = 2
End If

End Sub

Private Sub TDBGrid1_Change()

Dim row     As Long
Dim ctrl    As control

If TDBGrid1.AddNewMode = dbgAddNewPending Then
    row = obj_coll.count
ElseIf TDBGrid1.AddNewMode = dbgAddNewCurrent Then
    row = obj_coll.count
ElseIf IsNull(TDBGrid1.bookmark) Then
    row = 1
Else
    row = TDBGrid1.bookmark
End If

cell_to_obj row, TDBGrid1.col, TDBGrid1.text

'write_control control_array(TDBGrid1.col), TDBGrid1.col, TDBGrid1.text

End Sub


Private Sub tdbgrid1_lostfocus()

If TDBGrid1.DataChanged Then
    
    TDBGrid1.Update
End If

End Sub

Public Sub check_all_cells(Optional set_add_mode As Boolean)
'Prüft alle Zellen, indem Sie mit cell_to_obj() in die Dyn_objs zurückgeschrieben werden.

Dim i       As Long
Dim n       As Long
Dim record  As hostWare.Dyn_obj

If set_add_mode Then add_mode = True
n = obj_coll.count

For i = 1 To n
    check_row i
Next i
If set_add_mode Then add_mode = False

End Sub


Public Sub check_row(ByVal row As Long)

Dim j As Long
Dim n As Long
Dim record As hostWare.Dyn_obj

Set record = obj_coll(row)
For j = 0 To record.obj_field_count - 1
    cell_to_obj row, j, record(j)
Next j
    
End Sub


Private Sub cell_to_obj(ByVal row As Long, ByVal col As Long, ByVal value As Variant)

Dim record As hostWare.Dyn_obj

'Debug.Print "cell_to_obj " & CStr(row) & ":" + CStr(col) & " = " & strng(value)

On Error GoTo FEHLER

If is_column_bool(col) Then 'Weil False <> "0"
    If as_bool(obj_coll(row)(col)) = as_bool(value) Then GoTo OK
End If

If UBound(plausi_array) < col Then ReDim Preserve plausi_array(0 To col)
check_text plausi_array(col), value

If col <= UBound(control_array) Then
    write_control control_array(col), col, strng(value)
End If

'Besser: if obj_coll(bookmark)(col) = type_cast(typename(obj_coll(bookmark)(col)), value) then
Set record = obj_coll(row)
If record(col) = value Then
OK:
    'ok, nix is null
    clear_error row, col
 Else
    record(col) = value
    clear_error row, col
    set_modified
    RaiseEvent AfterCellChange(row, col)
End If

ende:
Exit Sub

FEHLER:
    set_error row, col, value, Err.Description

End Sub

Private Sub tdbgrid1_ButtonClick(ByVal ColIndex As Integer)

If UBound(plausi_array) < TDBGrid1.col Then ReDim Preserve plausi_array(0 To TDBGrid1.col)
RaiseEvent ButtonClick(ColIndex)

End Sub


Private Sub write_control(ByVal ctrl As control, ByVal col As Long, ByVal text As String)

If ctrl Is Nothing Then Exit Sub

If TypeName(ctrl) = "CheckBox" Then
    If IsNull(as_bool(text)) Then
        ctrl.value = 0  '2
     Else
        If is_column_bool(col) Then
            ctrl.value = IIf(is_column_bool_reverse(col) <> as_bool(text), 1, 0)
         Else
            ctrl.value = IIf(as_bool(text), 1, 0)
        End If
    End If
 Else
    ctrl = text
End If

End Sub


Public Sub read_control(ByVal ctrl As control)

Dim i       As Long
Dim text    As String

If IsNull(TDBGrid1.bookmark) Then Exit Sub      'Kann bei rekursivem Aufruf über write_control() passieren. jz 22.4.98

For i = 0 To UBound(control_array)
    If control_array(i) Is ctrl Then Exit For
Next i


If TypeName(ctrl) = "CheckBox" Then
    text = IIf(ctrl.value = 0, "0", IIf(ctrl.value = 1, "1", ""))
    If is_column_bool(i) Then
        If is_column_bool_reverse(i) Then text = IIf(text = "0", "1", "0")
    End If
 Else
    text = ctrl.text
End If

set_text TDBGrid1.bookmark, i, text

End Sub


Private Sub tdbgrid1_RowColChange(LastRow As Variant, ByVal LastCol As Integer)

Dim w
Dim i       As Long
Dim ctrl    As control

If Not IsNull(TDBGrid1.bookmark) Then
    
    If cell_status(TDBGrid1.bookmark, TDBGrid1.col) And grid_error Then
        w = wrong_value_idx(TDBGrid1.bookmark, TDBGrid1.col)
        If Not IsNull(w) Then set_status_error wrong_value_array(w).error_text
     Else
        set_status ""
    End If
    
    For i = 0 To UBound(control_array)
        Set ctrl = control_array(i)
        If Not ctrl Is Nothing Then write_control ctrl, i, TDBGrid1.columns(i).value 'text
    Next i
    
 Else
 
    set_status ""
    
    For i = 0 To UBound(control_array)
        Set ctrl = control_array(i)
        If Not ctrl Is Nothing Then write_control ctrl, i, ""
    Next i
    
End If


RaiseEvent RowColChange(LastRow, LastCol)

End Sub


Private Sub tdbgrid1_OnAddNew()
    
'Debug.Print "tdbgrid1_OnAddNew"

'tdbgrid_set_new_defaults TDBGrid1

End Sub


Private Sub tdbgrid1_KeyDown(KeyCode As Integer, Shift As Integer)
    
If (Shift And vbAltMask) Then
    Dim dest_row As Long
    If KeyCode = vbKeyUp Then
        If TDBGrid1.bookmark <= 0 Then Exit Sub
        dest_row = TDBGrid1.bookmark - 1
    ElseIf KeyCode = vbKeyDown Then
        If TDBGrid1.bookmark >= obj_coll.count Then Exit Sub
        dest_row = TDBGrid1.bookmark + 1
    Else
        Exit Sub
    End If

    move_row TDBGrid1.bookmark, dest_row
    set_modified
End If

End Sub


Public Function is_column_bool(ByVal col As Long) As Boolean

With TDBGrid1.columns(col).ValueItems
    If .count = 2 And .CycleOnClick Then
        is_column_bool = (.item(0).value = "0" And .item(1).value = "1") <> (.item(0).value = "1" And .item(1).value = "0")
    Else
        is_column_bool = False
    End If
End With

End Function


Public Function is_column_bool_reverse(ByVal col As Long) As Boolean

is_column_bool_reverse = TDBGrid1.columns(col).ValueItems.item(0).value = "1"

End Function


Private Sub tdbgrid1_KeyPress(KeyAscii As Integer)

If TDBGrid1.bookmark < 0 Then Exit Sub
If TDBGrid1.col < 0 Then Exit Sub

If is_column_bool(TDBGrid1.col) Then
    Select Case KeyAscii
    
        Case Asc("j"), Asc("J"), Asc("y"), Asc("Y"), Asc("1")
            If Not as_bool(obj_coll(TDBGrid1.bookmark)(TDBGrid1.col)) Then
                TDBGrid1.text = "1"
                'obj_coll(TDBGrid1.Bookmark)(TDBGrid1.Col) = "1"
                TDBGrid1.Update
                set_modified
            End If
        
        Case Asc("n"), Asc("N"), Asc("0")
            If as_bool(obj_coll(TDBGrid1.bookmark)(TDBGrid1.col)) Then
                TDBGrid1.text = "0"
                'obj_coll(TDBGrid1.Bookmark)(TDBGrid1.Col) = "0"
                TDBGrid1.Update
                set_modified
            End If
        
        Case Asc(" ") ' Leerzeichen, toggle
            If as_bool(obj_coll(TDBGrid1.bookmark)(TDBGrid1.col)) Then
                TDBGrid1.text = "0"
             Else
                TDBGrid1.text = "1"
            End If
            'obj_coll(TDBGrid1.Bookmark)(TDBGrid1.Col) = TDBGrid1.text
            TDBGrid1.Update
            set_modified
        
        Case Else ' Andere Werte bleiben wie sie sind
            KeyAscii = 0
            Exit Sub
    
    End Select
    
    KeyAscii = 0

 Else

    With TDBGrid1.columns(TDBGrid1.col).ValueItems
        If .CycleOnClick And .Validate Then
            'Nur Werte aus der Werteliste erlaubt? Dann Tastatur sperren, denn getippte Werte, die nicht in der
            'Werteliste sind führen dazu, dass Focuswechsel ignoriert und die Eingabe gelöscht wird.
            'Der Bediener weiß dann nicht, wie er aus dem Feld herauskommt. jz 26.2.98
            KeyAscii = 0   'Tastatur sperren
            Exit Sub
        End If
    End With

End If

End Sub


Private Sub tdbgrid1_DragCell(ByVal SplitIndex As Integer, RowBookmark As Variant, ByVal ColIndex As Integer)

Debug.Print "params_DragCell"
If ColIndex >= 0 Then
    ' Set the current cell to the one being dragged
    TDBGrid1.col = ColIndex
    ' Wenn Draggen von jeder Spalte erlaubt sein soll ...
    Exit Sub
End If
TDBGrid1.row = RowBookmark

' HighlightMode setzen
TDBGrid1.MarqueeStyle = dbgHighlightRow

' Set up drag operation, such as creating visual effects by
' highlighting the cell or row being dragged.
' Use VB manual drag support (put TDBGrid1 into drag mode)
TDBGrid1.Drag vbBeginDrag

End Sub


Private Sub tdbgrid1_DragDrop(Source As control, X As Single, Y As Single)

Debug.Print "params_DragDrop"

Dim dropRow As Long

dropRow = TDBGrid1.RowContaining(Y)

'Debug.Print "DragDrop: Cell=" & CStr(dropRow) & "," & CStr(dropCol) & ")"
If tdbgrid_DragDrop_Row(TDBGrid1, dropRow) Then set_modified

ResetDragDrop

End Sub

Private Sub tdbgrid1_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)

' Debug.Print "params_MouseMove"
' Wenn MouseMove-Events kommen, dann muß der DragModus zurückgesetzt werden
If Button = 0 Then ResetDragDrop

End Sub


Private Sub ResetDragDrop()

'Debug.Print "ResetDragDrop"
' Highlight Style zurücksetzen
If TDBGrid1.MarqueeStyle = dbgHighlightRow Then grid.MarqueeStyle = defaultMarqueeStyle

End Sub

Private Sub tdbgrid1_DragOver(Source As control, X As Single, Y As Single, State As Integer)
'Debug.Print "params_DragOver"

' Set current cell to "track" the dragging object
'Dim overCol As Integer
'Dim overRow As Long

'overCol = params.ColContaining(X)
'overRow = params.RowContaining(Y)

'Debug.Print "DragOver: Cell=" & CStr(overRow) & "," & CStr(overCol) & ")"

End Sub


Private Sub tdbgrid1_grid_FetchCellTips(ByVal SplitIndex As Integer, ByVal ColIndex As Integer, ByVal RowIndex As Long, CellTip As String, ByVal FullyDisplayed As Boolean, ByVal TipStyle As TrueDBGrid50.StyleDisp)

RaiseEvent FetchCellTips(SplitIndex, ColIndex, RowIndex, CellTip, FullyDisplayed, TipStyle)

End Sub


Public Sub remove_row(Optional ByVal row As Variant)

#If False Then
If IsMissing(row) Then row = TDBGrid1.bookmark

If TDBGrid1.bookmark >= obj_coll.count Then TDBGrid1.bookmark = IIf(obj_coll.count > 1, obj_coll.count - 1, Null)

coll.remove row

'If obj_coll.count = 0 Then
'    TDBGrid1.bookmark = Null
' Else
'    If IsNull(TDBGrid1.bookmark) Then TDBGrid1.bookmark = 1
'    If TDBGrid1.bookmark > obj_coll.count Then TDBGrid1.bookmark = obj_coll.count
'End If

TDBGrid1.rebind
#Else
If Not IsMissing(row) Then TDBGrid1.bookmark = row
TDBGrid1.Delete
TDBGrid1.ApproxCount = obj_coll.count
#End If

End Sub


Public Sub move_row(ByVal source_row As Long, ByVal dest_row As Long)

Exit Sub

Dim i   As Long
Dim obj As hostWare.Dyn_obj
Dim tag As Variant

Set obj = obj_coll(source_row)
'If tag_coll.count >= source_row Then tag = tag_coll(source_row)

If dest_row > source_row Then
    For i = source_row To dest_row - 1
        obj_coll(i) = obj_coll(i + 1)
'        If Not tag_coll Is Nothing Then tag_coll(i) = tag_coll(i + 1)
    Next i
 Else
    For i = source_row To dest_row + 1 Step -1
        obj_coll(i) = obj_coll(i - 1)
'        If Not tag_coll Is Nothing Then tag_coll(i) = tag_coll(i - 1)
    Next i
End If

Set obj_coll(dest_row) = obj
'If Not tag_coll Is Nothing Then tag_coll(dest_row) = tag

TDBGrid1.rebind

If Not IsNull(TDBGrid1.bookmark) Then
    If TDBGrid1.bookmark = source_row Then TDBGrid1.bookmark = dest_row
End If

End Sub



Private Sub tdbgrid1_PostEvent(ByVal MsgId As Integer)

RaiseEvent PostEvent(MsgId)

End Sub

Private Sub check_text(ByVal flags As Long, ByVal value As Variant)

If (flags And grid_not_null) <> 0 Then
    If RTrim$(strng(value)) = "" Then err_raise "Control_object_link", "Bitte füllen Sie das Feld aus"
End If

End Sub


Private Sub set_error(ByVal row As Long, ByVal col As Long, ByVal value As Variant, ByVal error_text As String)

Dim wrong_idx   As Long

set_status_error error_text

If (cell_status(row, col) And grid_error) = 0 Then
    set_cell_status row, col, grid_error
    wrong_value_q = wrong_value_q + 1
    If wrong_value_q > UBound(wrong_value_array) Then wrong_value_q = 1
    wrong_idx = wrong_value_q
 Else
    wrong_idx = wrong_value_idx(row, col)
End If

With wrong_value_array(wrong_idx)
    .row = row
    .col = col
    .value = value
    .error_text = error_text
End With

err_flag = True

RaiseEvent error(True)
If Not ctrls Is Nothing Then ctrls.set_error TDBGrid1

End Sub


Private Sub clear_error(ByVal row As Long, ByVal col As Long)

Dim i           As Long
Dim any_error   As Boolean
Dim status      As Long

If err_flag Then    'Liegt überhaupt ein Fehler vor?
    status = cell_status(row, col)
    
    If status And grid_error Then
        set_cell_status row, col, status And Not grid_error
        set_status ""
        For i = 1 To UBound(wrong_value_array)
            With wrong_value_array(i)
                If .row = row And .col = col Then .row = 0: .value = Null: .error_text = "": Exit For
                If .row > 0 Then any_error = True
            End With
        Next i
     Else
        i = 1
    End If

    For i = i To UBound(wrong_value_array)
        If wrong_value_array(i).row > 0 Then any_error = True: Exit For
    Next i
    
    If Not any_error Then
        err_flag = False
        RaiseEvent error(False)
        If Not ctrls Is Nothing Then ctrls.clear_error TDBGrid1
    End If
End If

End Sub


Private Sub set_modified()

If add_mode Then Exit Sub

RaiseEvent modified
If Not ctrls Is Nothing Then ctrls.set_modified TDBGrid1

End Sub


Private Sub UserControl_Initialize()

ReDim status_array(0 To 0)
ReDim plausi_array(0 To 0)
ReDim control_array(0 To 0)
ReDim tag_array(1 To 1)

While TDBGrid1.columns.count > 0
    TDBGrid1.columns.remove 0
Wend

End Sub


Private Sub UserControl_Resize()

TDBGrid1.Width = UserControl.Width
TDBGrid1.Height = UserControl.Height

End Sub

