package parse;

use strict;
use Data::Dumper qw(Dumper);

use Exporter qw(import);
our @EXPORT_OK = qw(parse types);
use List::Util qw(any);

my %types = (
    'u8'  => 'uint8_t',  'i8'  => 'int8_t',
    'u16' => 'uint16_t', 'i16' => 'int16_t',
    'u32' => 'uint32_t', 'i32' => 'int32_t',
    'u64' => 'uint64_t', 'i64' => 'int64_t',
);

sub checkType {
    my ($token) = @_;
    if( my ($type, $value, $aptr) = $token =~ m{\[([*]|[0-9]+):([^\]]*)\](\*)?}x ) {
        if( $type eq '*') {
            return { kind => 'Array<undef>', value => $value, ptr => $aptr };
        } else {
            return { kind => 'Array<Integer>', value => $value, ptr => $aptr, size => $type };
        }
    } elsif( my ($sign, $bits, $iptr) = $token =~ m{(i|u)([0-9]+)(\*)?}x ) {
        return { kind => 'Integer', bits => $bits, ptr => $iptr, sign => $sign };
    }
    return undef;
}

sub isType {
    my ($token) = @_;
    if( $types{$token} ) {
        return 1;
    } else {
        return checkType($token);
    }
}

sub is_kind_type {
    my ($type) = @_;
    return any { $_ eq $type } ('Integer', 'Array<undef>', 'Array<Integer>');
}

sub isKeyword {
    my ($type) = @_;
    return any { $_ eq $type } ('ret');
}

sub parse {
    my (@tokens) = @_;
    my @results;
    my @kindfull;

    my $i = 0;
    for ($i = 0; $i < @tokens; $i++) {
        my $token = $tokens[$i];
        my $first = substr($token, 0, 1);

        if ($first eq '(') {
            my $ctx = $token;
            while (substr($token, -1) ne ')') {
                $i++;
                die "Erro de sintaxe: parêntese de fechamento ')' esperado" if $i >= @tokens;
                $token = $tokens[$i];
                $ctx .= $token;
            }
            push @kindfull, { kind => 'Desconstructor', value => $ctx };
        }
        elsif (isKeyword($token)) {
            push @kindfull, { kind => $token };
        } elsif (isType($token)) {
            push @kindfull, checkType($token);
        } elsif ($token =~ m{[a-zA-Z][a-zA-Z0-9_]+}x) {
            push @kindfull, { kind => 'Identifier', value => $token };
        } elsif ($token =~ m{[0-9]+}x) {
            push @kindfull, { kind => 'LiteralInteger', value => $token };
        } elsif ($token eq '{') {
            push @kindfull, { kind => 'OPEN' };
        } elsif ($token eq '}') {
            push @kindfull, { kind => 'CLOSE' };
        } elsif ($token eq '@_') {
            push @kindfull, { kind => 'THAT' };
        } else {
            push @kindfull, { kind => 'Unknown', value => $token };
        }
    }


    for( my $i = 0; $i < @kindfull; $i++ ) {
        my $current = $kindfull[$i];

        if( $current->{kind} eq 'Desconstructor' and $kindfull[$i + 1]->{kind} eq 'THAT' ) {
            my $ctx = substr $kindfull[$i]->{value}, 1, -1 ;
            my @identifiers = split /[,]\s*/, $ctx;
            @identifiers = grep /\S/, @identifiers;

            push @results, {
                kind => 'Rename',
                identifiers => \@identifiers
            };
        }

        elsif( is_kind_type($current->{kind}) and $kindfull[$i + 1]->{kind} eq 'Identifier' ) {
            my $func_name = $kindfull[$i + 1]->{value};
            my @arguments;

            $i += 2;
            while( $i < @kindfull && $kindfull[$i]->{kind} ne 'OPEN') {
                push @arguments, $kindfull[$i];
                $i++;
            }

            push @results, {
                return_type => $current,
                kind => 'Function',
                name => $func_name,
                arguments => \@arguments
            };
        }

        elsif( $current->{kind} eq 'CLOSE' ) {
            push @results, {
                kind => 'CLOSE'
            };
        } else {
            push @results, $current
        }
    }

    return @results;
}

1;
