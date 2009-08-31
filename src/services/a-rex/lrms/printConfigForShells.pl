#!/usr/bin/perl -w

# Reads an XML config file containing config for A-REX and prints out
# configuration options for LRMS control scripts.  It's meant to be used by
# 'config_parser.sh'.
# If the gmconfig option is present, it will parse the linked ini file. Otherwise it will
# extract LRMS options from XML config.

use strict;
use warnings;

use XML::Simple;
use Data::Dumper qw(Dumper);

use ConfigParser;

my $lrmsNS = 'http://www.nordugrid.org/schemas/ArcConfig/2009/arex/LRMS';
my $infoNS = 'http://www.nordugrid.org/schemas/ArcConfig/2009/arex/InfoProvider';
my $arexNS = 'http://www.nordugrid.org/schemas/ArcConfig/2009/arex';
my $arcNS = 'http://www.nordugrid.org/schemas/ArcConfig/2007';

# options for XML::Simple with namespace expansion disabled
my $nameAttr = {Service => 'name', clusterShare => 'name', nodeGroup => 'name'};
my %xmlopts = (ForceArray => 1, KeepRoot => 1, NSexpand => 0, KeyAttr => $nameAttr);
# options for XML::Simple with namespace expansion enabled
my $nameAttrNS = {"{$arcNS}Service" => 'name', "{$infoNS}clusterShare" => 'name', "{$infoNS}nodeGroup" => 'name'};
my %xmloptsNS = (ForceArray => 1, KeepRoot => 1, NSexpand => 1, KeyAttr => $nameAttrNS);

# strip namespace prefix from the keys of the hash passed by reference
# with the excetion of keeping the lrms: prefix.
sub strip_namespaces_except_lrms {
    my ($h) = @_;
    my %t;
    while (my ($k,$v) = each %$h) {
        next if $k =~ m/^xmlns/;
        $k = "lrms:$k" if $k =~ s/^{$lrmsNS}//g;
        $k =~ s/^\w+:// unless $k =~ m/^lrms:/;
        $k =~ s/^{[^}]+}//;
        $t{$k} = $v;
    }
    %$h=%t;
    return;
}

# walk-a-tree of hashes and arrays
# apply a function to each hash
sub hash_fu {
    my ($ref, $func) = @_;
    if (not ref($ref)) {
        return;
    } elsif (ref($ref) eq 'ARRAY') {
        map {hash_fu($_,$func)} @$ref;
        return;
    } elsif (ref($ref) eq 'HASH') {
        &$func($ref);
        map {hash_fu($_,$func)} values %$ref;
        return;
    } else {
        return;
    }
}

# 
sub collapse_arrays {
    my ($h, @multivalued) = @_;
    while (my ($k,$v) = each %$h) {
        next unless ref($v) eq 'ARRAY';
        next if ref($v->[0]); # skip anything other than arrays of scalars
        next if grep {$k eq $_} @multivalued; # leave multivalued options as arrays
        $h->{$k} = pop @$v; # single valued options, remember last defined value only
    }
    return $h;
}

# 
sub expand_multivalued {
    my ($h, @multivalued) = @_;
    while (my ($k,$v) = each %$h) {
        next if ref $v; # skip anything other than arrays of scalars
        $h->{$k} = [split '\[separator\]', $v];
        unless (grep {$k eq $_} @multivalued) {
            $h->{$k} = pop @{$h->{$k}}; # single valued options, remember last defined value only
        }
    }
    return $h;
}

sub extract_lrms_opts {
    my ($h) = @_;
    my %t;
    while (my ($k,$v) = each %$h) {
        next unless $k =~ s/^lrms://;
        $t{$k} = $v;
    }
    return \%t;
}

sub get_arex_config {
    my ($file) = @_;
    my $xml = XML::Simple->new(%xmlopts);
    my $data = $xml->XMLin($file);
    hash_fu($data, \&strip_namespaces_except_lrms);
    #print(Dumper $data);
    if ($data->{ArcConfig}) {
        return $data->{ArcConfig}[0]{Chain}[0]{Service}{'a-rex'};
    } else {
        return $data->{Service}{'a-rex'};
    }
}

