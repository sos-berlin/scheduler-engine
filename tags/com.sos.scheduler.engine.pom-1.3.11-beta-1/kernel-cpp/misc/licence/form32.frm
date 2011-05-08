VERSION 5.00
Begin VB.Form Form1 
   BorderStyle     =   3  'Fester Dialog
   Caption         =   "Lizenzgenerator"
   ClientHeight    =   9495
   ClientLeft      =   1170
   ClientTop       =   465
   ClientWidth     =   8625
   BeginProperty Font 
      Name            =   "MS Sans Serif"
      Size            =   8.25
      Charset         =   0
      Weight          =   700
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   ForeColor       =   &H80000008&
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   PaletteMode     =   1  'ZReihenfolge
   ScaleHeight     =   9495
   ScaleWidth      =   8625
   Begin VB.ComboBox aussteller 
      Height          =   315
      Left            =   120
      Style           =   2  'Dropdown-Liste
      TabIndex        =   1
      Top             =   360
      Width           =   1215
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   31
      Left            =   120
      TabIndex        =   47
      Top             =   7320
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   23
      Left            =   120
      TabIndex        =   46
      Top             =   7560
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   24
      Left            =   120
      TabIndex        =   45
      Top             =   7800
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   25
      Left            =   120
      TabIndex        =   44
      Top             =   8040
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   26
      Left            =   120
      TabIndex        =   43
      Top             =   8280
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   27
      Left            =   120
      TabIndex        =   42
      Top             =   8520
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   28
      Left            =   120
      TabIndex        =   41
      Top             =   8760
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   29
      Left            =   120
      TabIndex        =   40
      Top             =   9000
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   30
      Left            =   120
      TabIndex        =   39
      Top             =   9240
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox alles 
      Caption         =   "Alles freischalten"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   38
      Top             =   1440
      Width           =   4444
   End
   Begin VB.TextBox anzahl 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   4800
      TabIndex        =   5
      Text            =   "1"
      Top             =   360
      Width           =   855
   End
   Begin VB.CommandButton Command1 
      Caption         =   "In die Zwischenablage!"
      Default         =   -1  'True
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   4800
      TabIndex        =   36
      Top             =   840
      Width           =   2295
   End
   Begin VB.CheckBox solaris 
      Caption         =   "&Solaris"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
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
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1440
      TabIndex        =   9
      Top             =   840
      Width           =   1095
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   22
      Left            =   120
      TabIndex        =   33
      Top             =   7080
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   21
      Left            =   120
      TabIndex        =   32
      Top             =   6840
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   20
      Left            =   120
      TabIndex        =   31
      Top             =   6600
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   19
      Left            =   120
      TabIndex        =   30
      Top             =   6360
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   18
      Left            =   120
      TabIndex        =   29
      Top             =   6120
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   17
      Left            =   120
      TabIndex        =   28
      Top             =   5880
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   16
      Left            =   120
      TabIndex        =   27
      Top             =   5640
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   15
      Left            =   120
      TabIndex        =   26
      Top             =   5400
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   14
      Left            =   120
      TabIndex        =   25
      Top             =   5160
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   13
      Left            =   120
      TabIndex        =   24
      Top             =   4920
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   12
      Left            =   120
      TabIndex        =   23
      Top             =   4680
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   11
      Left            =   120
      TabIndex        =   22
      Top             =   4440
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   10
      Left            =   120
      TabIndex        =   21
      Top             =   4200
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   9
      Left            =   120
      TabIndex        =   20
      Top             =   3960
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   8
      Left            =   120
      TabIndex        =   19
      Top             =   3720
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   7
      Left            =   120
      TabIndex        =   18
      Top             =   3480
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   6
      Left            =   120
      TabIndex        =   17
      Top             =   3240
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   5
      Left            =   120
      TabIndex        =   16
      Top             =   3000
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   4
      Left            =   120
      TabIndex        =   15
      Top             =   2760
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   3
      Left            =   120
      TabIndex        =   14
      Top             =   2520
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   2
      Left            =   120
      TabIndex        =   13
      Top             =   2280
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   12
      Top             =   2040
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.CheckBox Check1 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   11
      Top             =   1800
      Visible         =   0   'False
      Width           =   4444
   End
   Begin VB.TextBox gueltig_bis 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   5880
      TabIndex        =   8
      Top             =   360
      Width           =   1215
   End
   Begin VB.TextBox nr 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   2760
      TabIndex        =   4
      Top             =   360
      Width           =   735
   End
   Begin VB.TextBox kunde 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   285
      Left            =   1440
      TabIndex        =   2
      Text            =   "DEMO"
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label key_label 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2775
      Left            =   4800
      TabIndex        =   48
      Top             =   1440
      Width           =   3615
   End
   Begin VB.Label Label7 
      Caption         =   "Anzahl"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   4800
      TabIndex        =   37
      Top             =   120
      Width           =   1095
   End
   Begin VB.Label Label6 
      Caption         =   "Plattformen:"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
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
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
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
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   5880
      TabIndex        =   7
      Top             =   120
      Width           =   1455
   End
   Begin VB.Label Label3 
      Caption         =   "Aussteller"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   120
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "&Nummer"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   2760
      TabIndex        =   3
      Top             =   120
      Width           =   735
   End
   Begin VB.Label Label1 
      Caption         =   "&Kundenkürzel"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   1440
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim key_file_name As String
Dim product_count As Integer
Dim max_product_count As Integer
Dim in_load_products As Boolean



