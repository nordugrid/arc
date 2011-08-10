package ConfigCentral;

# Builds an intermediate config hash that is used by the A-REX infoprovider and LRMS control scripts
# Can read XML and INI

use strict;
use warnings;

use XML::Simple;
use Data::Dumper qw(Dumper);

use IniParser;
use InfoChecker;
use LogUtils;

# while parsing, loglevel is WARNING (the default)
our $log = LogUtils->getLogger(__PACKAGE__);

###############################################################
# Internal representation of configuration data after parsing #
###############################################################

my $lrms_options = {
    pbs_bin_path => '*',
    pbs_log_path => '*',
    dedicated_node_string => '*',
    maui_bin_path => '*',
    condor_location => '*',
    condor_config => '*',
    condor_rank => '*',
    sge_bin_path => '*',
    sge_root => '*',
    sge_cell => '*',
    sge_qmaster_port => '*',
    sge_execd_port => '*',
    lsf_bin_path => '*',
    lsf_profile_path => '*',
    ll_bin_path => '*',
    slurm_bin_path => '*',
    slurm_wakeupperiod => '*',
};
my $lrms_share_options = {
    fork_job_limit => '*',
    queue_node_string => '*',
    condor_requirements => '*',
    sge_jobopts => '*',
    lsf_architecture => '*',
    ll_consumable_resources => '*',
    slurm_project => '*',
};
my $xenv_options = {
    Platform => '*',
    Homogeneous => '*',
    PhysicalCPUs => '*',
    LogicalCPUs => '*',
    CPUVendor => '*',
    CPUModel => '*',
    CPUVersion => '*',
    CPUClockSpeed => '*',
    CPUTimeScalingFactor => '*',
    WallTimeScalingFactor => '*',
    MainMemorySize => '*',
    VirtualMemorySize => '*',
    OSFamily => '*',
    OSName => '*',
    OSVersion => '*',
    VirtualMachine => '*',
    NetworkInfo => '*',
    ConnectivityIn => '*',
    ConnectivityOut => '*',
    Benchmark => [ '*' ],
    OpSys => [ '*' ],
    nodecpu => '*',
};
my $share_options = {
    MaxVirtualMemory => '*',
    MaxSlotsPerJob => '*',
    SchedulingPolicy => '*',
    Preemption => '*',
};
my $gmuser_options = {
    controldir => '',
    sessiondir => [ '' ],
    cachedir => [ '*' ],
    cachesize => '*',
    remotecachedir => [ '*' ],
    defaultttl => '*',
};
my $gmcommon_options = {
    lrms => '',
    gmconfig => '*',
    endpoint => '*',
    hostname => '*',
    maxjobs => '*',
    maxload => '*',
    maxloadshare => '*',
    wakeupperiod => '*',
    gridmap => '*',
    x509_user_key => '*',
    x509_user_cert => '*',
    x509_cert_dir => '*',
    runtimedir => '*',
    gnu_time => '*',
    shared_filesystem => '*',
    shared_scratch => '*',
    scratchdir => '*',
    use_janitor => '*',
};
my $ldap_infosys_options = {
    bdii_var_dir => '*',
    bdii_tmp_dir => '*',
    infosys_compat => '*',
    infosys_nordugrid => '*',
    infosys_glue12 => '*',
    infosys_glue2_ldap => '*'
};
my $gridftpd_options = {
    GridftpdEnabled => '*',
    GridftpdPort => '*',
    GridftpdMountPoint => '*',
    GridftpdAllowNew => '*',
    remotegmdirs => [ '*' ],
};

# # # # # # # # # # # # # #

