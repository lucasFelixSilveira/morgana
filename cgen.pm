package cgen;

use strict;
use warnings;

use Exporter qw(import);
our @EXPORT_OK = qw(cgen);
use Data::Dumper qw(Dumper);
use List::Util qw(any);

my %types = (
    'u8'  => 'uint8_t',  'i8'  => 'int8_t',
    'u16' => 'uint16_t', 'i16' => 'int16_t',
    'u32' => 'uint32_t', 'i32' => 'int32_t',
    'u64' => 'uint64_t', 'i64' => 'int64_t',
);

sub toType {
    my ($type, $var) = @_;
    my $t = '';

    if( $type->{kind} eq 'Integer' ) {
        $t .= $type->{sign} . $type->{bits};
        return $types{$t} || 'auto';
    }

    if( $type->{kind} eq 'Array<undef>' ) {
        $t .= $types{$type->{value}};
        if( $type->{ptr} ) { $t .= '*'; }

        $t .= $var;
        $t .= '[]';

        return $t;
    }

    if( $type->{kind} eq 'Array<Integer>' ) {
        $t .= $types{$type->{value}};
        if( $type->{ptr} ) { $t .= '*'; }

        $t .= $var;
        $t .= '[';
        $t .= $type->{size};
        $t .= ']';

        return $t;
    }
}

sub make {
    my ($pendence, $current) = @_;
    if( $pendence eq 'return' ) {
        return 'return ' . $current->{value} . ';';
    }
}

sub cgen {
    my $scope = 0;
    my $ctx = "#include <stdint.h>\n";
    my (@result) = @_;
    my $pendence;

    for( my $i = 0; $i < @result; $i++ ) {
        my $current = $result[$i];

        if( $current->{kind} eq 'Function' ) {
            $ctx .= "\t" x $scope;
            $ctx .= toType($current->{return_type}) . ' ' . $current->{name} . '(';

            my @identifiers;
            my $rename = 0;
            if( $result[$i + 1]->{kind} eq 'Rename' ) {
                $rename = 1;
                @identifiers = @{ $result[$i + 1]->{identifiers} };
                $i++;
            }

            my $j = 0;
            foreach my $argument (@{ $current->{arguments} }) {
                my $name = ($rename == 0) ? ' ___arg_' . $j . '_' : ' ' . $identifiers[$j];
                $ctx .= toType($argument, $name);
                if( not any { $_ eq $argument->{kind} } ('Array<undef>', 'Array<Integer>') ) {
                    $ctx .= $name;
                }

                $ctx .= ', ';
                $j++;
            }

            $ctx =~ s/, $//;
            $ctx .= ") {\n";
            $scope++;
        }

        elsif( $current->{kind} eq 'CLOSE' ) {
            $scope--;
            $ctx .= "\t" x $scope;
            $ctx .= "}\n";
        }

        elsif( $current->{kind} eq 'ret' ) {
            $pendence = 'return';
        }

        elsif( $current->{kind} eq 'LiteralInteger' ) {
            $ctx .= "\t" x $scope;
            $ctx .= make($pendence, $current) . "\n";
        }
    }

    $ctx .= "\n\n";

    print $ctx;
}
