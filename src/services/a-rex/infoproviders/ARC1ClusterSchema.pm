package ARC1ClusterSchema;

use base Exporter;
our @EXPORT_OK = qw(arc1_info_schema);

sub arc1_info_schema {

    my $location_t = {
            'LocalID'   => [ '' ],
            'Name'      => [ '' ],
            'Address'   => [ '*' ],
            'Place'     => [ '*' ],
            'Country'   => [ '*' ],
            'PostCode'  => [ '*' ],
            'Latitude'  => [ '*' ],
            'Longitude' => [ '*' ],
    };

    my $benchmark_t = {
            'LocalID' => [ '' ],
            'Type'    => [ '' ],
            'Value'   => [ '' ],
    };

    my $contact_t = {
            'LocalID'   => [ '' ],
            'Type'      => [ '' ],
            'URL'       => [ '' ],
            'OtherInfo' => [ '*' ],
    };

    my $access_pol_t = {
            'LocalID' => [ '' ],
            'Scheme'  => [ '' ],
            'Rule'    => [ '' ],
    };

    my $mapping_pol_t = {
            'LocalID' => [ '' ],
            'Scheme'  => [ '' ],
            'Rule'    => [ '' ],
    };

    my $comp_endp_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                 => [ '' ],
            'Name'               => [ '*' ],
            'URL'                => [ '' ],
            'Technology'         => [ '*' ],
            'Interface'          => [ '' ],
            'InterfaceExtension' => [ '*' ],
            'WSDL'               => [ '*' ],
            'SupportedProfile'   => [ '*' ],
            'Semantics'          => [ '*' ],
            'Implementor'        => [ '*' ],
            'ImplementationName' => [ '*' ],
            'ImplementationVersion' => [ '*' ],
            'QualityLevel'       => [ '' ],
            'HealthState'        => [ '' ],
            'HealthStateInfo'    => [ '*' ],
            'ServingState'       => [ '' ],
            'StartTime'          => [ '*' ],
            'IssuerCA'           => [ '*' ],
            'TrustedCA'          => [ '*' ],
            'DowntimeAnnounce'   => [ '*' ],
            'DowntimeStart'      => [ '*' ],
            'DowntimeEnd'        => [ '*' ],
            'DowntimeInfo'       => [ '*' ],
            'Staging'            => [ '*' ],
            'JobDescription'     => [ '*' ],
            'AccessPolicy'       => [ $access_pol_t ],
             'Associations' => {
                'ComputingShareLocalID' =>  [ '' ],
                'ComputingActivityID'   =>  [ '' ],
            }
    };

    my $comp_share_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'LocalID'               => [ '' ],
            'Name'                  => [ '*' ],
            'Description'           => [ '*' ],
            'MappingQueue'          => [ '' ],
            'MaxWallTime'           => [ '*' ], # units: seconds
            'MinWallTime'           => [ '*' ], # units: seconds
            'DefaultWallTime'       => [ '*' ], # units: seconds
            'MaxCPUTime'            => [ '*' ], # units: seconds
            'MinCPUTime'            => [ '*' ], # units: seconds
            'DefaultCPUTime'        => [ '*' ], # units: seconds
            'MaxTotalJobs'          => [ '*' ],
            'MaxRunningJobs'        => [ '*' ],
            'MaxWaitingJobs'        => [ '*' ],
            'MaxPreLRMSWaitingJobs' => [ '*' ],
            'MaxUserRunningJobs'    => [ '*' ],
            'MaxSlotsPerJob'        => [ '' ],
            'MaxStageInStreams'     => [ '*' ],
            'MaxStageOutStreams'    => [ '*' ],
            'SchedulingPolicy'      => [ '*' ],
            'MaxMemory'             => [ '' ],  # units: MB
            'MaxDiskSpace'          => [ '*' ], # units: GB
            'DefaultStorageService' => [ '*' ],
            'Preemption'            => [ '*' ],
            'ServingState'          => [ '' ],
            'TotalJobs'             => [ '' ],
            'RunningJobs'           => [ '' ],
            'LocalRunningJobs'      => [ '' ],
            'WaitingJobs'           => [ '' ],
            'LocalWaitingJobs'      => [ '' ],
            'StagingJobs'           => [ '' ],
            'SuspendedJobs'         => [ '' ],
            'PreLRMSWaitingJobs'    => [ '' ],
            'EstimatedAverageWaitingTime' => [ '*' ],
            'EstimatedWorstWaitingTime'   => [ '*' ],
            'FreeSlots'             => [ '' ],
            'FreeSlotsWithDuration' => [ '*' ], # units: slots:seconds
            'UsedSlots'             => [ '*' ],
            'RequestedSlots'        => [ '' ],
            'ReservationPolicy'     => [ '*' ],
            'Tag'                   => [ '*' ],
            #'MappingPolicy' => [ $mapping_pol_t ],
            #'Associations' => {
            #    'ComputingEndpointID'         => [ '' ],
            #    'ExecutionEnvironmentLocalID' => [ '' ],
            #    'ComputingActivityID'         => [ '' ],
            #},
    };

    my $comp_manager_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                    => [ '' ],
            'Name'                  => [ '' ],
            'Type'                  => [ '' ],
            'Version'               => [ '' ],
            'Reservation'           => [ '*' ],
            'BulkSubmission'        => [ '' ],
            'TotalPhysicalCPUs'     => [ '' ],
            'TotalLogicalCPUs'      => [ '' ],
            'TotalSlots'            => [ '' ],
            'SlotsUsedByLocalJobs'  => [ '' ],
            'SlotsUsedByGridJobs'   => [ '' ],
            'Homogeneity'           => [ '' ],
            'NetworkInfo'           => [ '*' ],
            'LogicalCPUDistribution'=> [ '' ],
            'WorkingAreaShared'     => [ '' ],
            'WorkingAreaTotal'      => [ '' ],
            'WorkingAreaFree'       => [ '' ],
            'WorkingAreaLifeTime'   => [ '' ],
            'CacheTotal'            => [ '' ],
            'CacheFree'             => [ '' ],
            'TmpDir'                => [ '*' ],
            'ScratchDir'            => [ '*' ],
            'ApplicationDir'        => [ '*' ],
            'OtherInfo'             => [ '*' ],
            'Benchmark' => [ $benchmark_t ],
            'ApplicationEnvironments' => {
                'ApplicationEnvironment' => [ $app_env_t ]
            },
            'ExecutionEnvironment' => [ $exec_env_t ],
    };

    my $app_env_t = {
            'LocalID'      => [ '' ],
            'Name'         => [ '' ],
            'Version'      => [ '*' ],
            'State'        => [ '*' ],
            'LifeTime'     => [ '*' ],
            'License'      => [ '*' ],
            'Description'  => [ '*' ],
            'ParallelType' => [ '*' ],
            'MaxJobs'      => [ '*' ],
            'MaxUserSeats' => [ '*' ],
            'FreeSlots'    => [ '*' ],
            'FreeJobs'     => [ '*' ],
            'FreeUserSeats' => [ '*' ],
            'ApplicationHandle' => [ {
                'LocalID' => [ '' ],
                'Type'    => [ '' ],
                'Value'   => [ '' ],
            } ],
            'Associations' => {
                'ExecutionEnvironmentLocalID' => [ '' ],
            }
    };

    my $exec_env_t = {
            'LocalID'              => [ '' ],
            'PlatformType'         => [ '' ],
            'VirtualMachine'       => [ '' ],
            'TotalInstances'       => [ '' ],
            'UsedInstances'        => [ '' ],
            'UnavailableInstances' => [ '' ],
            'PhysicalCPUs'         => [ '' ],
            'LogicalCPUs'          => [ '' ],
            'CPUMultiplicity'      => [ '' ],
            'CPUVendor'            => [ '' ],
            'CPUModel'             => [ '' ],
            'CPUVersion'           => [ '' ],
            'CPUClockSpeed'        => [ '' ],
            'CPUTimeScalingFactor' => [ '' ],
            'WallTimeScalingFactor'=> [ '' ],
            'MainMemorySize'       => [ '' ],
            'VirtualMemorySize'    => [ '' ],
            'OSFamily'             => [ '' ],
            'OSName'               => [ '' ],
            'OSVersion'            => [ '' ],
            'ConnectivityIn'       => [ '' ],
            'ConnectivityOut'      => [ '' ],
            'NetworkInfo'          => [ '' ],
            'Associations' => {
                'ComputingShareLocalID'         => [ '' ],
                'ApplicationEnvironmentLocalID' => [ '' ],
                'BenchmarkLocalID'              => [ '*' ],
                'ComputingActivityID'           => [ '*' ],
             },
    };

