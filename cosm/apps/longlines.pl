#!/usr/bin/perl -w

if ( !defined( @ARGV ) )
{
  print "\nFind any lines over 78 characters\n\n";
  print "Usage: longlines.pl <filenames>\n\n";
  exit();
}

foreach $file ( @ARGV )
{
  open( FILE, $file );
  @lines = <FILE>;
  close( FILE );

  $line_number = 1;
  foreach $line ( @lines )
  {
    # there is a linefeed, so it's 79 not 78
    if ( length( $line ) > 79 )
    {
      print "$file:$line_number:$line";
    }
    $line_number++;
  }             
}
