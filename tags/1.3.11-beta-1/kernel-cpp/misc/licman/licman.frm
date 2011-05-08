VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.1#0"; "COMDLG32.OCX"
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.1#0"; "RICHTX32.OCX"
Object = "{00028C4A-0000-0000-0000-000000000046}#5.0#0"; "TDBG5.OCX"
Begin VB.Form Form1 
   Caption         =   "SOS --- Lizenzen Administrierung"
   ClientHeight    =   5790
   ClientLeft      =   90
   ClientTop       =   375
   ClientWidth     =   8235
   LinkTopic       =   "Form1"
   ScaleHeight     =   5790
   ScaleWidth      =   8235
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame1 
      Caption         =   "Sortieren"
      Height          =   1215
      Left            =   6840
      TabIndex        =   7
      Top             =   1200
      Width           =   1335
      Begin VB.CommandButton SortKey 
         Caption         =   "Schlüssel"
         Height          =   375
         Left            =   120
         TabIndex        =   9
         Top             =   720
         Width           =   1095
      End
      Begin VB.CommandButton SortHostname 
         Caption         =   "Name"
         Height          =   375
         Left            =   120
         TabIndex        =   8
         Top             =   240
         Width           =   1095
      End
   End
   Begin VB.CommandButton Pruefen 
      Caption         =   "Prüfen"
      Height          =   375
      Left            =   6960
      TabIndex        =   5
      Top             =   600
      Width           =   1095
   End
   Begin VB.CommandButton Speichern 
      Caption         =   "Speichern"
      Height          =   375
      Left            =   6960
      TabIndex        =   4
      Top             =   120
      Width           =   1095
   End
   Begin TrueDBGrid50.TDBGrid TDBGrid1 
      Height          =   5175
      Left            =   120
      OleObjectBlob   =   "licman.frx":0000
      TabIndex        =   1
      Top             =   120
      Width           =   6615
   End
   Begin RichTextLib.RichTextBox RichTextBox1 
      Height          =   255
      Left            =   6960
      TabIndex        =   3
      Top             =   5400
      Visible         =   0   'False
      Width           =   375
      _ExtentX        =   661
      _ExtentY        =   450
      _Version        =   327680
      Enabled         =   0   'False
      TextRTF         =   $"licman.frx":1E17
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   7440
      Top             =   5400
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   327680
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Abbrechen"
      Height          =   375
      Left            =   6960
      TabIndex        =   2
      Top             =   4920
      Width           =   1095
   End
   Begin VB.CommandButton OkButton 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   6960
      TabIndex        =   0
      Top             =   4440
      Width           =   1095
   End
   Begin VB.Label Status 
      Alignment       =   1  'Right Justify
      Height          =   255
      Left            =   4440
      TabIndex        =   10
      Top             =   5400
      Width           =   2175
   End
   Begin VB.Label Label1 
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   5400
      Width           =   3735
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

'Verwaltung von Lizenzschlüsseln

'©1997 SOS GmbH Berlin
'Jörg Schwiemann


'WAS FEHLT:
'-: Hilfetext

'+: Abbruch nach Änderung, nur nach Nachfrage
'=> tdbgrid1_changed(...)  => changed = true,, ok=> changed=false   speichernknopf aktivieren

'Fehlerzustand: Roten Fehlertext, dass nicht gesichert werden kann, anzeigen

'+: Nach Eingabe eines Hostnamens nur die Hostnamen, nicht die Schlüssel prüfen.
'Prüf-Routine dazu in zwei Teile aufteilen

'-: Totop oder für Tabelle

'+: Dateinamen in Caption zeigen. Erst Dateiname, dann "-" Lizenzmanager SOS GmbH

'-: Auslieferungsverfahren machen (geht nicht mit vb5cce)


'ZWEITE VERSION (WUNSCHLISTE) - ERST AUF ANFORDERUNG REALISIEREN.
'Neue Lizenzschlüssel über Diskette aufnehmen, evtl. mehrere pro Rechner
'hostname=lizenz1 lizenz2 sollte möglich sein

'+: Tabelle sollte bei Programmstart kommen, Öffnen-Dialog überlappt Tabelle. Zur besseren Orientierung.


'---------------------------------------------------------------------------

Const licence_double_bit = 2
Const host_double_bit = 1

Private MD_X, MD_Y As Single
Private RecordArray As New XArray
Private Zeilenende As String
Private DataChanged As Boolean
Private DataStore As Boolean
Private Initialized As Boolean
Private RowCount As Long
Private LastRow As Long
Private SortMode As Integer
Private FileName As String