my $comp_activity_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                      => [ '' ],
            'Name'                    => [ '' ],
            'Type'                    => [ '' ],
            'IDFromEndpoint'          => [ '' ],
            'LocalIDFromManager'      => [ '*' ],
            'JobDescription'          => [ '' ],
            'State'                   => [ '' ],
            'RestartState'            => [ '' ],
            'ExitCode'                => [ '*' ],
            'ComputingManagerExitCode'=> [ '*' ],
            'Error'                   => [ '*' ],
            'WaitingPosition'         => [ '*' ],
            'UserDomain'              => [ '*' ],
            'Owner'                   => [ '' ],
            'LocalOwner'              => [ '' ],
            'RequestedTotalWallTime'  => [ '*' ],
            'RequestedTotalCPUTime'   => [ '*' ],
            'RequestedSlots'          => [ '*' ],
            'RequestedApplicationEnvironment' => [ '*' ],
            'StdIn'                   => [ '*' ],
            'StdOut'                  => [ '*' ],
            'StdErr'                  => [ '*' ],
            'LogDir'                  => [ '*' ],
            'ExecutionNode'           => [ '*' ],
            'Queue'                   => [ '' ],
            'UsedTotalWallTime'       => [ '*' ],
            'UsedTotalCPUTime'        => [ '*' ],
            'UsedMainMemory'          => [ '*' ],
            'SubmissionTime'          => [ '' ],
            'ComputingManagerSubmissionTime' => [ '*' ],
            'StartTime'               => [ '*' ],
            'ComputingManagerEndTime' => [ '*' ],
            'EndTime'                 => [ '*' ],
            'WorkingAreaEraseTime'    => [ '' ],
            'ProxyExpirationTime'     => [ '' ],
            'SubmissionHost'          => [ '' ],
            'SubmissionClientName'    => [ '' ],
            'OtherMessages'           => [ '*' ],
            'Associations' => {
                'ComputingShareLocalID'  => [ '' ],
                'ComputingEndpointID'    => [ '' ],
                'ExecutionEnvironmentID' => [ '' ],
             }
    };

    my $comp_serv_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                 => [ '' ],
            'Name'               => [ '' ],
            'Capability'         => [ '' ],
            'Type'               => [ '' ],
            'QualityLevel'       => [ '' ],
            'StatusPage'         => [ '*' ],
            'Complexity'         => [ '' ],
            'Otherinfo'          => [ '*' ],
            #'Location'           => $location_t,
            #'Contact'            => $contact_t,
            'TotalJobs'          => [ '' ],
            'RunningJobs'        => [ '' ],
            'WaitingJobs'        => [ '' ],
            'StagingJobs'        => [ '' ],
            'SuspendedJobs'      => [ '' ],
            'PreLRMSWaitingJobs' => [ '' ],
            'ComputingEndpoint'  => [ $comp_endp_t ],
            'ComputingShares' => {
                'ComputingShare' => [ $comp_share_t ]
            },
            'ComputingActivities' => {
                'ComputingActivity' => [ $comp_activity_t ]
            },
            #'ComputingResource' => [ $comp_manager_t ],
            #'Associations' => {
            #    'ServiceID' => [ '*' ],
            #}
    };


    return $comp_serv_t;
}

1;
