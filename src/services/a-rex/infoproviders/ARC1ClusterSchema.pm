package ARC1ClusterSchema;

use base Exporter;
our @EXPORT_OK = qw(arc1_info_schema);

sub arc1_info_schema {

    my $location_t = {
            'ID'        => [ '' ],
            'Name'      => [ '' ],
            'Address'   => [ '*' ],
            'Place'     => [ '*' ],
            'Country'   => [ '*' ],
            'PostCode'  => [ '*' ],
            'Latitude'  => [ '*' ],
            'Longitude' => [ '*' ],
    };

    my $benchmark_t = {
            'ID'      => [ '' ],
            'Type'    => [ '' ],
            'Value'   => [ '' ],
    };

    my $contact_t = {
            'ID'        => [ '' ],
            'Type'      => [ '' ],
            'URL'       => [ '' ],
            'OtherInfo' => [ '*' ],
    };

    my $access_pol_t = {
            'ID'      => [ '' ],
            'Scheme'  => [ '' ],
            'Rule'    => [ '' ],
    };

    my $mapping_pol_t = {
            'ID'      => [ '' ],
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
            'TotalJobs'          => [ '' ],
            'RunningJobs'        => [ '' ],
            'WaitingJobs'        => [ '' ],
            'SuspendedJobs'      => [ '' ],
            'StagingJobs'        => [ '' ],
            'PreLRMSWaitingJobs' => [ '' ],
            'AccessPolicy'       => [ $access_pol_t ],
             'Associations' => {
                'ComputingShareID'      =>  [ '' ],
                'ComputingActivityID'   =>  [ '' ],
            }
    };

    my $comp_share_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                    => [ '' ],
            'Name'                  => [ '*' ],
            'Description'           => [ '*' ],
            'MappingQueue'          => [ '' ],
            'MaxWallTime'           => [ '*' ], # units: seconds
            'MaxMultiSlotWallTime'  => [ '*' ], # units: seconds
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
            'GuaranteedVirtualMemory' => [ '' ],  # units: MB
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
            'LocalSuspendedJobs'    => [ '' ],
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
            'Associations' => {
                'ComputingEndpointID'         => [ '' ],
                'ExecutionEnvironmentID'      => [ '' ],
                'ComputingActivityID'         => [ '' ],
            },
    };

    my $exec_env_t = {
            'ID'                   => [ '' ],
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
                'ComputingShareID'         => [ '' ],
                'ApplicationEnvironmentID' => [ '' ],
                'BenchmarkID'              => [ '*' ],
                'ComputingActivityID'      => [ '*' ],
             },
    };

    my $app_env_t = {
            'ID'           => [ '' ],
            'AppName'      => [ '' ],
            'AppVersion'   => [ '*' ],
            'State'        => [ '*' ],
            'LifeTime'     => [ '*' ],
            'License'      => [ '*' ],
            'Description'  => [ '*' ],
            'ParallelSupport' => [ '*' ],
            'MaxJobs'      => [ '*' ],
            'MaxUserSeats' => [ '*' ],
            'FreeSlots'    => [ '*' ],
            'FreeJobs'     => [ '*' ],
            'FreeUserSeats' => [ '*' ],
            #'ApplicationHandle' => [ {
            #    'ID'      => [ '' ],
            #    'Type'    => [ '' ],
            #    'Value'   => [ '' ],
            #} ],
            #'Associations' => {
            #    'ExecutionEnvironmentID' => [ '' ],
            #}
    };

    my $comp_manager_t = {
            'BaseType'     => '',
            'CreationTime' => '',
            'Validity'     => '',
            'ID'                    => [ '' ],
            'Name'                  => [ '*' ],
            'Type'                  => [ '' ],
            'Version'               => [ '' ],
            'Reservation'           => [ '*' ],
            'BulkSubmission'        => [ '' ],
            'TotalPhysicalCPUs'     => [ '' ],
            'TotalLogicalCPUs'      => [ '' ],
            'TotalSlots'            => [ '' ],
            'SlotsUsedByLocalJobs'  => [ '' ],
            'SlotsUsedByGridJobs'   => [ '' ],
            'Homogeneous'           => [ '' ],
            'NetworkInfo'           => [ '*' ],
            'LogicalCPUDistribution'=> [ '' ],
            'WorkingAreaShared'     => [ '' ],
            'WorkingAreaTotal'      => [ '' ],
            'WorkingAreaFree'       => [ '' ],
            'WorkingAreaLifeTime'   => [ '' ],
            'WorkingAreaMPIShared'  => [ '*' ],
            'WorkingAreaMPITotal'   => [ '*' ],
            'WorkingAreaMPIFree'    => [ '*' ],
            'WorkingAreaMPILifeTime'=> [ '*' ],
            'CacheTotal'            => [ '' ],
            'CacheFree'             => [ '' ],
            'TmpDir'                => [ '*' ],
            'ScratchDir'            => [ '*' ],
            'ApplicationDir'        => [ '*' ],
            'OtherInfo'             => [ '*' ],
            #'Benchmark' => [ $benchmark_t ],
            'ApplicationEnvironments' => {
                'ApplicationEnvironment' => [ $app_env_t ]
            },
            #'ExecutionEnvironment' => [ $exec_env_t ],
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
            'RestartState'            => [ '*' ],
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
            'WorkingAreaEraseTime'    => [ '*' ],
            'ProxyExpirationTime'     => [ '' ],
            'SubmissionHost'          => [ '' ],
            'SubmissionClientName'    => [ '' ],
            'OtherMessages'           => [ '*' ],
            'Associations' => {
                'ComputingShareID'       => [ '' ],
                'ComputingEndpointID'    => [ '' ],
                'ExecutionEnvironmentID' => [ '' ],
                'ActivityID'             => [ '' ],
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
            'ComputingManager' => [ $comp_manager_t ],
            'Associations' => {
                'ServiceID' => [ '*' ],
            }
    };


    return $comp_serv_t;
}

1;
