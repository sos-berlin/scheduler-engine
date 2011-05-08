VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Resize_mgr"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
' anchor constants
'Const vbCenter = 1
Const vbEast = 1
Const vbSouth = 2
Const vbWest = 0
Const vbNorth = 0
Const vbNorthEast = vbNorth + vbEast
Const vbNorthWest = vbNorth + vbWest
Const vbSouthEast = vbSouth + vbEast
Const vbSouthWest = vbSouth + vbWest
Const vbAll = vbNorth + vbEast + vbSouth + vbWest

' fill constants
Const vbNone = 1
Const vbHorizontal = 2
Const vbVertical = 4
Const vbBoth = vbHorizontal + vbVertical

Private Type Resize_constraint
    anchor  As Long     ' Verankerung
    fill    As Long     ' Resize-Verhalten
    ctrl    As control
    Top     As Long
    Left    As Long
    Right   As Long
    Bottom  As Long
    parentWidth As Long
    parentHeight As Long
    tag     As String
End Type

' Array von Constraints
Private constraint_array() As Resize_constraint
Private constraint_count As Long
Private initialized As Long

' Private Variablen
Private minFormWidth As Long
Private minFormHeight As Long
Private Dlg As Form
Private activeTag As String

Public Property Get TopLeft() As Long
    TopLeft = vbNorthWest
End Property
Public Property Get TopRight() As Long
    TopRight = vbNorthEast
End Property
Public Property Get BottomRight() As Long
    BottomRight = vbSouthEast
End Property
Public Property Get BottomLeft() As Long
    BottomLeft = vbSouthWest
End Property
Public Property Get Top() As Long
    Top = vbNorth
End Property
Public Property Get Left() As Long
    Left = vbWest
End Property
Public Property Get Right() As Long
    Right = vbEast
End Property
Public Property Get Bottom() As Long
    Bottom = vbSouth
End Property
Public Property Get All() As Long
    All = vbAll
End Property
Public Property Get None() As Long
    None = vbNone
End Property
Public Property Get Horizontal() As Long
    Horizontal = vbHorizontal
End Property
Public Property Get Vertical() As Long
    Vertical = vbVertical
End Property
Public Property Get Both() As Long
    Both = vbBoth
End Property

Public Sub init(ByRef f As Form, Optional ByVal mode As Integer)
Set Dlg = f
minFormWidth = Dlg.Width
minFormHeight = Dlg.Height

If mode = vbAutomatic Then
    Dim ctrl As control
    For Each ctrl In Dlg.Controls
        add_by_tag ctrl
    Next ctrl
End If
End Sub
Public Sub start()
    initialized = 1
End Sub

Public Sub stopp()
    initialized = 0
End Sub
Public Sub setTag(ByVal tag As String)
    activeTag = tag
End Sub

Public Function get_param(ByVal p As String, ByRef value As String) As String
    Dim pos As Long
    If p = "" Then
        get_param = "": value = "": GoTo Ende
    End If
    If Len(p) < 2 Or Mid$(p, 1, 1) <> "-" Then
        get_param = "": value = p: GoTo Ende
    End If
    pos = InStr(2, p, "=")
    If pos > 0 Then
        get_param = Mid$(p, 2, pos - 2): value = Mid$(p, pos + 1): GoTo Ende
    Else
        get_param = Mid$(p, 2): value = "": GoTo Ende
    End If
