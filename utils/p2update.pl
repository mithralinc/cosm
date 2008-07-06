#!/usr/bin/perl -w

if ( !defined( @ARGV ) )
{
  print "\nUpdates user source to Cosm Phase 2.\n";
  print "Does all automateable updates.\n\n";
  print "Usage: p2update.pl <filenames>\n\n";
  exit();
}

sub replace
{
  my( $text, $n, $search, $replace ) = @_;

  $pre = $text;
  $line =~ s/$search/eval $replace/eg;
  if ( ( $n > 0 ) && ( $line ne $pre ) )
  {
    print "Rule $n:\n";
    print $pre;
    print $line, "\n";
  }
}

foreach $file ( @ARGV )
{
  open( FILE, $file );
  @lines = <FILE>;
  close( FILE );

  open( FILE, ">$file" );
  foreach $line ( @lines )
  {
    $line =~ s/v3_time/cosmtime/g;
    $line =~ s/unicode/utf8/g;

    $line =~ s/v3Input\(/v3InputRaw\(/g;
    $line =~ s/v3InputA\(/v3Input\(/g;
    $line =~ s/v3InputU\(/v3Input\(/g;
    $line =~ s/v3StrCopyUA\(/v3StrCopyUA-DEPRECIATED\(/g;
    $line =~ s/v3StrLengthA\(/v3StrBytes\(/g;
    $line =~ s/v3StrLengthU\(/v3StrBytes\(/g;
    $line =~ s/v3StrCopyA\(/v3StrCopy\(/g;
    $line =~ s/v3StrCopyU\(/v3StrCopy\(/g;
    $line =~ s/v3StrAppendA\(/v3StrAppend\(/g;
    $line =~ s/v3StrAppendU\(/v3StrAppend\(/g;
    $line =~ s/v3StrCmpA\(/v3StrCmp\(/g;
    $line =~ s/v3StrCmpU\(/v3StrCmp\(/g;
    $line =~ s/v3StrCharA\(/v3StrChar\(/g;
    $line =~ s/v3StrCharU\(/v3StrChar\(/g;
    $line =~ s/v3StrStrA\(/v3StrStr\(/g;
    $line =~ s/v3StrStrU\(/v3StrStr\(/g;
    $line =~ s/v3u32A\(/v3u32Str\(/g;
    $line =~ s/v3u32U\(/v3u32Str\(/g;
    $line =~ s/v3s32A\(/v3s32Str\(/g;
    $line =~ s/v3s32U\(/v3s32Str\(/g;
    $line =~ s/v3u64A\(/v3u64Str\(/g;
    $line =~ s/v3u64U\(/v3u64Str\(/g;
    $line =~ s/v3s64A\(/v3s64Str\(/g;
    $line =~ s/v3s64U\(/v3s64Str\(/g;
    $line =~ s/v3u128A\(/v3u128Str\(/g;
    $line =~ s/v3u128U\(/v3u128Str\(/g;
    $line =~ s/v3s128A\(/v3s128Str\(/g;
    $line =~ s/v3s128U\(/v3s128Str\(/g;
    $line =~ s/v3f32A\(/v3f32Str\(/g;
    $line =~ s/v3f32U\(/v3f32Str\(/g;
    $line =~ s/v3f64A\(/v3f64Str\(/g;
    $line =~ s/v3f64U\(/v3f64Str\(/g;
    $line =~ s/v3PrintAFile\(/v3PrintFile\(/g;
    $line =~ s/v3PrintuFile\(/v3PrintFile\(/g;
    $line =~ s/v3PrintAStr\(/v3PrintStr\(/g;
    $line =~ s/v3PrintUStr\(/v3PrintStr\(/g;
    $line =~ s/v3PrintA\(/v3Print\(/g;
    $line =~ s/v3PrintU\(/v3Print\(/g;

    $line =~ s/v3Compress\(/v3Encoder\(/g;
    $line =~ s/V3_COMPRESS_BASE64\(/V3_ENCODER_BASE64\(/g;

    $line =~ s/v3NetACLTest\(/v3NetACLCheck\(/g;

    $line =~ s/v3f64NaN\(/v3FloatNaN\(/g;
    $line =~ s/v3f64Inf\(/v3FloatInf\(/g;

    $line =~ s/v3BNA\(/v3BNStr\(/g;

    $line =~ s/v3ProcessSignal\(/v3Signal\(/g;

    $line =~ s/cosmfile\./os_file\./g;
    $line =~ s/cosmio\./os_io\./g;
    $line =~ s/cosmmath\./os_math\./g;
    $line =~ s/cosmmem\./os_mem\./g;
    $line =~ s/cosmnet\./os_net\./g;
    $line =~ s/cosmtask\./os_task\./g;

    $line =~ s/v3_ENCODER_TMP/v3_ENCODER/g;
    $line =~ s/V3_DIR_MODE_READ/0/g;

    $line =~ s/\,\ V3_MEM_NORMAL//g;

    # take the parens off simple (well formatted) return statements
    $line =~ s/^(.*)return\(\ (.*)\ \);(.*)$/$1return\ $2;$3/;

    # take out most of those (ascii *) in front of strings
    $line =~ s/\(ascii\ \*\)\ \"/\"/g;
    $line =~ s/\ \(ascii\ \*\)$//g;

    # _V3_SET64( size, 00000000, 00000100 )
    $line =~ s/_V3_SET64\(\ (.*?),\ (.*?),\ (.*?)\ \);/$1 = 0x$2$3LL;/g;
    $line =~ s/_V3_EQ64\(\ (.*?),\ (.*?),\ (.*?)\ \)/\(\ $1 == 0x$2$3LL\ \)/g;
    # _V3_SET128( size, 00000000, 00000100, 00000000, 00000000 )
    $line =~ s/_V3_SET128\(\ (.*?),\ (.*?),\ (.*?),\ (.*?),\ (.*?)\ \);/_V3_SET128\(\ $1,\ $2$3,\ $4$5\ \);/g;
    $line =~ s/_V3_EQ128\(\ (.*?),\ (.*?),\ (.*?),\ (.*?),\ (.*?)\ \)/_V3_EQ128\(\ $1,\ $2$3,\ $4$5\ \)/g;

    # v3u32u64( x )
    $line =~ s/v3u32u64\(\ ([^\(]*?)\ \)/(u32) $1/g;
    $line =~ s/v3s32s64\(\ ([^\(]*?)\ \)/(s32) $1/g;
    $line =~ s/v3u64u32\(\ ([^\(]*?)\ \)/(u64) $1/g;
    $line =~ s/v3s64s32\(\ ([^\(]*?)\ \)/(s64) $1/g;
    # v3u64s64( a ) ( (u64) a )
    $line =~ s/v3u64s64\(\ ([^\(]*?)\ \)/(u64) $1/g;
    $line =~ s/v3s64u64\(\ ([^\(]*?)\ \)/(s64) $1/g;

    # 1 parameter math
    $line =~ s/v3u64Inc\(\ &([^\(]*?)\ \)/$1++/g;
    $line =~ s/v3u64Inc\(\ ([^\(]*?)\ \)/\(\*$1\)++/g;
    $line =~ s/v3u64Dec\(\ &([^\(]*?)\ \)/$1--/g;
    $line =~ s/v3u64Dec\(\ ([^\(]*?)\ \)/\(\*$1\)--/g;
    $line =~ s/v3s64Inc\(\ &([^\(]*?)\ \)/$1++/g;
    $line =~ s/v3s64Inc\(\ ([^\(]*?)\ \)/\(\*$1\)++/g;
    $line =~ s/v3s64Dec\(\ &([^\(]*?)\ \)/$1--/g;
    $line =~ s/v3s64Dec\(\ ([^\(]*?)\ \)/\(\*$1\)--/g;
    $line =~ s/v3u64Not\(\ ([^\(]*?)\ \)/\( ~$1 \)/g;

    # 2 parameter logic
    $line =~ s/v3u64And\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 & $2 \)/g;
    $line =~ s/v3u64Or\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 | $2 \)/g;
    $line =~ s/v3u64Xor\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 ^ $2 \)/g;
    $line =~ s/v3u64Lsh\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 << $2 \)/g;
    $line =~ s/v3u64Rsh\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 >> $2 \)/g;

    $line =~ s/v3u64Gt\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 > $2 \)/g;
    $line =~ s/v3u64Lt\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 < $2 \)/g;
    $line =~ s/\!v3u64Eq\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 != $2 \)/g;
    $line =~ s/v3u64Eq\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 == $2 \)/g;
    $line =~ s/v3s64Gt\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 > $2 \)/g;
    $line =~ s/v3s64Lt\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 < $2 \)/g;
    $line =~ s/\!v3s64Eq\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 != $2 \)/g;
    $line =~ s/v3s64Eq\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 == $2 \)/g;

    # 2 parameter math
    $line =~ s/v3u64Add\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 + $2 \)/g;
    $line =~ s/v3u64Sub\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 - $2 \)/g;
    $line =~ s/v3u64Mul\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 * $2 \)/g;
    $line =~ s/v3u64Div\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 \/ $2 \)/g;
    $line =~ s/v3u64Mod\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 % $2 \)/g;
    $line =~ s/v3s64Add\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 + $2 \)/g;
    $line =~ s/v3s64Sub\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 - $2 \)/g;
    $line =~ s/v3s64Mul\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 * $2 \)/g;
    $line =~ s/v3s64Div\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 \/ $2 \)/g;
    $line =~ s/v3s64Mod\(\ ([^\(]*?),\ ([^\(]*?)\ \)/\( $1 % $2 \)/g;

    #fix constants
    $line =~ s/\(u64\)\ ([1-9]\d+)\ /$1LL /g;
    $line =~ s/\(u64\)\ ([1-9]\d+);/$1LL;/g;
    $line =~ s/\(u64\)\ 0\ /0\ /g;
    $line =~ s/\(s64\)\ ([1-9]\d+)\ /$1LL /g;
    $line =~ s/\(s64\)\ ([1-9]\d+);/$1LL;/g;
    $line =~ s/\(s64\)\ 0\ /0\ /g;

    #fix !=
    $line =~ s/\!\(\ ([^\(]*?)\ ==\ ([^\(]*?)\ \)/$1 != $2/g;

    # Migrate v3 to Cosm

    # macros
    &replace( $line, 0, '_V3_', '"_COSM_"' );

    # defines
    &replace( $line, 0, 'V3_', '"COSM_"' );

    # globals
    &replace( $line, 0, '__v3', '"__cosm"' );

    # public functions
    &replace( $line, 0, 'v3([a-zA-Z])', '"Cosm$1"' );
    # private functions
    &replace( $line, 0, 'v3_([A-Z][a-z])', '"Cosm_$1"' );

    # structs - has to not have lowercase anywhere
    &replace( $line, 0, 'v3_([_A-Z0-9]+[^_A-Za-z0-9])', '"cosm_$1"' );

    # private functions with CAPS in them
    &replace( $line, 0, 'v3_([_A-Za-z0-9])', '"Cosm_$1"' );

    # last straglers
    &replace( $line, 0, 'V3', '"COSM"' );
    &replace( $line, 0, 'v3', '"Cosm"' );

    # the (type)Add/Sub/Load/Save functions are lowercase, lets fix
    &replace( $line, 0, 'Cosmu', '"CosmU"' );
    &replace( $line, 0, 'Cosms', '"CosmS"' );
    &replace( $line, 0, '128u', '"128U"' );
    &replace( $line, 0, '128s', '"128S"' );
    &replace( $line, 0, 'U32u', '"U32U"' );
    &replace( $line, 0, 'S32s', '"S32S"' );
    &replace( $line, 0, 'U64u', '"U64U"' );
    &replace( $line, 0, 'S64s', '"S64S"' );

    # encoder -> transform
    &replace( $line, 0, 'CosmEncoder', '"CosmTransform"' );
    &replace( $line, 0, 'cosm_ENCODER', '"cosm_TRANSFORM"' );
    &replace( $line, 0, 'COSM_ENCODER_', '"COSM_TRANSFORM_"' );

    # Odd function name, we use Init
    &replace( $line, 0, 'CosmBufferCreate', '"CosmBufferInit"' );

    # hash functiosn are transforms now
    &replace( $line, 0, 'cosm_HASH_TMP', '"cosm_TRANSFORM"' );

    # cosmtime is now time
    &replace( $line, 0, 'cosmtime\.', '"time\."' );

    # speed functions are gone
    &replace( $line, 0, 'CosmSpeedInt', '"CosmSpeedInt-DEPRECIATED"' );
    &replace( $line, 0, 'CosmSpeedFloat', '"CosmSpeedFloat-DEPRECIATED"' );

    # NONPORTABLE_CODE now ALLOW_UNSAFE_C
    &replace( $line, 0, 'NONPORTABLE_CODE', '"ALLOW_UNSAFE_C"' );

    # CosmMemWarning is removed, not much we can do when not supported
    &replace( $line, 0, 'CosmMemWarning', '"CosmMemWarning-DEPRECIATED"' );

    # parameter 2 and 3 switched
    &replace( $line, 0, 'CosmNetOpen', '"CosmNetOpen-API-CHANGE-SEE-DOCS"' );

    print FILE $line;
  }
  close FILE;
}

