//
// The nordugrid-arc-java package is required. To compile and run this example:
// 
// export CLASSPATH=/usr/lib64/java/arc.jar:.
// export LD_LIBRARY_PATH=/usr/lib64/java
// javac ResourceDiscovery.java
// java ResourceDiscovery ldap://index1.nordugrid.org/Mds-vo-name=NorduGrid,o=grid
//  
// The PATHs above may vary depending on ARC install location and system
// architecture. 

import nordugrid.arc.*; // For the sake of brevity in this example import everything from arc

public class ResourceDiscovery
{
    public static void main(String[] args)
    {
        // Set up logging to stderr with level VERBOSE (a lot of output will be shown)
        LogStream_ostream logstdout = new LogStream_ostream(nordugrid.arc.arc.getStdout());
        logstdout.setFormat(nordugrid.arc.LogFormat.ShortFormat);
        Logger.getRootLogger().addDestination(logstdout);
        Logger.getRootLogger().setThreshold(nordugrid.arc.LogLevel.VERBOSE);
        Logger logger = new Logger(Logger.getRootLogger(), "resourcediscovery");

        // Create Endpoint object from the passed argument (registry or index service)
        Endpoint e = new Endpoint(args[0], Endpoint.CapabilityEnum.REGISTRY);
        
        // This object holds various attributes, including proxy location and selected services.
        UserConfig uc = new UserConfig("");
        
        // Create a instance for discovering computing services at the registry service.
        ComputingServiceRetriever csr = new ComputingServiceRetriever(uc);
        csr.addEndpoint(e); // Add endpoint ... which initiates discovery.
        csr._wait(); // Wait for results to be retrieved.
        
        ExecutionTargetList etl = new ExecutionTargetList();
        csr.GetExecutionTargets(etl);
        for (ExecutionTarget et : etl) {
            System.out.println("Execution Target on Computing Service: " + et.getComputingService().getName());
            if (!et.getComputingEndpoint().getURLString().isEmpty()) {
                System.out.println(" Computing endpoint URL: " + et.getComputingEndpoint().getURLString());
            }
            if (!et.getComputingEndpoint().getInterfaceName().isEmpty()) {
                System.out.println(" Computing endpoint interface name: " + et.getComputingEndpoint().getInterfaceName());
            }
            if (!et.getComputingShare().getName().isEmpty()) {
                System.out.println(" Queue: " + et.getComputingShare().getName());
            }
            if (!et.getComputingShare().getMappingQueue().isEmpty()) {
                System.out.println(" Mapping queue: " + et.getComputingShare().getMappingQueue());
            }
            if (!et.getComputingEndpoint().getHealthState().isEmpty()) {
                System.out.println(" Health state: " + et.getComputingEndpoint().getHealthState());
            }
        }
    }
}