Ende:
End Function
Public Sub add_by_tag(ByRef ctrl As control)
    Dim c As Resize_constraint
    Dim params As Variant
    Dim value As String
    
    Dim i As Integer
    
    If ctrl.tag = "" Then
        Exit Sub
    End If
    ' ctrl_tag auswerten
    Debug.Print "Tag von " & ctrl.name & ": " & CStr(ctrl.tag)
    params = split_string(ctrl.tag, " ")
    If IsNull(params) Then Exit Sub
    c.anchor = 0
    c.fill = vbNone ' wird überschrieben
    For i = 0 To UBound(params) - 1
        Select Case get_param(params(i), value)
            Case "vertical"
                c.fill = vbVertical
            Case "horizontal"
                c.fill = vbHorizontal
            Case "both"
                c.fill = vbBoth
            Case "right"
                c.anchor = c.anchor + vbEast
                c.Right = IIf(value = "", -1, CLng(value))
            Case "bottom"
                c.anchor = c.anchor + vbSouth
                c.Bottom = CLng(value)
            Case "tag"
                c.tag = value
            Case Else
        End Select
    Next i
    
    Set c.ctrl = ctrl
    c.Left = ctrl.Left
    c.Top = ctrl.Top
    c.parentWidth = ctrl.parent.Width
    c.parentHeight = ctrl.parent.Height
    ' Die Berechnung von C.Right kann falsch sein
    'C.Right = C.parentWidth - (ctrl.Left + diff + ctrl.Width)
    
'    add_constraint c
    ' Constraint einfügen
    constraint_count = constraint_count + 1
    ' Array vergrössern, sofern nötig
    If UBound(constraint_array) < constraint_count Then
        ReDim Preserve constraint_array(1 To constraint_count + 10)
    End If
    constraint_array(constraint_count) = c
    
End Sub

Public Sub add(ByRef ctrl As control, ByVal anchor As Long, ByVal fill As Long, Optional ByVal tag As String)
    Dim c As Resize_constraint
    Dim diff As Long
    'If IsEmpty(tag) Then
    '    tag = ""
    'End If
    
    ' TODO: Constraint prüfen (Freiheitsgrade!)
    ' falsch: CENTER UND !NONE
    ' falsch: HORIZONTAL UND Not (WEST And EAST)
    ' falsch: VERTICAL UND Not (NORTH And SOUTH)
    ' falsch: NONE UND nicht genau zwei Himmelsrichtungen
    
    ' Werte übernehmen
    ' Set c = New Resize_constraint
    c.anchor = anchor
    c.fill = fill
    Set c.ctrl = ctrl
    ' Default-Werte wie zur Design-Zeit übernehmen
    'C.Top = ctrl.Top
    'C.Left = ctrl.Left
    'C.Width = ctrl.Width
    'C.Height = ctrl.Height
    Debug.Print CStr(ctrl.name) & ".Parent(" & CStr(ctrl.parent.name) & ")=" & CStr(ctrl.parent.Width) & "x" & CStr(ctrl.parent.Height)
    Debug.Print CStr(ctrl.parent.Width) & "-" & CStr(ctrl.Left) & "-" & CStr(ctrl.Width)
'    If C.ctrl.Left < -10000 Then
'        diff = 75000
'    Else
'        diff = 0
'    End If
    c.Left = ctrl.Left
    c.Top = ctrl.Top
    c.parentWidth = ctrl.parent.Width
    c.parentHeight = ctrl.parent.Height
    ' Die Berechnung von C.Right kann falsch sein
    'C.Right = C.parentWidth - (ctrl.Left + diff + ctrl.Width)
    c.Right = -1 ' Initialwert
    c.Bottom = c.parentHeight - (ctrl.Top + ctrl.Height)
    c.tag = tag
    
'    add_constraint c
    ' Constraint einfügen
    constraint_count = constraint_count + 1
    ' Array vergrössern, sofern nötig
    If UBound(constraint_array) < constraint_count Then
        ReDim Preserve constraint_array(1 To constraint_count + 10)
    End If
    constraint_array(constraint_count) = c
    
End Sub
Public Sub resize()
    Dim i As Long
    
    If initialized = 0 Then
        Exit Sub
    End If
    
    ' TODO: Width und Height von Form überprüfen
    If Dlg.Width < minFormWidth Or Dlg.Height < minFormHeight Then
        ' dlg.Move dlg.Left, dlg.Top, min(dlg.Width, minFormWidth), min(dlg.Height, minFormHeight)
        Exit Sub
    End If
    
    For i = 1 To constraint_count
        ctrl_resize (i)
    Next i