my $config_schema = {
    defaultLocalName => '*',
    debugLevel => '*',
    ProviderLog => '*',
    PublishNordugrid => '*',
    AdminDomain => '*',
    ttl => '*',
    %$gmcommon_options,
    %$gridftpd_options,
    %$ldap_infosys_options,
    %$lrms_options,
    %$lrms_share_options,
    control => {
        '*' => {
            %$gmuser_options
        }
    },
    service => {
        OtherInfo => [ '*' ],
        StatusInfo => [ '*' ],
        Downtime => '*',
        ClusterName => '*',
        ClusterAlias => '*',
        ClusterComment => '*',
        ClusterOwner => [ '*' ],
        Middleware => [ '*' ],
        AuthorizedVO => [ '*' ],
        LocalSE => [ '*' ],
        InteractiveContactstring => [ '*' ],
        %$xenv_options,
        %$share_options,
    },
    location => {
        Name => '*',
        Address => '*',
        Place => '*',
        Country => '*',
        PostCode => '*',
        Latitude => '*',
        Longitude => '*',
    },
    contacts => [ {
        Name => '*',
        OtherInfo => [ '*' ],
        Detail => '',
        Type => '',
    } ],
    accesspolicies => [ {
        Rule => [ '' ],
        UserDomainID => [ '' ],
    } ],
    mappingpolicies => [ {
        ShareName => [ '' ],
        Rule => [ '' ],
        UserDomainID => [ '' ],
    } ],
    xenvs => {
        '*' => {
            OtherInfo => [ '*' ],
            NodeSelection => {
                Regex => [ '*' ],
                Command => [ '*' ],
                Tag => [ '*' ],
            },
            %$xenv_options,
        }
    },
    shares => {
        '*' => {
            Description => '*',
            OtherInfo => [ '*' ],
            MappingQueue => '*',
            ExecutionEnvironmentName => [ '' ],
            %$share_options,
            %$lrms_share_options,
        }
    }
};

my $allbools = [ qw(
                 PublishNordugrid Homogeneous VirtualMachine
                 ConnectivityIn ConnectivityOut Preemption
                 infosys_compat infosys_nordugrid infosys_glue12 infosys_glue2_ldap
                 GridftpdEnabled GridftpdAllowNew use_janitor) ];

############################ Generic functions ###########################

# walks a tree of hashes and arrays while applying a function to each hash.
sub hash_tree_apply {
    my ($ref, $func) = @_;
    if (not ref($ref)) {
        return;
    } elsif (ref($ref) eq 'ARRAY') {
        map {hash_tree_apply($_,$func)} @$ref;
        return;
    } elsif (ref($ref) eq 'HASH') {
        &$func($ref);
        map {hash_tree_apply($_,$func)} values %$ref;
        return;
    } else {
        return;
    }
}

# Strips namespace prefixes from the keys of the hash passed by reference
sub hash_strip_prefixes {
    my ($h) = @_;
    my %t;
    while (my ($k,$v) = each %$h) {
        next if $k =~ m/^xmlns/;
        $k =~ s/^\w+://;
        $t{$k} = $v;
    }
    %$h=%t;
    return;
}

# Verifies that a key is an HASH reference and returns that reference
sub hash_get_hashref {
    my ($h, $key) = @_;
    my $r = ($h->{$key} ||= {});
    $log->fatal("badly formed '$key' element in XML config") unless ref $r eq 'HASH';
    return $r;
}

# Verifies that a key is an ARRAY reference and returns that reference
sub hash_get_arrayref {
    my ($h, $key) = @_;
    my $r = ($h->{$key} ||= []);
    $log->fatal("badly formed '$key' element in XML config") unless ref $r eq 'ARRAY';
    return $r;
}

# Set selected keys to either 'true' or 'false'
sub fixbools {
    my ($h,$bools) = @_;
    for my $key (@$bools) {
        next unless exists $h->{$key};
        my $val = $h->{$key};
        if ($val eq '0' or lc $val eq 'false' or lc $val eq 'no' or lc $val eq 'disable') {
            $h->{$key} = '0';
        } elsif ($val eq '1' or lc $val eq 'true' or lc $val eq 'yes' or lc $val eq 'enable') {
            $h->{$key} = '1';
        } else {
            $log->error("Invalid value for $key");
        }
    }
    return $h;
}

sub move_keys {
    my ($h, $k, $names) = @_;
    for my $key (@$names) {
        next unless exists $h->{$key};
        $k->{$key} = $h->{$key};
        delete $h->{$key};
    }
}

sub rename_keys {
    my ($h, $k, $names) = @_;
    for my $key (keys %$names) {
        next unless exists $h->{$key};
        my $newkey = $names->{$key};
        $k->{$newkey} = $h->{$key};
        delete $h->{$key};
    }
}

############################ ARC xml config ##############################

