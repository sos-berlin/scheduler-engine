VERSION 4.00
Begin VB.Form Form1 
   Caption         =   "Lizenzgenerator"
   ClientHeight    =   7485
   ClientLeft      =   1185
   ClientTop       =   480
   ClientWidth     =   6915
   BeginProperty Font 
      name            =   "MS Sans Serif"
      charset         =   1
      weight          =   700
      size            =   8.25
      underline       =   0   'False
      italic          =   0   'False
      strikethrough   =   0   'False
   EndProperty
   ForeColor       =   &H80000008&
   Height          =   7890
   Left            =   1125
   LinkTopic       =   "Form1"
   ScaleHeight     =   7485
   ScaleWidth      =   6915
   Top             =   135
   Width           =   7035
   Begin VB.CheckBox alles 
      Caption         =   "Alles freischalten"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   38
      Top             =   1440
      Width           =   3735
   End
   Begin VB.TextBox anzahl 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   4080
      TabIndex        =   4
      Text            =   "1"
      Top             =   360
      Width           =   975
   End
   Begin VB.CommandButton Command1 
      Caption         =   "in die Zwischenablage!"
      Default         =   -1  'True
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   4080
      TabIndex        =   36
      Top             =   1320
      Width           =   2295
   End
   Begin VB.CheckBox solaris 
      Caption         =   "&Solaris"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   2640
      TabIndex        =   10
      Top             =   840
      Width           =   1215
   End
   Begin VB.CheckBox windows 
      Caption         =   "&Windows"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1440
      TabIndex        =   9
      Top             =   840
      Width           =   1095
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   22
      Left            =   120
      TabIndex        =   33
      Top             =   7080
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   21
      Left            =   120
      TabIndex        =   32
      Top             =   6840
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   20
      Left            =   120
      TabIndex        =   31
      Top             =   6600
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   19
      Left            =   120
      TabIndex        =   30
      Top             =   6360
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   18
      Left            =   120
      TabIndex        =   29
      Top             =   6120
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   17
      Left            =   120
      TabIndex        =   28
      Top             =   5880
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   16
      Left            =   120
      TabIndex        =   27
      Top             =   5640
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   15
      Left            =   120
      TabIndex        =   26
      Top             =   5400
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   14
      Left            =   120
      TabIndex        =   25
      Top             =   5160
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   13
      Left            =   120
      TabIndex        =   24
      Top             =   4920
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   12
      Left            =   120
      TabIndex        =   23
      Top             =   4680
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   11
      Left            =   120
      TabIndex        =   22
      Top             =   4440
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   10
      Left            =   120
      TabIndex        =   21
      Top             =   4200
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   9
      Left            =   120
      TabIndex        =   20
      Top             =   3960
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   8
      Left            =   120
      TabIndex        =   19
      Top             =   3720
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   7
      Left            =   120
      TabIndex        =   18
      Top             =   3480
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   6
      Left            =   120
      TabIndex        =   17
      Top             =   3240
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   5
      Left            =   120
      TabIndex        =   16
      Top             =   3000
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   4
      Left            =   120
      TabIndex        =   15
      Top             =   2760
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   3
      Left            =   120
      TabIndex        =   14
      Top             =   2520
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   2
      Left            =   120
      TabIndex        =   13
      Top             =   2280
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   12
      Top             =   2040
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   11
      Top             =   1800
      Visible         =   0   'False
      Width           =   3735
   End
   Begin VB.TextBox gueltig_bis 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   5520
      TabIndex        =   8
      Top             =   360
      Width           =   1215
   End
   Begin VB.TextBox aussteller 
      Enabled         =   0   'False
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   120
      TabIndex        =   6
      Text            =   "SOS"
      Top             =   360
      Width           =   975
   End
   Begin VB.TextBox nr 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   2520
      TabIndex        =   3
      Top             =   360
      Width           =   735
   End
   Begin VB.TextBox kunde 
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   1200
      TabIndex        =   1
      Text            =   "DEMO"
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label time_label 
      Caption         =   "Label9"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   0
         weight          =   700
         size            =   13.5
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4200
      TabIndex        =   41
      Top             =   3600
      Width           =   2295
   End
   Begin VB.Label date_label 
      Caption         =   "Label9"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   0
         weight          =   700
         size            =   13.5
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4200
      TabIndex        =   40
      Top             =   3000
      Width           =   2295
   End
   Begin VB.Label Label8 
      Caption         =   "Produktliste steht in j:\data\lic-keys.txt"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   4080
      TabIndex        =   39
      Top             =   1920
      Width           =   2895
   End
   Begin VB.Label Label7 
      Caption         =   "Anzahl"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   4080
      TabIndex        =   37
      Top             =   120
      Width           =   1095
   End
   Begin VB.Label Label6 
      Caption         =   "Plattformen:"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   35
      Top             =   840
      Width           =   1095
   End
   Begin VB.Label Label5 
      Caption         =   "Freigeschaltete Produkte"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   34
      Top             =   1200
      Width           =   3735
   End
   Begin VB.Label Label4 
      Caption         =   "&Gültig bis"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   5520
      TabIndex        =   7
      Top             =   120
      Width           =   1455
   End
   Begin VB.Label Label3 
      Caption         =   "Aussteller"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   120
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "&Nummer"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   2520
      TabIndex        =   2
      Top             =   120
      Width           =   735
   End
   Begin VB.Label Label1 
      Caption         =   "&Kundenkürzel"
      BeginProperty Font 
         name            =   "MS Sans Serif"
         charset         =   1
         weight          =   400
         size            =   8.25
         underline       =   0   'False
         italic          =   0   'False
         strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1200
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Option Explicit

