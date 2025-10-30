package cgen;

use strict;
use warnings;

use Exporter qw(import);
our @EXPORT_OK = qw(cgen);
use Data::Dumper qw(Dumper);

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
        $t .= $var;
        $t .= '[]';
        if( $type->{ptr} ) {
            $t .= '*';
        }

        return $t;
    }

    if( $type->{kind} eq 'Array<Integer>' ) {
        $t .= $types{$type->{value}};
        $t .= $var;
        $t .= '[';
        $t .= $type->{size};
        $t .= ']';
        if( $type->{ptr} ) {
            $t .= '*';
        }

        return $t;
    }
}

sub cgen {
    my $scope = 0;
    my $ctx = "#include <stdint.h>\n";
    my (@result) = @_;

    for( my $i = 0; $i < @result; $i++ ) {
        my $current = $result[$i];

        if( $current->{kind} eq 'Function' ) {
            $ctx .= "\t" x $scope;
            $ctx .= toType($current->{return_type}) . ' ' . $current->{name} . '(';

            my $j = 0;
            foreach my $argument (@{$current->{arguments}}) {
                my $name = ' ___arg_' . $j . '_';
                $ctx .= toType($argument, $name) . ', ';
                $j++;
            }

            $ctx =~ s/, $//;
            $ctx .= ") {\n";
            $scope++;
        } elsif( $current->{kind} eq 'CLOSE' ) {
            $scope--;
            $ctx .= "\t" x $scope;
            $ctx .= "}\n";
        }
    }

    $ctx .= "\n\n";

    print $ctx;
}