Private Sub CancelButton_Click()
    Dim ret As VbMsgBoxResult
    
    If DataStore Then
        ret = MsgBox("Daten wurden geändert. Programm trotzdem verlassen?", vbQuestion + vbYesNo)
        If ret = vbYes Then
            End
        End If
    Else
        End
    End If
End Sub


Private Sub Form_Activate()
    TDBGrid1.SetFocus
End Sub

Private Sub Form_Load()
    Dim Args As Variant
     
    SetWaitCursor True
    ' App.HelpFile = "LICMAN.HLP"
    
    ' Kommentarspalte rot setzen
    TDBGrid1.Columns(2).ForeColor = vbRed
    
    ' Kommandozeile auslesen
    Args = Bef_Zeile_Abrufen(1)
    If Not IsNull(Args) Then
        FileName = Args(1)
    End If
    
    ' Keine Sortierung
    SortMode = 0
    ' Array initialisieren
    RecordArray.ReDim 0, -1, 0, 3
    Zeilenende = Chr$(13) + Chr$(10)
    
    ' js 10/12/97: Form1.Show
    Form1.Enabled = False
    If FileName = "" Or Not Exist_File(FileName) Then
        ' Set CancelError is True
        CommonDialog1.CancelError = True
        On Error GoTo ErrHandler
        ' Set flags
        CommonDialog1.Flags = cdlOFNHideReadOnly
        ' Set filters
        CommonDialog1.Filter = "Alle Dateien (*.*)|*.*|Lizenzdateien (*.ini;*.txt)" & _
        "|*.ini;*.txt"
        ' Specify default filter
        CommonDialog1.FilterIndex = 2
        ' Specify default Filename
        If FileName = "" Then
            FileName = "licence.ini"
        End If
        CommonDialog1.FileName = FileName ' !!! Kommandozeilenparamter auswerten
        ' Display the Open dialog box
        SetWaitCursor False
        Do
            CommonDialog1.ShowOpen
            If Exist_File(CommonDialog1.FileName) Then Exit Do
        Loop
        SetWaitCursor True
        ' Display name of selected file
        ' MsgBox CommonDialog1.FileName
        FileName = CommonDialog1.FileName
    End If
    
    On Error GoTo 0
    
    Form1.Caption = FileName + " --- Lizenzmanager (SOS GmbH Berlin)"
    Form1.Enabled = True
    'Form1.SetFocus
    Form1.Refresh
    DoEvents
    
    Read_Licence_File
    Initialized = True
    
    SetWaitCursor False
    Exit Sub
    
ErrHandler:
    'User pressed the Cancel button
'    MsgBox "Fehler aufgetreten: " + Err.Description
    End

End Sub


Private Sub Form_Resize()
'    Dim diff As Long
'    Dim buttonWidth As Long
'    Dim buttonHeight As Long
'    Dim buttonX As Long

'    ScaleMode = vbPixels
'    diff = 5
'    buttonWidth = 80
'    buttonHeight = 25
    
'    TDBGrid1.Move diff, diff, ScaleWidth - 3 * diff - buttonWidth, ScaleHeight - 4 * diff
'    buttonX = TDBGrid1.Width() + 2 * diff
'    Status.Move (TDBGrid1.Width() - Status.Width()) + diff, ScaleHeight - 3 * diff, Status.Width(), Status.Height()
'    Label1.Move diff, ScaleHeight - 3 * diff, TDBGrid1.Width() - Status.Width() - diff, Label1.Height()
    ' Buttons
'    Speichern.Move buttonX, buttonHeight + diff, buttonWidth, buttonHeight
'    Pruefen.Move buttonX, Speichern.Top() + Speichern.Height() + diff, buttonWidth, Pruefen.Height()
'    SortHostname.Move buttonX, Pruefen.Top() + Pruefen.Height() + diff, buttonWidth, SortHostname.Height()
'    SortKey.Move buttonX, SortHostname.Top() + SortHostname.Height() + diff, buttonWidth, SortKey.Height()
    
