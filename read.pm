package read;

use strict;
use warnings;

use Exporter qw(import);
our @EXPORT_OK = qw(read_file);

sub read_file {
    my ($path) = @_;
    open my $fh, '<', $path or die "Could not open '$path': $!";
    my $content = do { local $/; <$fh> };
    close $fh;
    return $content;
}
