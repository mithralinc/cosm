#!/usr/bin/perl -w
#
# html2man -- automatically convert an HTML file containing documentation
#             for Cosm function(s) into the corresponding man page(s).

use strict;

sub main;
sub read_func;
sub process_func;
sub output_func;
sub html_convert;
sub gen_symlinks;

# Global Variables:

# Copyright notice to prepend to all man pages.

my $COPYRIGHT
       = ".\\\" Copyright 1995-2012 Mithral Communications & Design Inc.\n"
       . ".\\\" Mithral(R) and Cosm(R) are registered trademarks of\n"
       . ".\\\" Mithral Communications & Design Inc.\n";

# Suffix of output files

my $FILENAME_SUFFIX = '.3';

# 1. Open HTML file for reading.
# 2. Skip header (first <hr>).
# 3. Read function (until next <hr>).
# 4. Process function (convert HTML-> man).
# 5. Write output.
# 6. Loop to 3, until footer found.

exit( main() );

sub main
{
  my( $html ) = @ARGV;
  my( $func, $out, $filename, $header ) = ();
  my( @links );
  local *HTML;

  if ( !defined( $html ) )
  {
    print "Usage: html2man <filename>\n";
    return;
  }

  # Open HTML file for reading.
  open( HTML, "<$html" ) || die "  Unable to open file: $!\n";

  read_func( undef, 1 );                        # Skip header

  while( read_func( \$func, 0 ) == 1 )          # Read function.
  {
    $out = $filename = $header = '';
    @links = ();

    process_func( \$out, \$filename, \@links, \$header, $func )
     && next;                                   # Process function.
    output_func( $out, $filename, $header );    # Write output.
    gen_symlinks( $filename, @links );
  }
  close( HTML );
  return 0;
}

sub read_func
{
  my ( $pfunc, $skip ) = @_;
  my ( $found, $read ) = ( undef , 0, undef );
  my ( $foot_re ) = ( '&copy; Copyright' );

  if ( !( $skip || defined($pfunc) ) )
  {
    # Yay. an undefined reference (aka NULL pointer)
    die "  BUG: Undefined reference";
  }

  ## Read up to the next <hr>, then store it, or just ignore it if $skip.
  $$pfunc = '';

  while ( defined( $read = <HTML> ) )
  {
    if ( ! ( $read =~ /<hr>/i ) )
    {
      $found = 1;
      if ( !$skip )
      {
        $$pfunc .= $read;
      }
    }
    else
    {
      last;
    }
  }

  if ( !$skip && $$pfunc =~ /$foot_re/i )
  {
    $found = 2;
  }

  return $found;
}