Dim hostapi As Long

Dim prod_count As Integer

Private Sub Command1_Click()

Dim file As Long
Dim fn As String
Dim text As String
Dim keys As String
Dim ln As Integer

fn = "licence_key -aussteller=" + Me.aussteller
fn = fn + " -kunde=" + Me.kunde
If nr <> "" Then fn = fn + " -nr=" + Me.nr
fn = fn + " -count=" + Me.anzahl

If Me.alles Then
    fn = fn + " -ZZ"
Else
    Dim i As Integer
    For i = 0 To prod_count - 1
        If Me.Check1(i).value Then fn = fn + " -" + Me.Check1(i).Tag
    Next i
End If

If Me.windows.value Or Me.solaris.value Then
    fn = fn + " -1A="
    If Me.windows.value Then fn = fn + "W"
    If Me.solaris.value Then fn = fn + "S"
End If

If Me.gueltig_bis <> "" Then
    fn = fn + " -19='" + Me.gueltig_bis + "'"
End If

file = sos_open(hostapi, fn, 1)
If file = 0 Then GoTo C1_HOSTAPI_FEHLER

Do
    If keys <> "" Then keys = keys + Chr$(13) + Chr$(10)

    text = Space$(100)
    ln = sos_get(file, text, Len(text))
    If ln = -1 Then Exit Do
    If ln < 0 Then GoTo C1_HOSTAPI_FEHLER

    keys = keys + Left$(text, ln)
Loop

Clipboard.Clear
Clipboard.SetText keys

GoTo C1_ENDE

C1_HOSTAPI_FEHLER:
    text = Space$(300)
    ln = sos_get_error_text(hostapi, text, Len(text))
    MsgBox "Fehler: " + Left$(text, ln)

C1_ENDE:

ln = sos_close(file)

End Sub

Private Sub Form_Load()

Dim file As Long
Dim text As String
Dim er As Long
Dim i As Integer
Dim code As String

date_label = Date
time_label = Time()

hostapi = sos_init()

file = sos_open(hostapi, "record/tabbed -field-names -tab=';' | j:\data\lic-keys.txt", 1)
If file = 0 Then GoTo HOSTAPI_FEHLER

Do
    er = sos_get(file, text, -1)
    If er = -1 Then Exit Do
    If er < 0 Then GoTo HOSTAPI_FEHLER

    code = sos_read_field_as_string$(file, "key")
    Me.Check1(i).Tag = code
    If Len(code) = 1 Then code = "&" + code
    Me.Check1(i).Caption = code + "  " + sos_read_field_as_string$(file, "name")
    Me.Check1(i).Visible = True

    i = i + 1
Loop

prod_count = i

i = i - 1
Me.Height = Me.Check1(i).Top + Me.Check1(i).Height + 500

GoTo FL_ENDE

HOSTAPI_FEHLER:
text = Space$(300)
er = sos_get_error_text(hostapi, text, Len(text))
MsgBox "Fehler: " + Left$(text, er)

FL_ENDE:

er = sos_close(file)
End Sub

Private Sub Form_Unload(Cancel As Integer)

Dim er As Long

er = sos_exit(hostapi)

End Sub

Private Sub kunde_Change()

Dim i As Integer
Dim c As String * 1


For i = 1 To Len(Me.kunde)
    c = Mid$(Me.kunde, i, 1)
    If Not (c >= "a" And c <= "z" Or c >= "0" Or c <= "9") Then GoTo KC_FALSCH
Next i

Me.kunde.ForeColor = &HC0&
GoTo KC_ENDE

KC_FALSCH:
Me.kunde.ForeColor = &H80000005

KC_ENDE:

End Sub