Sub load_products()

Dim products  As New hostWare.file
Dim record As hostWare.Dyn_obj


On Error GoTo FEHLER

reset_products

If in_load_products Then Exit Sub
in_load_products = True

alles.Visible = aussteller = "SOS"
If Not alles.Visible Then alles = 0

products.open "-in licence -products -aussteller=" & aussteller
While Not products.EOF
    Set record = products.get()
    If record!code <> "19" And record!code <> "20" And record!code <> "1A" Then
        If record!code <> "10" Or aussteller = "SOS" Then add_product record!code, record!name
    End If
Wend

GoTo FERTIG

FEHLER:
    'MsgBox Err.Description
    key_label = Err.Description
    key_label.FontBold = False


FERTIG:
in_load_products = False

End Sub


Sub add_product(ByVal code As String, ByVal name As String)

Me.Check1(product_count).Tag = code
If Len(code) = 1 Then code = "&" + code
Me.Check1(product_count).Caption = code & "  " & name
Me.Check1(product_count).Visible = True

product_count = product_count + 1

If max_product_count < product_count Then max_product_count = product_count

End Sub


Sub reset_products()

Dim i As Integer

For i = 1 To Me.Check1.ubound
    Me.Check1(i).Visible = False
Next i

product_count = 0

End Sub

#If 0 Then
Sub load_key_file()

Dim file    As New hostWare.file
Dim record  As hostWare.Dyn_obj

On Error GoTo FEHLER

key_file_error_label = ""

reset_products

file.open "-in record/tabbed -field-names -tab=';' | " & key_file_edit

Do
    If file.EOF() Then Exit Do
    Set record = file.get()
    add_product record!key, record!name
Loop

file.Close
adjust_size

Exit Sub

FEHLER:
    key_file_error_label = Err.Description

End Sub
#End If


Sub adjust_size()

Me.Height = Me.Check1(max_product_count).Top + Me.Check1(max_product_count).Height + 500

End Sub



Private Sub alles_Click()

Dim i As Long

alles.FontBold = alles <> 0

'For i = Check1.lbound To Check1.ubound
'    Check1(i).Enabled = alles = 0
'Next

show_key

End Sub

Private Sub aussteller_Change()
    
Dim lics   As New hostWare.file
Dim record As hostWare.Dyn_obj

If in_load_products Then Exit Sub
load_products

End Sub




Private Sub aussteller_Click()

load_products
show_key

End Sub



Private Function open_key(ByVal count As Long) As hostWare.file

Dim file    As New hostWare.file
Dim fn      As String
Dim keys    As String
Dim i       As Integer