sub read_arex_config {
    my ($file) = @_;
    my %xmlopts = (NSexpand => 0, ForceArray => 1, KeepRoot => 1, KeyAttr => {});
    my $xml = XML::Simple->new(%xmlopts);
    my $data;
    eval { $data = $xml->XMLin($file) };
    $log->error("Failed to parse XML file $file: $@") if $@;
    hash_tree_apply $data, \&hash_strip_prefixes;
    my $services;
    $services = $data->{Service}
        if  ref $data eq 'HASH'
        and ref $data->{Service} eq 'ARRAY';
    $services = $data->{ArcConfig}[0]{Chain}[0]{Service}
        if  not $services
        and ref $data eq 'HASH'
        and ref $data->{ArcConfig} eq 'ARRAY'
        and ref $data->{ArcConfig}[0] eq 'HASH'
        and ref $data->{ArcConfig}[0]{Chain} eq 'ARRAY'
        and ref $data->{ArcConfig}[0]{Chain}[0] eq 'HASH'
        and ref $data->{ArcConfig}[0]{Chain}[0]{Service} eq 'ARRAY';
    return undef unless $services;
    for my $srv (@$services) {
        next unless ref $srv eq 'HASH';
        return $srv if $srv->{name} eq 'a-rex';
    }
    return undef;
}


