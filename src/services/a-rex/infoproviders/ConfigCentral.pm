package ConfigCentral;

# Builds an intermediate config hash that is used by the A-REX infoprovider and LRMS control scripts
# Can read XML and INI

use strict;
use warnings;

use XML::Simple;
use Data::Dumper qw(Dumper);

use ConfigParser;
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
    AuthorizedVO => [ '*' ],
    MaxVirtualMemory => '*',
    MaxSlotsPerJob => '*',
    SchedulingPolicy => '*',
    Preemption => '*',
};
my $gmuser_options = {
    controldir => '',
    sessiondir => [ '' ],
    cachedir => [ '*' ],
    remotecachedir => [ '*' ],
    maxjobs => '*',
    maxload => '*',
    maxloadshare => [ '*' ],
    defaultttl => '*',
};
my $gmcommon_options = {
    lrms => '',
    gmconfig => '*',
    endpoint => '*',
    hostname => '*',
    gridmap => '*',
    x509_user_key => '*',
    x509_user_cert => '*',
    x509_cert_dir => '*',
    runtimedir => '*',
    gnu_time => '*',
    shared_filesystem => '*',
    shared_scratch => '*',
    scratchdir => '*',
};

# # # # # # # # # # # # # #

my $config_schema = {
    debugLevel => '*',
    PublishNordugrid => '*',
    AdminDomain => '*',
    ttl => '*',
    %$gmcommon_options,
    %$gmuser_options,
    %$lrms_options,
    %$lrms_share_options,
    control => {
        '*' => {
            %$gmuser_options
        }
    },
    service => {
        ClusterName => '*',
        OtherInfo => [ '*' ],
        StatusInfo => [ '*' ],
        Downtime => '*',
        ClusterOwner => [ '*' ],
        Middleware => [ '*' ],
        LocalSE => [ '*' ],
        InteractiveContactstring => '*',
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
        Type => '*',
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
            ExecEnvName => [ '' ],
            %$share_options,
            %$lrms_share_options,
        }
    }
};

my $allbools = [ qw(shared_filesystem shared_scratch
                 PublishNordugrid Homogeneous VirtualMachine
                 ConnectivityIn ConnectivityOut Preemption) ];

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

