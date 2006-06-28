# $Id$

sub expect
{
    my $expr = shift;
    my $real_value = shift;
    my $expected_value = shift;
    
    #my $real_value = eval( $expr );
    if( $real_value ne $expected_value )  { $spooler_log->error( "$expr == '$real_value', erwartet wird aber '$expected_value'" ); }
}