#
# Reads the XML config file passed as the first argument and produces a config
# hash conforming to $config_schema.
#
sub build_config_from_xmlfile {
    my ($file, $arc_location) = @_;

    my $arex = read_arex_config($file);
    $log->fatal("A-REX config not found in $file") unless ref $arex eq 'HASH';

    # The structure that will hold all config options
    my $config = {};
    $config->{control} = {};
    $config->{service} = {};
    $config->{location} = {};
    $config->{contacts} = [];
    $config->{accesspolicies} = [];
    $config->{mappingpolicies} = [];
    $config->{xenvs} = {};
    $config->{shares} = {};

    # Special treatment for xml elements that have empty content. XMLSimple
    # converts these into an empty hash. Since ForceArray => 1 was used, these
    # hashes are placed inside an array. Replace these empty hashes with the
    # empty string. Do not touch keys that normally contain deep structures.
    my @deepstruct = qw(control dataTransfer Globus cache location remotelocation loadLimits
                        LRMS InfoProvider Location Contact ExecutionEnvironment
                        ComputingShare NodeSelection AccessPolicy MappingPolicy);
    hash_tree_apply $arex, sub { my $h = shift;
                                 while (my ($k,$v) = each %$h) {
                                     next unless ref($v) eq 'ARRAY';
                                     next if grep {$k eq $_} @deepstruct;
                                     my $nv = [ map {(ref $_ eq 'HASH' && ! scalar %$_) ? '' : $_ } @$v ];
                                     $h->{$k} = $nv;
                                 }
                           };

    # Collapse unnnecessary arrays created by XMLSimple.  All hash keys are
    # array valued now due to using ForceArray => 1.  For keys that corresound
    # to options which are not multivalued, the arrays should contain only one
    # element. Replace these arrays with the value of the last element. Keys
    # corresponding to multivalued options are left untouched.
    my @multival = qw(cache location remotelocation control sessionRootDir remotegmdirs
                      OpSys Middleware LocalSE ClusterOwner Benchmark OtherInfo
                      StatusInfo Regex Command Tag ExecutionEnvironmentName AuthorizedVO
                      Contact ExecutionEnvironment ComputingShare
                      InteractiveContactstring AccessPolicy MappingPolicy ShareName Rule UserDomainID);
    hash_tree_apply $arex, sub { my $h = shift;
                                 while (my ($k,$v) = each %$h) {
                                     next unless ref($v) eq 'ARRAY';
                                     next if grep {$k eq $_} @multival;
                                     $v = pop @$v;
                                     $h->{$k} = $v;
                                 }
                           };

    move_keys $arex, $config, ['endpoint', 'debugLevel'];
    $config->{ttl} = 2 * $arex->{InfoproviderWakeupPeriod} if $arex->{InfoproviderWakeupPeriod};

    my $usermap = hash_get_hashref($arex, 'usermap');
    my $username = $usermap->{'defaultLocalName'};
    $config->{defaultLocalName} = $username if $username;

    my $gmconfig = $arex->{gmconfig};
    if ($gmconfig) {
        if (not ref $gmconfig) {
            $config->{gmconfig} = $gmconfig;
        } elsif (ref $gmconfig eq 'HASH') {
            $config->{gmconfig} = $gmconfig->{content}
                if $gmconfig->{content} and $gmconfig->{type}
                                        and $gmconfig->{type} eq 'INI';
        }
    }

    my $controls = hash_get_arrayref($arex, 'control');
    for my $control (@$controls) {
        $log->fatal("badly formed 'control' element in XML config") unless ref $control eq 'HASH';
        my $user = $control->{username} || '.';
        my $cconf = {};

        my $controldir = $control->{controlDir};
        $cconf->{controldir} = $controldir if $controldir;
        my $sessiondirs =  $control->{sessionRootDir}; # an array
        $cconf->{sessiondir} = $sessiondirs if $sessiondirs;

        my $ttl = $control->{defaultTTL} || '';
        my $ttr = $control->{defaultTTR} || '';
        $cconf->{defaultttl} = "$ttl $ttr" if $ttl;

        my $caches = hash_get_arrayref($control, 'cache');
        for my $cache (@$caches) {
            $log->fatal("badly formed 'cache' element in XML config") unless ref $cache eq 'HASH';
            my $locations = hash_get_arrayref($cache, 'location');
            for my $location (@$locations) {
                $log->fatal("badly formed 'location' element in XML config") unless ref $location eq 'HASH';
                next unless $location->{path};
                push @{$cconf->{cachedir}}, $location->{path};
            }
            my $rlocations = hash_get_arrayref($cache, 'remotelocation');
            for my $location (@$rlocations) {
                $log->fatal("badly formed 'location' element in XML config") unless ref $location eq 'HASH';
                next unless $location->{path};
                push @{$cconf->{remotecachedir}}, $location->{path};
            }
            my $low = $cache->{lowWatermark} || '';
            my $high = $cache->{highWatermark} || '';
            $cconf->{cachesize} = "$low $high" if $low;
        }
        $config->{control}{$user} = $cconf;
    }

    my $globus = hash_get_hashref(hash_get_hashref($arex, 'dataTransfer'), 'Globus');

    # these are obsolete now -- kept only for backwards compatibility
    rename_keys $globus, $config, {certpath => 'x509_user_cert', keypath => 'x509_user_key',
                                   cadir => 'x509_cert_dir'};

    rename_keys $globus, $config, {CertificatePath => 'x509_user_cert', KeyPath => 'x509_user_key',
                                   CACertificatesDir => 'x509_cert_dir', gridmapfile => 'gridmap'};

    my $load = hash_get_hashref($arex, 'loadLimits');
    my $mj = $load->{maxJobsTracked} || '-1';
    my $mjr = $load->{maxJobsRun} || '-1';
    # maxJobsTransfered, maxJobsTransferedAddtional and maxFilesTransfered are parsed for backward compatibility.
    my $mjt = $load->{maxJobsTransferred} || $load->{maxJobsTransfered} || '-1';
    my $mjta = $load->{maxJobsTransferredAdditional} || $load->{maxJobsTransferedAdditional} || '-1';
    my $mft = $load->{maxFilesTransferred} || $load->{maxFilesTransfered} || '-1';
    $config->{maxjobs} = "$mj $mjr";
    $config->{maxload} = "$mjt $mjta $mft";

    if ($load->{maxLoadShare} and $load->{loadShareType}) {
        $config->{maxloadshare} = $load->{maxLoadShare}." ".$load->{loadShareType};
    }

    $config->{wakeupperiod} = $load->{wakeupPeriod} if defined $load->{wakeupPeriod};

    my $lrms = hash_get_hashref($arex, 'LRMS');
    $config->{lrms} = $lrms->{type} if $lrms->{type};
    $config->{lrms} .= " ".$lrms->{defaultShare} if $lrms->{defaultShare};

    move_keys $lrms, $config, [keys %$lrms_options, keys %$lrms_share_options];
    rename_keys $lrms, $config, {runtimeDir => 'runtimedir',
                                 scratchDir => 'scratchdir',
                                 sharedScratch => 'shared_scratch',
                                 sharedFilesystem => 'shared_filesystem',
                                 GNUTimeUtility => 'gnu_time',
                                 useJanitor => 'use_janitor'};

    my $ipcfg = hash_get_hashref($arex, 'InfoProvider');

    rename_keys $ipcfg, $ipcfg, {Name => 'ClusterName'};

    move_keys $ipcfg, $config->{service}, [keys %{$config_schema->{service}}];
    move_keys $ipcfg, $config, ['debugLevel', 'ProviderLog', 'PublishNordugrid', 'AdminDomain'];
    move_keys $ipcfg, $config, [keys %$gridftpd_options];
    rename_keys $ipcfg, $config, {Location => 'location', Contact => 'contacts'};

    rename_keys $ipcfg, $config, {AccessPolicy => 'accesspolicies', MappingPolicy => 'mappingpolicies'};

    my $xenvs = hash_get_arrayref($ipcfg, 'ExecutionEnvironment');
    for my $xe (@$xenvs) {
        $log->fatal("badly formed 'ExecutionEnvironment' element in XML config") unless ref $xe eq 'HASH';
        my $name = $xe->{name};
        $log->fatal("ExecutionEnvironment without name attribute") unless $name;
        my $xeconf = $config->{xenvs}{$name} = {};
        $xeconf->{NodeSelection} = hash_get_hashref($xe, 'NodeSelection');
        move_keys $xe, $xeconf, [keys %{$config_schema->{xenvs}{'*'}}];
    }

    my $shares = hash_get_arrayref($ipcfg, 'ComputingShare');
    for my $s (@{$shares}) {
        $log->fatal("badly formed 'ComputingShare' element in XML config") unless ref $s eq 'HASH';
        my $name = $s->{name};
        $log->error("ComputingShare without name attribute") unless $name;
        my $sconf = $config->{shares}{$name} = {};
        move_keys $s, $sconf, [keys %{$config_schema->{shares}{'*'}}];
    }

    hash_tree_apply $config, sub { fixbools shift, $allbools };

    _substitute($config, $arc_location);

    #print(Dumper $config);
    return $config;
}

