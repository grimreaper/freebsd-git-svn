package warnings::register ;

require warnings ;

sub mkMask
{
    my ($bit) = @_ ;
    my $mask = "" ;

    vec($mask, $bit, 1) = 1 ;
    return $mask ;
}

sub import
{
    shift ;
    my $package = (caller(0))[0] ;
    if (! defined $warnings::Bits{$package}) {
        $warnings::Bits{$package}     = mkMask($warnings::LAST_BIT) ;
        vec($warnings::Bits{'all'}, $warnings::LAST_BIT, 1) = 1 ;
        $warnings::Offsets{$package}  = $warnings::LAST_BIT ++ ;
	foreach my $k (keys %warnings::Bits) {
	    vec($warnings::Bits{$k}, $warnings::LAST_BIT, 1) = 0 ;
	}
        $warnings::DeadBits{$package} = mkMask($warnings::LAST_BIT);
        vec($warnings::DeadBits{'all'}, $warnings::LAST_BIT++, 1) = 1 ;
    }
}

1 ;