# Set selected keys to either 'true' or 'false'
sub fixbools {
    my ($h,$bools) = @_;
    for my $key (@$bools) {
        next unless exists $h->{$key};
        my $val = $h->{$key};
        if ($val eq '0' or lc $val eq 'false' or lc $val eq 'no') {
            $h->{$key} = 'false';
        } elsif ($val eq '0' or lc $val eq 'true' or lc $val eq 'yes') {
            $h->{$key} = 'true';
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
    my $nameAttr = {Service => 'name'};
    my %xmlopts = (NSexpand => 0, ForceArray => 1, KeepRoot => 1, KeyAttr => $nameAttr);
    my $xml = XML::Simple->new(%xmlopts);
    my $data;
    eval { $data = $xml->XMLin($file) };
    hash_tree_apply $data, \&hash_strip_prefixes;
    return $data->{Service}{'a-rex'}
        || $data->{ArcConfig}[0]{Chain}[0]{Service}{'a-rex'}
        || undef;
}


#
# Reads the XML config file passed as the first argument and produces a config
# hash conforming to $config_schema.
#
sub build_config_from_xmlfile {
    my ($file) = @_;

    my $arex = read_arex_config($file);

    my $config = {};
    $config->{control} = {};
    $config->{service} = {};
    $config->{location} = {};
    $config->{contacts} = [];
    $config->{xenvs} = {};
    $config->{shares} = {};

	# Special treatment for xml elements that have empty content. XMLSimple
	# converts these into an empty hash. Since ForceArray => 1 was used, these
	# hashes are placed inside an array. Replace these empty hashes with the
	# empty string. Do not touch keys that normally contain deep structures.
	my @deepstruct = qw(control dataTransfer Globus cache location loadLimits
						LRMS InfoProvider Location Contact ExecutionEnvironment
                        ComputingShare NodeSelection);
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
    my @multival = qw(cache location control sessionRootDir maxJobsPerShare
                      OpSys Middleware LocalSE ClusterOwner Benchmark OtherInfo
                      StatusInfo Regex Command Tag ExecEnvName AuthorizedVO
                      Contact ExecutionEnvironment ComputingShare);
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

    my $gmconfig = $arex->{gmconfig};
    if ($gmconfig) {
        if (not ref $gmconfig) {
            $config->{gmconfig} = $gmconfig;
        } elsif (exists $gmconfig->{type} and $gmconfig->{type} eq 'INI') {
            $config->{gmconfig} = $gmconfig->{content};
        }
    }

    $config->{control}{'.'} = {};

    $log->error("No control element in config") unless $arex->{control};
    for my $control (@{$arex->{control}}) {
        my $user = $control->{username} || '.';
        my $cconf = {};

        my $controldir = $control->{controlDir};
        $cconf->{controldir} = $controldir if $controldir;
        my $sessiondirs =  $control->{sessionRootDir}; # an array
        $cconf->{sessiondir} = $sessiondirs if $sessiondirs;

        my $ttl = $control->{defaultTTL} || '';
        my $ttr = $control->{defaultTTR} || '';
        $cconf->{defaultttl} = "$ttl $ttr" if $ttl;

        for my $cache (@{$control->{cache}}) {
            my $type = $cache->{type} || '';
            for (@{$cache->{location}}) {
                my $loc = $_->{path};
                next unless $loc;
                push @{$cconf->{cachedir}}, $loc if $type ne 'REMOTE';
                push @{$cconf->{remotecachedir}}, $loc if $type eq 'REMOTE';
            }
        }
        $config->{control}{$user} = $cconf;
    }
    # Merge control options for default user
    $config = { %$config, %{$config->{control}{'.'}} };
    delete $config->{control}{'.'};
    delete $config->{control} unless keys %{$config->{control}};

    my $globus = $arex->{dataTransfer}{Globus} || {};
    $log->warning("No Globus element in config") unless %$globus;
    rename_keys $globus, $config, {certpath => 'x509_user_cert', keypath => 'x509_user_key',
                                   cadir => 'x509_cert_dir', gridmapfile => 'gridmap'};
    my $load = $arex->{loadLimits};
    $log->warning("No loadLimits element in config") unless $load;
    my $maxjobs = $load->{maxJobsTracked} || '';
    my $maxjobsrun = $load->{maxJobsRun} || '';
    $config->{maxjobs} = "$maxjobs $maxjobsrun";

    for my $ts (@{$load->{maxJobsPerShare}}) {
        push @{$config->{maxloadshare}}, $ts->{content}." ".$ts->{shareType};
    }

    if (my $lrms = $arex->{LRMS}) {
        $config->{lrms} = $lrms->{type} if $lrms->{type};
        $config->{lrms} .= " ".$lrms->{defaultShare} if $lrms->{defaultShare};
        
        move_keys $lrms, $config, [keys %$lrms_options, keys %$lrms_share_options];
        rename_keys $lrms, $config, {runtimeDir => 'runtimedir', scratchDir => 'scratchdir',
                 sharedScratch => 'shared_scratch', sharedFilesystem => 'shared_filesystem',
                 GNUTimeUtility => 'gnu_time'};
    }

    my $ipcfg = $arex->{InfoProvider} || {};

    # OBS: just a temporary hack
    $ipcfg = $arex->{ComputingService} if $arex->{ComputingService};
    rename_keys $ipcfg,$ipcfg, {Name => 'ClusterName'};

    move_keys $ipcfg, $config->{service}, [keys %{$config_schema->{service}}];
    move_keys $ipcfg, $config, ['debugLevel', 'PublishNordugrid', 'AdminDomain'];
    rename_keys $ipcfg, $config, {Location => 'location', Contact => 'contacts'};

    my $xenvs = $ipcfg->{ExecutionEnvironment};
    for my $xe (@$xenvs) {
        my $name = $xe->{name};
        $log->error("ExecutionEnvironment witout name") unless $name;
        delete $xe->{name};
        my $xeconf = $config->{xenvs}{$name} = $xe;
        $xeconf->{NodeSelection} ||= {};
    }

    my $shares = $ipcfg->{ComputingShare};
    for my $s (@{$shares}) {
        my $name = $s->{name};
        $log->error("ComputingShare without name") unless $name;
        delete $s->{name};
        my $sconf = $config->{shares}{$name} = $s;
    }

    hash_tree_apply $config, sub { fixbools shift, $allbools };

    #print(Dumper $config);
    return $config;
}

#
# Reads the INI config file passed as the first argument and produces a config
# hash conforming to $config_schema. An already existing config hash can be
# passed as a second, optional argument in which case opptions read from the
# INI file will be merged into the hash overriding options already present.
#
sub build_config_from_inifile {
    my ($inifile, $config) = @_;

    my $iniparser = ConfigParser->new($inifile);
    if (not $iniparser) {
        $log->error("Cannot open $inifile\n");
        return $config;
    }
    # Will add to an already existing config.
    $config ||= {};
    $config->{service} ||= {};
    $config->{control} ||= {};
    $config->{location} ||= {};
    $config->{contacts} ||= [];
    $config->{xenvs} ||= {};
    $config->{shares} ||= {};

    my $common = { $iniparser->get_section("common") };
    my $gm = { $iniparser->get_section("grid-manager") };
    move_keys $common, $config, [keys %$gmcommon_options];
    move_keys $common, $config, [keys %$lrms_options, keys %$lrms_share_options];
    move_keys $gm, $config, [keys %$gmcommon_options, keys %$gmuser_options];

    $config->{debugLevel} = $common->{debug} if $common->{debug};

    my @cnames = $iniparser->list_subsections('grid-manager');
    for my $name (@cnames) {
        my $section = { $iniparser->get_section("grid-manager/$name") };
        move_keys $section, $config->{control}{$name}, [keys %$gmuser_options];
    }

    ################################ deprecated config file structure #############################

    rename_keys $common, $config, {arex_mount_point => 'endpoint'};
    move_keys $common, $config, ['AdminDomain'];
    move_keys $common, $config->{service}, [keys %{$config_schema->{service}}];

    my $cluster = { $iniparser->get_section('cluster') };
    if (%$cluster) {
        # Ignored: cluster_location, lrmsconfig
        rename_keys $cluster, $config->{service}, {
                                 interactive_contactstring => 'InteractiveContactstring',
                                 cluster_owner => 'ClusterOwner', localse => 'LocalSE',
                                 authorizedvo => 'AuthorizedVO', homogeneity => 'Homogeneous',
                                 architecture => 'Platform', opsys => 'OpSys', benchmark => 'Benchmark',
                                 nodememory => 'MaxVirtualMemory', middleware => 'Middleware'};
        push @{$config->{service}{OtherInfo}}, $cluster->{cluster_alias} if $cluster->{cluster_alias};
        push @{$config->{service}{OtherInfo}}, $cluster->{comment} if $cluster->{comment};
        if ($cluster->{clustersupport} and $cluster->{clustersupport} =~ /(.*)@/) {
            my $contact = {};
            push @{$config->{contacts}}, $contact;
            $contact->{Name} = $1;
            $contact->{Detail} = "mailto:".$cluster->{clustersupport};
            $contact->{Type} = 'usersupport';
        }
        for (split '\[separator\]', $cluster->{nodeaccess} || '') {
            $config->{service}{ConnectivityIn} = 'true' if $_ eq 'inbound';
            $config->{service}{ConnectivityOut} = 'true' if $_ eq 'outbound';
        }
        move_keys $cluster, $config->{service}, [keys %$share_options, keys %$xenv_options];
        move_keys $cluster, $config, [keys %$lrms_share_options];
    }
    my @qnames = $iniparser->list_subsections('queue');
    for my $name (@qnames) {
        my $queue = { $iniparser->get_section("queue/$name") };

        my $sconf = $config->{shares}{$name} ||= {};
        my $xeconf = $config->{xenvs}{$name} ||= {};
        push @{$sconf->{ExecEnvName}}, $name;

        $log->error("MappingQuue option only allowed under ComputingShare section") if $queue->{MappingQuue};
        delete $queue->{MappingQueue};
        $log->error("ExecEnvName option only allowed under ComputingShare section") if $queue->{ExecEnvName};
        delete $queue->{ExecEnvName};
        $log->error("NodeSelection option only allowed under ExecutionEnvironment section") if $queue->{NodeSelection};
        delete $queue->{NodeSelection};

        # Ignored: totalcpus
        rename_keys $queue, $sconf, {scheduling_policy => 'SchedulingPolicy',
                                     nodememory => 'MaxVirtualMemory', comment => 'Description'};
        move_keys $queue, $sconf, [keys %$share_options, keys %$lrms_share_options];

        rename_keys $queue, $xeconf, {homogeneity => 'Homogeneous', architecture => 'Platform',
                                      opsys => 'OpSys', benchmark => 'Benchmark'};
        move_keys $queue, $xeconf, [keys %$xenv_options];
    }

    ################################# new config file structure ##################################

    my $provider = { $iniparser->get_section("InfoProvider") };
    move_keys $provider, $config, ['debugLevel', 'PublishNordugrid', 'AdminDomain'];
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

    #print(Dumper $config);
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
    my $file = shift;
    my $config;
    if (isXML($file)) {
        $config = build_config_from_xmlfile($file);
        my $inifile = $config->{gmconfig};
        $config = build_config_from_inifile($inifile, $config) if $inifile;
    } else {
        $config = build_config_from_inifile($file);
    }

    LogUtils::level($config->{debugLevel}) if $config->{debugLevel};

    $log->error("No ExecutionEnvironment configured") unless %{$config->{xenvs}};
    $log->error("No ComputingShare configured") unless %{$config->{shares}};

    my $checker = InfoChecker->new($config_schema);
    my @messages = $checker->verify($config,1);
    $log->info("config key config->$_") foreach @messages;
    $log->info("Some required config options are missing") if @messages;

    #print(Dumper $config);
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
# shell script meant to be consumed by 'config_parser.sh'.
#
sub printLRMSConfigScript {
    my $file = shift;
    my $config = parseConfig($file);

    _print_shell_start();

    my $common = {};
    # controldir is a hack needed for finish-condor-job
    move_keys $config, $common, [keys %$lrms_options, keys %$lrms_share_options, 'controldir'];
    _print_shell_section('common', $common);
    
    for my $sname (keys %{$config->{shares}}) {
        my $queue = {};
        move_keys $config->{shares}{$sname}, $queue, [keys %$lrms_options, keys %$lrms_share_options];

        my $qname = $config->{shares}{$sname}{MappingQueue} || $sname;
        _print_shell_section("queue/$qname", $queue);
    }

    _print_shell_end();
}


1;

__END__