sub _substitute {
    my ($config, $arc_location) = @_;
    my $control = $config->{control};

    my ($lrms, $defqueue) = split " ", $config->{lrms} || '';

    die 'Gridmap user list feature is not supported anymore. Please use @filename to specify user list.'
        if exists $control->{'*'};

    # expand user sections whose user name is like @filename
    my @users = keys %$control;
    for my $user (@users) {
        next unless $user =~ m/^\@(.*)$/;
        my $path = $1;
        my $fh;
        # read in user names from file
        if (open ($fh, "< $path")) {
            while (my $line = <$fh>) {
                chomp (my $newuser = $line);
                next if exists $control->{$newuser};         # Duplicate user!!!!
                $control->{$newuser} = { %{$control->{$user}} }; # shallow copy
            }
            close $fh;
            delete $control->{$user};
        } else {
            die "Failed opening file to read user list from: $path: $!";
        }
    }

    # substitute per-user options
    @users = keys %$control;
    for my $user (@users) {
        my @pw;
        my $home;
        if ($user ne '.') {
            @pw = getpwnam($user);
            $log->warning("getpwnam failed for user: $user: $!") unless @pw;
            $home = $pw[7] if @pw;
        } else {
            $home = "/tmp";
        }

        my $opts = $control->{$user};

        # Default for controldir, sessiondir
        if ($opts->{controldir} eq '*') {
            $opts->{controldir} = $pw[7]."/.jobstatus" if @pw;
        }
        $opts->{sessiondir} ||= ['*'];
        $opts->{sessiondir} = [ map { $_ ne '*' ? $_ : "$home/.jobs" } @{$opts->{sessiondir}} ];

        my $controldir = $opts->{controldir};
        my @sessiondirs = @{$opts->{sessiondir}};

        my $subst_opt = sub {
            my ($val) = @_;

            #  %R - session root
            $val =~ s/%R/$sessiondirs[0]/g if $val =~ m/%R/;
            #  %C - control dir
            $val =~ s/%C/$controldir/g if $val =~ m/%C/;
            if (@pw) {
                #  %U - username
                $val =~ s/%U/$user/g       if $val =~ m/%U/;
                #  %u - userid
                #  %g - groupid
                #  %H - home dir
                $val =~ s/%u/$pw[2]/g      if $val =~ m/%u/;
                $val =~ s/%g/$pw[3]/g      if $val =~ m/%g/;
                $val =~ s/%H/$home/g       if $val =~ m/%H/;
            }
            #  %L - default lrms
            #  %Q - default queue
            $val =~ s/%L/$lrms/g           if $val =~ m/%L/;
            $val =~ s/%Q/$defqueue/g       if $val =~ m/%Q/;
            #  %W - installation path
            $val =~ s/%W/$arc_location/g   if $val =~ m/%W/;
            #  %G - globus path
            my $G = $ENV{GLOBUS_LOCATION} || '/usr';
            $val =~ s/%G/$G/g              if $val =~ m/%G/;

            return $val;
        };
        if ($opts->{controldir}) {
            $opts->{controldir} = &$subst_opt($opts->{controldir});
        }
        if ($opts->{sessiondir}) {
            $opts->{sessiondir} = [ map {&$subst_opt($_)} @{$opts->{sessiondir}} ];
        }
        if ($opts->{cachedir}) {
            $opts->{cachedir} = [ map {&$subst_opt($_)} @{$opts->{cachedir}} ];
        }
        if ($opts->{remotecachedir}) {
            $opts->{remotecachedir} = [ map {&$subst_opt($_)} @{$opts->{remotecachedir}} ];
        }
    }

    # authplugin, localcred, helper: not substituted

    return $config;
}