sub get_controldir {
    my ($arex) = @_;
    return undef unless $arex->{control};
    my @controls = @{$arex->{control}};
    #print(Dumper \@controls);
    for my $c (@controls) {
        my $username = $c->{username}[0];
        next unless not defined $username or $username eq ".";
        return $c->{controlDir}[0] || undef;
    
    return undef;
}

sub get_gmconfig_inifile {
    my ($arex) = @_;
    return undef unless $arex->{gmconfig}[0];
    return undef unless $arex->{gmconfig}[0]{type} eq 'INI';
    return $arex->{gmconfig}[0]{content};
}

sub build_config_from_inifile {
    my ($inifile) = @_;
    my $iniparser = ConfigParser->new($inifile);
    if (not $iniparser) {
        print STDERR "Cannot open $inifile\n";
        return undef;
    }
    my $config = {};
    for my $section (qw(common grid-manager cluster)) {
        $config->{$section} = {$iniparser->get_section($section)};
    }
    my @qnames = $iniparser->list_subsections('queue');
    for my $qname (@qnames) {
        $config->{shares}{$qname} = {$iniparser->get_section("queue/$qname")};
    } 
    my @opts = qw(condor_requirements opsys jobreport benchmark middleware);
    hash_fu $config, sub {expand_multivalued shift, @opts};
    #print(Dumper $config);
    return $config;
}

sub build_config_from_parsed_xml {
    my ($arex) = @_;
    my $controldir = get_controldir($arex);
    my $scratchdir = $arex->{scratchDir}[0] || undef;
    my $info = $arex->{infoProvider}[0];
    my $shares = $info->{clusterShare};
    #print(Dumper $shares);
    my $config = {};
    $config->{'common'} = extract_lrms_opts collapse_arrays $arex;
    $config->{'grid-manager'}{controldir} = $controldir if $controldir;
    $config->{'grid-manager'}{scratchdir} = $scratchdir if $scratchdir;
    map { $config->{shares}{$_} = extract_lrms_opts collapse_arrays $shares->{$_} } keys %$shares;
    return $config;
}

sub print_section_as_shellscript {
    my ($prefix,$bn,$opts) = @_;
    print $prefix."_NAME=\Q$bn\E\n";
    my $no=0;
    while (my ($opt,$val)=each %$opts) {
        unless ($opt =~ m/^\w+$/) {
            print "echo config_parser: Skipping malformed option \Q$opt\E 1>&2\n"; 
            next;
        }
        if (not ref $val) {
            $no++;
            $val = '' if not defined $val;
            print $prefix."_OPT${no}_NAME=$opt\n";
            print $prefix."_OPT${no}_VALUE=\Q$val\E\n";
        } elsif (ref $val eq 'ARRAY') {
            # multi-valued option
            for (my $i=0; $i<@$val; $i++) {
                $no++;
                $val->[$i] = '' if not defined $val->[$i];
                print $prefix."_OPT${no}_NAME=$opt"."_".($i+1)."\n";
                print $prefix."_OPT${no}_VALUE=\Q@{[$val->[$i]]}\E\n";
            }
        }
    }
    print $prefix."_NUM=$no\n";
}

sub print_config_as_shellscript {
    my ($lrms) = @_;
    my $nb = 0;
    for my $bn ('common', 'grid-manager', 'cluster') {
        my $opts = $lrms->{$bn};
        $nb++;
        my $prefix = "_CONFIG_BLOCK$nb";
        print_section_as_shellscript($prefix, $bn, $opts);
    }
    for my $bn (keys %{$lrms->{shares}}) {
        my $opts = $lrms->{shares}{$bn};
        $nb++;
        my $prefix = "_CONFIG_BLOCK$nb";
        print_section_as_shellscript($prefix, "queue/$bn", $opts);
    }
    print "_CONFIG_NUM_BLOCKS=$nb\n";
}

sub main {
    exit 1 unless $ARGV[0];
    my $xmlfile = $ARGV[0];
    my $arex_config = get_arex_config($xmlfile);
    my $lrms_config = build_config_from_parsed_xml($arex_config);
    my $inifile = get_gmconfig_inifile($arex_config);
    if ($inifile) {
        $lrms_config = build_config_from_inifile($inifile);
    }
    #print(Dumper $lrms_config);
    print_config_as_shellscript($lrms_config);
}

main();

