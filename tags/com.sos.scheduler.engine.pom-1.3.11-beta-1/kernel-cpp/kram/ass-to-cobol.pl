# $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
#
# Macht aus einer Assembler-Datenbeschreibung eine Vorlage für eine Cobol-Datenbeschreibung
 

my $line_nr = 0;
%variables = ();        # Für SETC

while(<>)
{
    $line_nr++;
    my $line = $_;
    $line =~ s/\r?\n?$//;
    if( $line =~ /^[0-9]/ ) { $line = substr( $line, 8 ); }   # ISAM-Schlüssel abschneiden
    
    my ( $name, $instr, $operand, $comment ) = split( / +/, $line );   # Wir erwarten kein Blank in einem String
    $name =~ s/ *$//g;

    print substr( "      *     " . lc( $line ), 0, 70 ) . "\n";  # Max. 71 Spalten
    #print "      *   " . substr( "$name        ", 1, 8 ) . " $instr  $operand   $comment\n";
    
    
    if( substr( $name, 0, 1 ) ne "*"  &&
        substr( $name, 0, 1 ) ne "."     )
    {
        if( $instr eq 'SETC' )
        {
            my $value = $operand;
            $value =~ s/'//g;       #  Wir nehmen einen String ohne Blank an, nur ein Zeichen
            $variables{ $name } = $value;
        }

        if( $instr eq 'AGO'    ||
            $instr eq 'AIF'    ||
            $instr eq 'ANOP'   ||
            $instr eq 'DSECT'  ||
            $instr eq 'EJECT'  ||
            $instr eq 'EQU'    ||
            $instr eq 'MACRO'  ||
            $instr eq 'MEND'   ||
            $instr eq 'MEXIT'  ||
            $instr eq 'MNOTE'  ||
            $instr eq 'ORG'    ||
            $instr eq 'SETC'   ||
            $instr eq 'SPACE'  ||
            $instr eq 'SPACE2' ||
            $instr eq 'SPACE3' ||
            $instr eq 'SPACE4'   )
        {
            # Diese Anweisungen sind uns egal.
            # ORG muss aber beachtet werden, wenn es nicht am Ende steht!
        }
        elsif( $instr eq "DS"  || 
               $instr eq "DC"    ) 
        {
            foreach my $variable_name ( keys %variables )       # &-Variablen ersetzen
            {
                my $variable_value = $variables{ $variable_name };
                $name =~ s/$variable_name./$variable_value/g;       # A&VAR.B
                $name =~ s/$variable_name$/$variable_value/g;       # A&VAR
            }
            
            my $type = $operand;       # Von $type werden Salamischeiben abgeschnitten
            
            $type =~ s/^([0-9]+)//;
            my $wdh = defined $1? 1 * $1 : 1;
            
            my $type_letter = substr( $type, 0, 1 );
            $type = substr( $type, 1 );
            
            my $length = $type_letter eq 'A'? 4 :
                         $type_letter eq 'F'? 4 :
                         $type_letter eq 'H'? 2 :
                         $type_letter eq 'Y'? 2 
                                            : 1;
                                            
            my $cobol_type = $type;

            if( $type =~ /^L/ )
            {
                $type =~ s/^L(([0-9]+)|[(][^)]+[)])//;   # L123 oder L(...)
                $length = $1;
            }
            
            my $value = "";
        
            if( $type =~ /^'/ )
            {
                $type =~ s/'([^']*)'?//;            # '...': Konstante abschneiden
                $value = $1;
            }
            elsif( $type =~ /^[(]/ )
            {
                $type =~ s/[(]([^)]+)[)]//;         # (...): Konstante abschneiden
                $value = $1;
            }


            if( $type_letter eq 'P'  ||
                $type_letter eq 'Z'     )
            {
                my $scale = $value =~ /\.([0-9]+)/? length( $1 ) : 0;
                
                $cobol_type = "PIC S9(" . ( ( $length * 2 - 1 ) - $scale ) . ")";
                $cobol_type .= "V9($scale)"  if $scale;
                $cobol_type .= " COMP-3"  if $type_letter eq 'P';
            }
            elsif( $type_letter eq 'A'  ||
                   $type_letter eq 'Y'     )
            {
                $cobol_type = "PIC 9(" . $length*2 . ") COMP";
            }
            elsif( $type_letter eq 'F'  ||
                   $type_letter eq 'H'     )
            {
                $cobol_type = "PIC S9(" . $length*2 . ") COMP";
            }
            elsif( $type_letter eq 'C'  ||
                   $type_letter eq 'X'     )
            {
                $cobol_type = "PIC X($length)";
            }
            
            my $w = $wdh == 1? "" : " OCCURS $wdh TIMES";

            $name = "FILLER"  if $name eq "";
            
            my $c = $wdh == 0? "*" : " ";
            
            my $name_width = 13;
            print "      $c  02 $name" . ( " " x ( $name_width - length($name) ) ) .
                  " $cobol_type$w" . ( $type eq ""? "" : " $type" ) . ".\n";   # $type sollte leer sein
            print "      *\n";
        }
        elsif( $line_nr != 2 )
        {
            print "--- FEHLER: Vorherige Zeile nicht erkannt\n";
            print STDERR "Zeile $line_nr nicht erkannt: $line\n";
        }
    }
}