#
# Reads the INI config file passed as the first argument and produces a config
# hash conforming to $config_schema. An already existing config hash can be
# passed as a second, optional, argument in which case opptions read from the
# INI file will be merged into the hash overriding options already present.
#
sub build_config_from_inifile {
    my ($inifile, $config) = @_;

    my $iniparser = SubstitutingIniParser->new($inifile);
    if (not $iniparser) {
        $log->error("Failed parsing config file: $inifile\n");
        return $config;
    }
    $log->error("Not a valid INI configuration file: $inifile") unless $iniparser->list_sections();

    # Will add to an already existing config.
    $config ||= {};
    $config->{service} ||= {};
    $config->{control} ||= {};
    $config->{location} ||= {};
    $config->{contacts} ||= [];
    $config->{accesspolicies} ||= [];
    $config->{mappingpolicies} ||= [];
    $config->{xenvs} ||= {};
    $config->{shares} ||= {};


    my $common = { $iniparser->get_section("common") };
    my $gm = { $iniparser->get_section("grid-manager") };
    rename_keys $common, $config, {providerlog => 'ProviderLog'};
    move_keys $common, $config, [keys %$gmcommon_options];
    move_keys $common, $config, [keys %$lrms_options, keys %$lrms_share_options];
    move_keys $gm, $config, [keys %$gmcommon_options];
    rename_keys $gm, $config, {arex_mount_point => 'endpoint'};

    $config->{debugLevel} = $common->{debug} if $common->{debug};

    move_keys $common, $config, [keys %$ldap_infosys_options];

    my $infosys = { $iniparser->get_section("infosys") };
    rename_keys $infosys, $config, {providerlog => 'ProviderLog',
                                    provider_loglevel => 'debugLevel'};
    move_keys $infosys, $config, [keys %$ldap_infosys_options];

    my @cnames = $iniparser->list_subsections('grid-manager');
    for my $name (@cnames) {
        my $section = { $iniparser->get_section("grid-manager/$name") };
        $config->{control}{$name} ||= {};
        move_keys $section, $config->{control}{$name}, [keys %$gmuser_options];
    }

    # Cherry-pick some gridftp options
    if ($iniparser->has_section('gridftpd/jobs')) {
        my %gconf = $iniparser->get_section('gridftpd');
        my %gjconf = $iniparser->get_section('gridftpd/jobs');
        $config->{GridftpdEnabled} = 'yes';
        $config->{GridftpdPort} = $gconf{port} if $gconf{port};
        $config->{GridftpdMountPoint} = $gjconf{path} if $gjconf{path};
        $config->{GridftpdAllowNew} = $gjconf{allownew} if defined $gjconf{allownew};
        $config->{remotegmdirs} = $gjconf{remotegmdirs} if defined $gjconf{remotegmdirs};
    } else {
        $config->{GridftpdEnabled} = 'no';
    }

    ############################ legacy ini config file structure #############################

    move_keys $common, $config, ['AdminDomain'];

    my $cluster = { $iniparser->get_section('cluster') };
    if (%$cluster) {
        # Ignored: cluster_location, lrmsconfig
        rename_keys $cluster, $config, {arex_mount_point => 'endpoint'};
        rename_keys $cluster, $config->{location}, { cluster_location => 'PostCode' };
        rename_keys $cluster, $config->{service}, {
                                 interactive_contactstring => 'InteractiveContactstring',
                                 cluster_owner => 'ClusterOwner', localse => 'LocalSE',
                                 authorizedvo => 'AuthorizedVO', homogeneity => 'Homogeneous',
                                 architecture => 'Platform', opsys => 'OpSys', benchmark => 'Benchmark',
                                 nodememory => 'MaxVirtualMemory', middleware => 'Middleware',
                                 cluster_alias => 'ClusterAlias', comment => 'ClusterComment'};
        if ($cluster->{clustersupport} and $cluster->{clustersupport} =~ /(.*)@/) {
            my $contact = {};
            push @{$config->{contacts}}, $contact;
            $contact->{Name} = $1;
            $contact->{Detail} = "mailto:".$cluster->{clustersupport};
            $contact->{Type} = 'usersupport';
        }
        if (defined $cluster->{nodeaccess}) {
            $config->{service}{ConnectivityIn} = 0;
            $config->{service}{ConnectivityOut} = 0;
            for (split '\[separator\]', $cluster->{nodeaccess}) {
                $config->{service}{ConnectivityIn} = 1 if lc $_ eq 'inbound';
                $config->{service}{ConnectivityOut} = 1 if lc $_ eq 'outbound';
            }
        }
        move_keys $cluster, $config->{service}, [keys %$share_options, keys %$xenv_options];
        move_keys $cluster, $config, [keys %$lrms_options, keys %$lrms_share_options];
    }
    my @qnames = $iniparser->list_subsections('queue');
    for my $name (@qnames) {
        my $queue = { $iniparser->get_section("queue/$name") };

        my $sconf = $config->{shares}{$name} ||= {};
        my $xeconf = $config->{xenvs}{$name} ||= {};
        push @{$sconf->{ExecutionEnvironmentName}}, $name;

        $log->error("MappingQuue option only allowed under ComputingShare section") if $queue->{MappingQuue};
        delete $queue->{MappingQueue};
        $log->error("ExecutionEnvironmentName option only allowed under ComputingShare section") if $queue->{ExecutionEnvironmentName};
        delete $queue->{ExecutionEnvironmentName};
        $log->error("NodeSelection option only allowed under ExecutionEnvironment section") if $queue->{NodeSelection};
        delete $queue->{NodeSelection};

        # Ignored: totalcpus
        rename_keys $queue, $sconf, {scheduling_policy => 'SchedulingPolicy',
                                     nodememory => 'MaxVirtualMemory', comment => 'Description'};
        move_keys $queue, $sconf, [keys %$share_options, keys %$lrms_share_options];

        rename_keys $queue, $xeconf, {homogeneity => 'Homogeneous', architecture => 'Platform',
                                      opsys => 'OpSys', benchmark => 'Benchmark'};
        move_keys $queue, $xeconf, [keys %$xenv_options];
        $xeconf->{NodeSelection} = {};
    }

    ################################# new ini config file structure ##############################

    my $provider = { $iniparser->get_section("InfoProvider") };
    move_keys $provider, $config, ['debugLevel', 'ProviderLog', 'PublishNordugrid', 'AdminDomain'];
    move_keys $provider, $config->{service}, [keys %{$config_schema->{service}}];

    my @gnames = $iniparser->list_subsections('ExecutionEnvironment');
    for my $name (@gnames) {
        my $xeconf = $config->{xenvs}{$name} ||= {};
        my $section = { $iniparser->get_section("ExecutionEnvironment/$name") };
        $xeconf->{NodeSelection} ||= {};
        $xeconf->{NodeSelection}{Regex} = $section->{NodeSelectionRegex} if $section->{NodeSelectionRegex};
        $xeconf->{NodeSelection}{Command} = $section->{NodeSelectionCommand} if $section->{NodeSelectionCommand};
        $xeconf->{NodeSelection}{Tag} = $section->{NodeSelectionTag} if $section->{NodeSelectionTag};
        move_keys $section, $xeconf, [keys %$xenv_options, 'OtherInfo'];
    }
    my @snames = $iniparser->list_subsections('ComputingShare');
    for my $name (@snames) {
        my $sconf = $config->{shares}{$name} ||= {};
        my $section = { $iniparser->get_section("ComputingShare/$name") };
        move_keys $section, $sconf, [keys %{$config_schema->{shares}{'*'}}];
    }
    my $location = { $iniparser->get_section("Location") };
    $config->{location} = $location if %$location;
    my @ctnames = $iniparser->list_subsections('Contact');
    for my $name (@ctnames) {
        my $section = { $iniparser->get_section("Contact/$name") };
        push @{$config->{contacts}}, $section;
    }

    # Create a list with all multi-valued options based on $config_schema.
    my @multival = ();
    hash_tree_apply $config_schema, sub { my $h = shift;
                                           for (keys %$h) {
                                               next if ref $h->{$_} ne 'ARRAY';
                                               next if ref $h->{$_}[0]; # exclude deep structures
                                               push @multival, $_;
                                           }
                                     };
    # Transform multi-valued options into arrays
    hash_tree_apply $config, sub { my $h = shift;
                                   while (my ($k,$v) = each %$h) {
                                       next if ref $v; # skip anything other than scalars
                                       $h->{$k} = [split '\[separator\]', $v];
                                       unless (grep {$k eq $_} @multival) {
                                           $h->{$k} = pop @{$h->{$k}}; # single valued options, remember last defined value only
                                       }
                                   }
                             };

    hash_tree_apply $config, sub { fixbools shift, $allbools };

    #print (Dumper $config);
    return $config;
}