'    CancelButton.Move buttonX, TDBGrid1.Top() + TDBGrid1.Width() - CancelButton.Height(), buttonWidth, CancelButton.Height()
'    OkButton.Move buttonX, CancelButton.Top() + CancelButton.Width() - OkButton.Height(), buttonWidth, OkButton.Height()
    
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Dim ret As Long
    
    If DataStore Then   'Daten geändert?
        ret = MsgBox("Daten wurden geändert. Programm trotzdem verlassen?", vbQuestion + vbYesNo)
        If ret = vbYes Then
            End
        End If

        If Write_Licence_File = True Then
            End
         Else
            Cancel = True
        End If
    Else
        End
    End If
    
End Sub

Private Sub HelpButton_Click()
    ' Hilfe aktivieren ...
End Sub

Private Sub OkButton_Click()
    If DataStore Then   'Daten geändert?
        If Write_Licence_File = True Then
            End
        End If
     Else
        End
    End If
End Sub

Private Sub Read_Licence_File()
    Dim lf As String
    Dim Zeile As String
    Dim PC As String
    Dim Licence As String
    Dim Pos As Long
    Dim Pos2 As Long
    Dim LastPos As Long
    
    'On Error GoTo ErrHandler
    
    RichTextBox1.LoadFile FileName, rtfText

    RowCount = 0
    lf = RichTextBox1.Text
    Pos = 1
    LastPos = Pos
    Pos = InStr(Pos, lf, Zeilenende)
    While Pos <> 0
        Zeile = Trim$(Mid$(lf, LastPos, Pos - LastPos))
        ' Zeile auswerten
        If Mid$(Zeile, 1, 1) <> ";" Then
            Pos2 = InStr(1, Zeile, "=")
            If Pos2 <> 0 Then
                PC = Mid$(Zeile, 1, Pos2 - 1)
                If PC = "@" Then
                    PC = ""
                End If
                Licence = Mid$(Zeile, Pos2 + 1)
                RecordArray.Insert 1, RowCount
                RecordArray(RowCount, 0) = PC
                RecordArray(RowCount, 1) = Licence
                RecordArray(RowCount, 2) = ""
                RowCount = RowCount + 1
            End If
        End If
        Pos = Pos + 2
        LastPos = Pos
        Pos = InStr(Pos, lf, Zeilenende)
    Wend
    Host_Licence_Check
    Sort SortMode
    With TDBGrid1
        If .DataMode <> 4 Then MsgBox "tdbgrid.datamode <> 4"
        .Array = RecordArray
        .ReOpen
        .Refresh
    End With
    
    Exit Sub
    
End Sub
Private Function Lookup(ByVal a As XArray, ByVal col As Integer, ByVal str As String, ByVal MaxIndex As Long) As Long
    Dim i As Long
    Dim lstr As String
    Dim l As Long
    Dim vendor As String
    Dim kunde As String
    Dim lfdnr As String
    Dim rest As String
    Dim vendor2 As String
    Dim kunde2 As String
    Dim lfdnr2 As String
    Dim rest2 As String
    
    If col = 1 Then ' Schlüsselspalte
        SplitKey str, vendor, kunde, lfdnr, rest
    End If
    
    For i = RecordArray.LowerBound(1) To MaxIndex
        If col = 1 Then ' Schlüsselspalte
            SplitKey Trim$(RecordArray(i, col)), vendor2, kunde2, lfdnr2, rest2
            If vendor = vendor2 And kunde = kunde2 And Val(lfdnr) = Val(lfdnr2) Then
                Lookup = i
                Exit Function
            End If
        Else
            If str = Trim$(RecordArray(i, col)) Then
                Lookup = i
                Exit Function
            End If
        End If
    Next i
    Lookup = -1
End Function
Private Sub Host_Check(Auswertung As Boolean)
    Dim Pos As Long
    Dim i As Long
    
    SetWaitCursor True
    For i = RecordArray.LowerBound(1) To RecordArray.UpperBound(1)
        RecordArray(i, 2) = "" ' Zurücksetzen
        RecordArray(i, 3) = RecordArray(i, 3) And Not host_double_bit
    Next i
    ' Doppelte Rechnernamen überprüfen
    For i = RecordArray.LowerBound(1) + 1 To RecordArray.UpperBound(1)
        If RecordArray(i, 0) <> "" Then
            Pos = Lookup(RecordArray, 0, RecordArray(i, 0), i - 1)
            If Pos <> -1 Then
                RecordArray(i, 3) = RecordArray(i, 3) Or host_double_bit
                RecordArray(Pos, 3) = RecordArray(Pos, 3) Or host_double_bit
            End If
        End If
    Next i
    If Auswertung Then
        Marker_Auswerten
    End If
    SetWaitCursor False

