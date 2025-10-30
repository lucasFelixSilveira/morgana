use lib '.';

use strict;
use Data::Dumper qw(Dumper);

my ($path, $isa) = @ARGV;
if( not defined $path ) { die "Usage: $0 <path>\n"; }

use read qw(read_file);
use tokenize qw(tokenize);
use parse qw(parse);
use cgen qw(cgen);

my $f_content = read_file($path);
my @tokens = tokenize($f_content);
my @parser = parse(@tokens);
cgen(@parser);