End Sub
Private Sub ctrl_resize(ByVal idx As Long)
    Dim c As Resize_constraint
    Dim newTop As Long
    Dim newLeft As Long
    Dim newWidth As Long
    Dim newHeight As Long
    
    c = constraint_array(idx)
    If Not (c.tag = "" Or c.tag = activeTag) Then
        Exit Sub
    End If
    
    If c.Left <> c.ctrl.Left Then
        If c.Right = -1 Then
            ' Control wurde verschoben seit add-Methode, aber nicht von uns,
            ' sondern von z.B. SSTab => C.Right neu berechnen
            'js !!! LOG CStr(c.ctrl.name) + IIf(c.tag = "", "", "/" & c.tag) & ": C.Right wird neu berechnet !!!"
            c.Right = c.parentWidth - (c.ctrl.Left + c.ctrl.Width)
        Else
            ' HACK ???
            c.ctrl.Left = min(c.Left, c.ctrl.Left)
        End If
        c.Left = c.ctrl.Left
    End If
    ' Das gleiche für Top?
    
    newTop = c.ctrl.Top
    newLeft = c.ctrl.Left
    newWidth = c.ctrl.Width
    newHeight = c.ctrl.Height
    
    ' TODO: Fälle unterscheiden und ausrechnen
    ' Move, MoveResize, Resize
    If c.fill = vbBoth Then
        newWidth = c.ctrl.parent.Width - (c.ctrl.Left + c.Right)
        newHeight = c.ctrl.parent.Height - (c.ctrl.Top + c.Bottom)
    Else
        If c.fill = vbHorizontal Then
            newWidth = c.ctrl.parent.Width - (c.ctrl.Left + c.Right)
            If newWidth < 0 Then
                Stop
            End If
        End If
        If c.fill = vbVertical Then
            newHeight = c.ctrl.parent.Height - (c.ctrl.Top + c.Bottom)
        End If
        If c.fill = vbNone Then
            ' nix zu tun
        End If
        ' Anker auswerten
        If c.anchor = vbEast + vbSouth Then
            If c.ctrl.parent.Height - (newTop + newHeight) <> c.Bottom Then
                newTop = c.ctrl.parent.Height - (newHeight + c.Bottom)
            End If
            If c.ctrl.parent.Width - (newLeft + newWidth) <> c.Right Then
                newLeft = c.ctrl.parent.Width - (newWidth + c.Right)
            End If
        End If
        If c.anchor = vbSouth Then
            If c.ctrl.parent.Height - (newTop + newHeight) <> c.Bottom Then
                newTop = c.ctrl.parent.Height - (newHeight + c.Bottom)
            End If
        End If
        If c.anchor = vbEast Then
            If c.ctrl.parent.Width - (newLeft + newWidth) <> c.Right Then
                newLeft = c.ctrl.parent.Width - (newWidth + c.Right)
            End If
        End If
    End If
    ' Control verschieben/Resizen
    'LOG CStr(c.ctrl.name) + IIf(c.tag = "", "", "/" + c.tag) & ": " + CStr(c.ctrl.Left) & "+" + CStr(c.ctrl.Top) & "+" + CStr(c.ctrl.Width) & "x" + CStr(c.ctrl.Height) & " => " + CStr(newLeft) & "+" + CStr(newTop) + "+" + CStr(newWidth) + "x" + CStr(newHeight)
    'While newWidth < -10000
    '    newWidth = newWidth + 75000
    'Wend
    
    ' Sicherheitsabfrage
    If newWidth < 0 Then
        'LOG "FATAL: newWidth < 0: " + CStr(newWidth)
        Exit Sub ' Resizen nicht möglich
    End If
    If c.fill = vbVertical Or c.fill = vbBoth Then
        c.ctrl.move newLeft, newTop, newWidth, newHeight
    Else
        c.ctrl.move newLeft, newTop, newWidth
    End If
    ' aktuelle Werte merken
    c.Left = newLeft
    c.Right = c.ctrl.parent.Width - (c.ctrl.Left + c.ctrl.Width)
    c.Bottom = c.ctrl.parent.Height - (c.ctrl.Top + c.ctrl.Height)
    constraint_array(idx) = c
    
End Sub

Private Sub Class_Initialize()
    initialized = 0
    constraint_count = 0
    ReDim Preserve constraint_array(1 To constraint_count + 10)
End Sub