End Sub
Private Sub Licence_Check()
    Dim Pos As Long
    Dim i As Long

    SetWaitCursor True
    For i = RecordArray.LowerBound(1) To RecordArray.UpperBound(1)
        RecordArray(i, 2) = "" ' Zurücksetzen
        RecordArray(i, 3) = RecordArray(i, 3) And Not licence_double_bit
    Next i
    ' Doppelte Lizenzen überprüfen
    For i = RecordArray.LowerBound(1) + 1 To RecordArray.UpperBound(1)
        Pos = Lookup(RecordArray, 1, RecordArray(i, 1), i - 1)
        If Pos <> -1 Then
            RecordArray(i, 3) = RecordArray(i, 3) Or licence_double_bit
            RecordArray(Pos, 3) = RecordArray(Pos, 3) Or licence_double_bit
        End If
    Next i
    
    Marker_Auswerten
    SetWaitCursor False
End Sub
Private Sub Marker_Auswerten()
    Dim i As Long
    Dim FreeCount As Long
    Dim Speicherbar As Boolean
    Dim s As String
    

    ' Marker auswerten
    Speicherbar = True
    FreeCount = 0
    
    For i = RecordArray.LowerBound(1) To RecordArray.UpperBound(1)
        RecordArray(i, 2) = ""
        If RecordArray(i, 3) And host_double_bit Then
            Speicherbar = False
            RecordArray(i, 2) = "Rechnername doppelt"
        End If
        If RecordArray(i, 3) And licence_double_bit Then
            Speicherbar = False
            If RecordArray(i, 3) And host_double_bit Then
                RecordArray(i, 2) = "Rechnername und Lizenz doppelt"
            Else
                RecordArray(i, 2) = "Lizenz doppelt"
            End If
        End If
        If RecordArray(i, 3) = 0 And RecordArray(i, 0) = "" Then
            FreeCount = FreeCount + 1
        End If
    Next i
        
    Speichern.Enabled = Speicherbar
    OkButton.Enabled = Speicherbar
    If Speicherbar Then
        Status.Caption = "Status: konsistent"
        Status.ForeColor = vbBlack
    Else
        Status.Caption = "Status: inkonsistent"
        Status.ForeColor = vbRed
    End If
    Label1.Caption = str$(RecordArray.Count(1)) + " Einträge, " + str$(FreeCount) + " freie Schlüssel"
    

End Sub

Private Sub Host_Licence_Check(Optional DeleteDoubleKeys As Boolean)
    Host_Check False
    Licence_Check
End Sub
Private Function Exist_File(ByVal f As String) As Boolean
    Dim fn As Integer
    ' Überprüfen ob BAK-Datei existiert und evtl. löschen
    On Error GoTo FileNotExist
    
    fn = FreeFile
    Open f For Input As #fn
    Close #fn
    
    Exist_File = True
    Exit Function
FileNotExist:
    Exist_File = False
   
End Function

Private Function Write_Licence_File() As Boolean
    SetWaitCursor True
    
    Dim ret As Long
    Dim i As Long
    Dim PC As String
    Dim out As String
    Dim FileNameTmp As String
    Dim FileNameBak As String
    Dim Pos As Long
    
    If DataStore Then
        TDBGrid1.Update
        Host_Licence_Check
    End If
    
    ' Dateinamen zusammenbauen
    Pos = InStr(Len(FileName) - 4, FileName, ".")
    If Pos <> 0 Then
        FileNameBak = Mid$(FileName, 1, Pos - 1)
        FileNameTmp = FileNameBak
        FileNameBak = FileNameBak + ".bak"
        FileNameTmp = FileNameTmp + ".tmp"
    Else
        FileNameBak = FileName + ".bak"
        FileNameTmp = FileName + ".tmp"
    End If
    
    out = "[licence]" + Zeilenende + "; DO NOT CHANGE: Diese Datei wurde automatisch mit licman geschrieben." + Zeilenende
    
    For i = RecordArray.LowerBound(1) To RecordArray.UpperBound(1)
        If RecordArray(i, 3) <> 0 Then ' irgendwas ist falsch
            ret = MsgBox("Speichern nicht möglich, da noch Inkonsistenzen vorhanden sind!")
            Write_Licence_File = False
            GoTo ENDE
        Else
            PC = Trim$(RecordArray(i, 0))
            If PC = "" Then
                PC = "@"
            End If
            out = out + PC + "=" + Trim$(RecordArray(i, 1)) + Zeilenende
        End If
    Next i
    RichTextBox1.Text = out
    
    ' Speichern und Umbenennen
    RichTextBox1.SaveFile FileNameTmp, rtfText
    
    
    If Exist_File(FileNameBak) = True Then
        Kill FileNameBak
    End If
    
    Name FileName As FileNameBak
    Name FileNameTmp As FileName
    
    DataStore = False   'Daten sind nicht mehr geändert und müssen nicht gespeichert werden.
    
    Write_Licence_File = True
    
