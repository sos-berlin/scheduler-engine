/* xception.inl                                         (c) SOS GmbH Berlin
*/

Exception::Exception()
 :  _level      ( 0 ),
    _base_level ( 0 )
{
    _name      [ 0 ] = 0;
    _error_code[ 0 ] = 0;
}