#
# Check whether a file is XML
#
sub isXML {
    my $file = shift;
    $log->fatal("Can't open $file") unless open (CONFIGFILE, "<$file");
    my $isxml = 0;
    while (my $line = <CONFIGFILE>) {
        chomp $line;
        next unless $line;
        if ($line =~ m/^\s*<\?xml/) {$isxml = 1; last};
        if ($line =~ m/^\s*<!--/)   {$isxml = 1; last};
        last;
    }
    close CONFIGFILE;
    return $isxml;
}

#
# Grand config parser for A-REX. It can parse INI and XML config files. When
# parsing an XML config, it checks for the gmconfig option and parses the
# linked INI file too, merging the 2 configs. Options defined in the INI
# file override the ones from the XML file.
#
sub parseConfig {
    my ($file, $arc_location) = @_;
    my $config;
    if (isXML($file)) {
        $config = build_config_from_xmlfile($file, $arc_location);
        my $inifile = $config->{gmconfig};
        $config = build_config_from_inifile($inifile, $config) if $inifile;
    } else {
        $config = build_config_from_inifile($file);
    }

    LogUtils::level($config->{debugLevel}) if $config->{debugLevel};

    my $checker = InfoChecker->new($config_schema);
    my @messages = $checker->verify($config,1);
    $log->info("config key config->$_") foreach @messages;
    $log->info("Some required config options are missing") if @messages;

    return $config;
}