sub process_func
{
  my( $pout, $pfn, $plinks, $pheader, $func ) = @_;
  my( $fname, @links );
  my( $syntax );

  if ( !( defined( $pout ) && defined( $pfn ) && defined( $plinks ) ) )
  {
    # Yay. an undefined reference (aka NULL pointer)
    die "  BUG: Undefined reference";
  }

  ## We have the HTML documentation of the function in $func.
  ## process_func works out the title and file name(s), etc, then
  ## hands off $func to html_convert.

  # First output the copyright.
  $$pout = $COPYRIGHT;


  if ( !defined( $func ) )
  {
    print "  ERROR: process_func was not given a function..\n";
    return 1;
  }

  # Header of manpage is the <h3>CosmFuncName</h3> line.
  ($$pheader) = ( $func =~ /<h3>\n?\s*(.+?)\n?\s*<\/h3>/i );

  if ( !defined( $$pheader ) )
  {
    print "  ERROR: Unable to determine manpage title\n";
    return 1;
  }

  # Now, the slightly harder task of extracting the file name(s).

  # Use the syntax/synopsis, each function prototyped is a filename.
  ($syntax) = ( $func =~ /<pre>(.+?)<\/pre>/is );

  # Extract function names.
  @$plinks = ();

  foreach $fname ( split( "\n", $syntax ) )
  {
    if ( $fname =~ /Cosm.+\(/i )
    {
      push( @$plinks, ($fname =~ /^\w+.+?(Cosm.+?)\s*\(/i) );
    }
  }

  # Append suffix to function names to make file name.
  foreach $fname ( @$plinks )
  {
    $fname .= $FILENAME_SUFFIX;
  }

  # First filename isnt actually a symlink, but the actual filename.
  $$pfn = shift( @$plinks );

  if ( !defined( $$pfn ) )
  {
    print "  ERROR: Unable to determine manpage filename\n";
    return 1;
  }

  # Finaly, convert the HTML documentation to man page format.
  return html_convert( $pout, $func, $$pheader );
}

sub output_func
{
  my( $out, $file, $header ) = @_;

  open( OUT, ">$file" ) ||
    do {
      print "  ERROR: Unable to open output file $file: $!\n";
      return 1;
    };

  print OUT $out;

  printf( "  %-38.38s %-38.38s\n", $header, $file );

  close( OUT );

  return 0;
}

sub gen_symlinks
{
  my( $file, @links ) = @_;
  my( $link, $error) = ('', 0);

  ## generate links
  foreach $link (@links)
  {
    if ( -e $link )
    {
      unlink( $link ) || print "  ERROR: Unable to delete existing file "
                             . "expect error on next line.\n";
    }
    symlink( $file, $link ) ||
      do {
        print "  ERROR: Unable to symlink $file to $link: $!\n";
        $error = 1;
      };
    printf( "  %-38.38s %-38.38s\n", "", $link );
  }

  return $error;
}

sub html_convert
{
  my( $pout, $func, $fname ) = @_;
  my( @see_also, $see_func, $pre, $mtime );
  my( $date, $day, $month, $year );
  my( @months ) = ( 'January', 'February', 'March', 'April',
                    'May', 'June', 'July', 'August', 'September',
                    'October', 'November', 'December' );

  if ( !defined( $pout ) )
  {
    # Yay. an undefined reference (aka NULL pointer)
    die "  BUG: Undefined reference";
  }

  ## Generate header.
  # Timestamp:
  (undef, undef,  undef, undef, undef, undef, undef,
   undef, undef, $mtime, undef, undef, undef ) =
     stat( HTML );

  (undef, undef, undef, $day, $month, $year, undef, undef, undef) =
    gmtime( $mtime );

  $date = "$day $months[$month] " . ( $year + 1900 );

  $$pout .= ".TH $fname 7 " . '"' . $date . '"' . "\n";

  ## Strip unused tags.
  $func =~ s#\n\s*?</p>\s*?##ig;
  $func =~ s#</?font.*>##ig;

  ## Generate all subheadings.

  while ( $func =~ m#<h4>(.+?)</h4>#ig )
  {
    $pre = uc( $1 );
    $func =~ s#<h4>(.+?)</h4>#.SH $pre#i;
  }

  ## Generate the 'SEE ALSO' list from hyperlinks.

  while ( ( $func =~ /<a href=".+">(.+)<\/a>/gi ) ) {
    $see_func = $1;
    if ( $see_func =~ /^Cosm/i )
    {
      $see_func = "\fB" . $see_func . "\fP" . '(7)';
      $see_func =~ s/\s*\n\s*//g;
      $see_func =~ s/\.//g;
      push( @see_also, $see_func );
   }
  }

  ## Tidy up .'s
  $func =~ s#>\.#>\\&.#igs;
  $func =~ s#\n\.#\n\\&.#igs;

  $func =~ s#\\n#\\\\n#isg;

  while ( $func =~ /<pre>\s*\n(.+?)<\/pre>/igs )
  {
    $pre = $1;
    $pre =~ s/^/!!/igm;
    $pre =~ s#\\\\n;#\\n#gis;
    $func =~ s/<pre>.*?<\/pre>/\n.nf\n$pre\n.sp\n/is;
  }

  ## Strip leading whitespace

  $func =~ s/^\s+//gm;
  $func =~ s/^!!//gm;

  ## Convert <p> -> .PP  New paragraph
  $func =~ s#<p>#\n.PP\n#ig;

  ## Convert <li> -> .TP \n .B  Indented bullet
  $func =~ s#<li>#.TP\n.B \\(bu\n#ig;

  $func =~ s#</ul>#.P\n#ig;

  ## Convert <em> <i> -> \fI  Italics
  $func =~ s#<em>#\\fI#ig;
  $func =~ s#<i>#\\fI#ig;

  ## Convert <strong> <b> -> \fB  Bold
  $func =~ s#<strong>#\\fB#ig;
  $func =~ s#<b>#\\fB#ig;

  ## Convert </em>, </strong>
  ##         </i>, </b> -> \fP  Plain
  $func =~ s#</em>#\\fP#ig;
  $func =~ s#</i>#\\fP#ig;
  $func =~ s#</b>#\\fP#ig;
  $func =~ s#</strong>#\\fP#ig;

  ## Guh.  Evil. man needs 2 tags surrounding the para, html uses one.
  $func =~ s#\n?\s*<dd>(.+?)(<\/?d.+?>)#\n.RS\n${1}\n.RE\n${2}#gsi;

  $func =~ s#<dt>#\n.TP\n.B \\(bu\n#ig;

  ## Strip remaining HTML gunk, and (un)escape icky codes.
  $func =~ s#<.+?>##g;
  $func =~ s#&lt;#<#g;
  $func =~ s#&gt;#>#g;
  $func =~ s#&amp;#&#g;

  $$pout .= $func;

  ## Append SEE ALSO list
  if ( defined( $see_also[0] ) )
  {
    $$pout .= ".SH SEE ALSO\n";
    $$pout .= join( ",\n", @see_also ) . "\n" ;
  }

  ## Strip multiple blank lines

  $$pout =~ s/\s*\n\s*\n\s*\n/\n/g;

  return 0;
}