ENDE:
     SetWaitCursor False

End Function


Private Sub Pruefen_Click()
    SetWaitCursor True
    TDBGrid1.Update
    Host_Licence_Check
    TDBGrid1.ReOpen
    SetWaitCursor False
End Sub

Private Sub SortHostname_Click()
    SortMode = 1
    Sort SortMode
    TDBGrid1.ReOpen
End Sub

Private Sub SortKey_Click()
    SortMode = 2
    Sort SortMode
    TDBGrid1.ReOpen
End Sub

Private Sub Speichern_Click()
    If Write_Licence_File = False Then
        'MsgBox "Datei konnte nicht gespeichert werden"
    End If
End Sub

Private Sub TDBGrid1_AfterColUpdate(ByVal ColIndex As Integer)
    DataChanged = True
    DataStore = True
    ' MsgBox "tdbgrid1_AfterColUpdate " + CStr(ColIndex)

End Sub

Private Sub TDBGrid1_RowEdit(ByVal ColIndex As Integer)
    DataChanged = True
    DataStore = True
    LastRow = ColIndex
End Sub

Private Sub TDBGrid1_HeadClick(ByVal ColIndex As Integer)
    Debug.Print "TDBGrid1_HeadClick: ColIndex=" + CStr(ColIndex)
    'MsgBox "TDBGrid1_HeadClick: ColIndex=" + CStr(ColIndex)
    ' TDBGrid liefert immer Col=0!
    'MsgBox "ButtonClick, Col=" + CStr(ColIndex) + "," + CStr(MD_X) + "," + CStr(MD_Y)
    
    ' Todo: col = compute_col(MD_X,MD_Y),
    ' Sortieren (NAME_ASC,NAME_DESC,KEY_ASC,KEY_DESC,NONE)
    ' sort( col, last_sort )
End Sub

Private Sub TDBGrid1_LostFocus()
    TDBGrid1.Update
    ' ??? Host_Licence_Check
End Sub

Private Sub TDBGrid1_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    MD_X = X
    MD_Y = Y
End Sub

Private Sub TDBGrid1_RowColChange(LastRow As Variant, ByVal LastCol As Integer)
Static rec As Boolean
    If rec Then
        Exit Sub
    End If
    rec = True
    SetWaitCursor True
    If Initialized And DataChanged Then
        DataChanged = False
        'TDBGrid1.Update
        If Trim$(RecordArray(LastRow, 0)) = "" And (RecordArray(LastRow, 3) And licence_double_bit) <> 0 Then
            ' Lizenz doppelt => Zeile kann gelöscht werden!
            RecordArray.Delete 1, LastRow
        Else ' neuer Eintrag?
            Host_Check True
        End If
        TDBGrid1.ReOpen
        TDBGrid1.SetFocus
    End If
    rec = False
    SetWaitCursor False
   
End Sub

Private Function Bef_Zeile_Abrufen(Optional MaxArgs) As Variant

    ' Variablen deklarieren.
    Dim C, BefZl, BefZlLng, i, ZahlArgs
    Dim NumArgs As Integer
    Dim InArg As Boolean
    
    ' Feststellen, ob MaxArgs angegeben wurde.
    If IsMissing(MaxArgs) Then MaxArgs = 10
    ' Datenfeld passender Größe erstellen.
    ReDim ArgFeld(MaxArgs)
    NumArgs = 0: InArg = False
    ' Befehlszeilenargumente abrufen.
    BefZl = Command()
    BefZlLng = Len(BefZl)
    ' Befehlszeile Zeichen für Zeichen
    ' durchgehen.
    For i = 1 To BefZlLng

        C = Mid(BefZl, i, 1)
        ' Auf Leerzeichen oder Tabulatoren prüfen.
        If (C <> " " And C <> vbTab) Then
            ' Weder Leerzeichen noch Tabulatoren.
            ' Überprüfen ob bereits in Argument enthalten.
            If Not InArg Then
            ' Anfang des neuen Arguments.
            ' Überprüfen, ob zu viele Argumente verwendet wurden.
                If ZahlArgs = MaxArgs Then Exit For
                    ZahlArgs = ZahlArgs + 1
                    InArg = True
                End If
            ' Zeichen dem aktuellem Argument hinzufügen.

            ArgFeld(ZahlArgs) = ArgFeld(ZahlArgs) + C
        Else
            ' Tabulator oder Leerzeichen gefunden.
            ' Das InArg-Attribut auf False festlegen.
            InArg = False
        End If
    Next i
    ' Größe des Datenfeldes neu bestimmen, so daß es
    ' gerade alle Argumente aufnehmen kann.
    If ZahlArgs = 0 Then
        Bef_Zeile_Abrufen = Null
    Else
        ReDim Preserve ArgFeld(ZahlArgs)
        ' Datenfeld in Funktionsnamen zurückgeben.
        Bef_Zeile_Abrufen = ArgFeld()
    End If
    