################### support for shell scripts ############################

{
    my $nb;

    sub _print_shell_start { my $nb = 0 }
    sub _print_shell_end { print "_CONFIG_NUM_BLOCKS=$nb\n" }

    sub _print_shell_section {
        my ($bn,$opts) = @_;
        $nb++;
        my $prefix = "_CONFIG_BLOCK$nb";

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
}

#
# Reads A-REX config and prints out configuration options for LRMS control
# scripts. Only the LRMS-related options are handled. The output is executable
# shell script meant to be sourced by 'config_parser.sh'.
#
sub printLRMSConfigScript {
    my $file = shift;
    my $config = parseConfig($file);

    _print_shell_start();

    my $common = {};
    move_keys $config, $common, [keys %$lrms_options, keys %$lrms_share_options];

    _print_shell_section('common', $common);

    my $gmopts = {};
    $gmopts->{runtimedir} = $config->{runtimedir} if $config->{runtimedir};
    $gmopts->{gnu_time} = $config->{gnu_time} if $config->{gnu_time};
    $gmopts->{scratchdir} = $config->{scratchdir} if $config->{scratchdir};
    $gmopts->{shared_scratch} = $config->{shared_scratch} if $config->{shared_scratch};
    # shared_filesystem: if not set, assume 'yes'
    $gmopts->{shared_filesystem} = $config->{shared_filesystem} if $config->{shared_filesystem};

    _print_shell_section('grid-manager', $gmopts);

    my $cluster = {};
    rename_keys $config->{service}, $cluster, {MaxVirtualMemory => 'nodememory'};

    _print_shell_section('cluster', $cluster) if %$cluster;

    for my $sname (keys %{$config->{shares}}) {
        my $queue = {};
        move_keys $config->{shares}{$sname}, $queue, [keys %$lrms_options, keys %$lrms_share_options];
        rename_keys $config->{shares}{$sname}, $queue, {MaxVirtualMemory => 'nodememory'};

        my $qname = $config->{shares}{$sname}{MappingQueue};
        $queue->{MappingQueue} = $qname if $qname;

        _print_shell_section("queue/$sname", $queue);
    }

    _print_shell_end();
}


1;

__END__