fn = "-in licence_key -aussteller=" + Me.aussteller
fn = fn + " -kunde=" & Me.kunde
If nr <> "" Then fn = fn & " -nr=" + Me.nr
fn = fn + " -count=" & CStr(count)

If Me.alles Then fn = fn + " -ZZ"
    
For i = 0 To product_count - 1
    If Me.Check1(i).Value Then fn = fn + " -" + Me.Check1(i).Tag
Next i

If Me.windows.Value Or Me.solaris.Value Then
    fn = fn + " -1A="
    If Me.windows.Value Then fn = fn + "W"
    If Me.solaris.Value Then fn = fn + "S"
End If

If Me.gueltig_bis <> "" Then
    fn = fn + " -20='" + Me.gueltig_bis + "'"
End If

file.open fn

Set open_key = file

End Function


Private Sub Command1_Click()

Dim file    As New hostWare.file
Dim fn      As String
Dim keys    As String
Dim i       As Integer

On Error GoTo FEHLER

Set file = open_key(Me.anzahl)

Do
    If keys <> "" Then keys = keys + Chr$(13) + Chr$(10)

    If file.EOF() Then Exit Do
    keys = keys + file.get()
Loop

file.Close

Clipboard.Clear
Clipboard.SetText keys

Exit Sub

FEHLER:
    MsgBox "Fehler:" & Err.Description

End Sub

Private Sub add_aussteller(ByVal a As String)

Dim products As New hostWare.file

On Error GoTo FERTIG

products.open "-in licence -products -aussteller=" & a
aussteller.AddItem a
aussteller.ListIndex = aussteller.ListCount - 1  'load_products veranlassen

FERTIG:

End Sub



Private Sub Form_Load()

Dim file As New hostWare.file
Dim lics As New hostWare.file
Dim key    As String
Dim a      As String
Dim i      As Long

aussteller.Clear

file.open "null :"     ' Ist hostole da?

On Error GoTo NO_LICENCE
lics.open "-in licence -installed-keys"
On Error Resume Next

While Not lics.EOF
    key = lics.get_line()
    a = Mid$(key, InStr(key, "-") + 1)
    a = Left$(a, InStr(a, "-") - 1)
    For i = 1 To aussteller.ListCount
        If aussteller.List(i) = a Then Exit For
    Next
    If i > aussteller.ListCount Then add_aussteller a
Wend

If aussteller.ListCount = 0 Then GoTo NO_LICENCE

aussteller.ListIndex = 0

load_products
adjust_size

'date_label = Date
'time_label = Time()

show_key
Exit Sub

NO_LICENCE:
Err.Raise vbError + 1, , "Es liegt kein Lizenzschüssel für den Lizenzgenerator vor"

End Sub

#If 0 Then
Private Sub key_file_edit_Change()

load_key_file

End Sub
#End If



Private Sub gueltig_bis_Change()

show_key

End Sub


Private Sub kunde_Change()

Dim i As Integer
Dim c As String * 1


If Len(Me.kunde) = 0 Then GoTo KC_FALSCH

For i = 1 To Len(Me.kunde)
    c = UCase$(Mid$(Me.kunde, i, 1))
    If c >= "A" And c <= "Z" Or c >= "0" And c <= "9" Then
        'ok
    Else
        GoTo KC_FALSCH
    End If
Next i

Me.kunde.BackColor = &H80000005
GoTo KC_ENDE

KC_FALSCH:
Me.kunde.BackColor = vbYellow

KC_ENDE:
show_key

End Sub


Private Sub show_key()

Dim file As hostWare.file

On Error GoTo FEHLER

Set file = open_key(1)

key_label = file.get()
key_label.FontBold = True
'key_label.ForeColor = &H80000012
Exit Sub

FEHLER:
    'key_label.ForeColor = vbRed
    key_label = Err.Description
    key_label.FontBold = False

End Sub

Private Sub nr_Change()

show_key

End Sub

Private Sub solaris_Click()

show_key

End Sub

Private Sub windows_Click()

show_key

End Sub


Private Sub Check1_Click(Index As Integer)

    show_key

End Sub