End Function

Private Sub SetWaitCursor(b As Boolean)
    If b Then
        Form1.MousePointer = 11 ' Sanduhr
    Else
        Form1.MousePointer = 0  ' normal
    End If
End Sub

Private Sub Sort(ByVal Mode As Integer)
    If Mode > 0 Then
        SetWaitCursor True
        Sort2DXArray RecordArray, Mode - 1
        SetWaitCursor False
    End If
End Sub

Private Sub Sort2DXArray(ByRef a As XArray, col As Integer)
    ' SortierAlgorithmus: ZschimmerSort?
    Dim i, j, m As Long
    Dim k As Integer
    Dim current As String
    
    For i = a.LowerBound(1) To a.UpperBound(1) - 1
        m = i
        For j = i + 1 To a.UpperBound(1)
            Dim tmp As String
            ' Todo: Deutsche Umlaute in Kleiner-Operator
            ' Leere Hostnamen am Ende
            ' 3.Teil des Schlüssel numerisch vergleichen, Rest alphabetisch
            If str_lt(Trim$(a(j, col)), Trim$(a(m, col)), col) Then
            ' TEST: If str_lt_func(Trim$(a(j, col)), Trim$(a(m, col))) Then
                m = j
            End If
        Next j
        ' i und m vertauschen
        If m > i Then
            For k = a.LowerBound(2) To a.UpperBound(2)
                tmp = a(m, k)
                a(m, k) = a(i, k)
                a(i, k) = tmp
            Next k
        End If
    Next i
End Sub

Private Function str_lt(ByVal str1 As String, ByVal str2 As String, col As Integer) As Boolean
    ' Kleiner-Funktion mit dt. Umlauten
    '
    If col = 0 Then ' Hostname
        If str1 = "" And str2 = "" Then
            str_lt = True  ' Nicht Vertauschen
        Else
            If str1 = "" Then
                str_lt = False ' Vertauschen
            Else
                If str2 = "" Then
                    str_lt = True ' Nicht Vertauschen
                Else
                    str_lt = str1 < str2 ' Normaler alphabetischer Vergleich
                End If
            End If
        End If
    Else
        If col = 1 Then
            Dim v1 As String
            Dim v2 As String
            Dim k1 As String
            Dim k2 As String
            Dim l1 As String
            Dim l2 As String
            Dim r1 As String
            Dim r2 As String
            
            SplitKey str1, v1, k1, l1, r1
            SplitKey str2, v2, k2, l2, r2
            
            str_lt = v1 < v2 Or (v1 = v2 And k1 < k2) Or (v1 = v2 And k1 = k2 And Val(l1) < Val(l2)) Or (v1 = v2 And k1 = k2 And Val(l1) = Val(l2) And r1 < r2)
            
        Else
            str_lt = str1 < str2 ' Für den Rest normal
        End If
    End If
End Function

Private Sub SplitKey(ByVal str As String, vendor As String, kunde As String, lfdnr As String, rest As String)
    Dim p As Long
    Dim l As Long
    
    ' Bsp.: SOS-LBANK-101-xxxx-yyy
    p = InStr(1, str, "-")
    vendor = Mid$(str, 1, p - 1)
    l = p + 1
    p = InStr(l, str, "-")
    kunde = Mid$(str, l, p - l)
    l = p + 1
    p = InStr(l, str, "-")
    lfdnr = Mid$(str, l, p - l)
    rest = Mid$(str, p + 1)
    
End Sub

