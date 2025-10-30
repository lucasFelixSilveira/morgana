package tokenize;

use strict;
use warnings;

use Exporter qw(import);
our @EXPORT_OK = qw(tokenize);

sub tokenize {
    my ($content) = @_;
    my @tokens = split /\s+/, $content;
    @tokens = grep { /\S/ } @tokens;
    return @tokens;
}
