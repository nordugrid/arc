//
// The nordugrid-arc-java package is required. To compile and run this example:
// 
// export CLASSPATH=/usr/lib64/java/arc.jar:.
// export LD_LIBRARY_PATH=/usr/lib64/java
// javac DTRGenerator.java
// java DTRGenerator /bin/ls /tmp/dtrtest
// 
// The PATHs above may vary depending on ARC install location and system
// architecture. 

import nordugrid.arc.*; // For the sake of brevity in this example import everything from arc
import com.sun.security.auth.module.UnixSystem; // to get uid

// Implementation of DTR Generator.
// Cannot inherit from DTRCallback as it is a pure virtual class and swig does not
// create a default constructor, so extend Scheduler which inherits from DTRCallback.
class DTRGenerator extends Scheduler {
    private Logger logger;
    private LogDestination logdest;
    private SimpleCondition cond;

    // Create a new Generator and set up logging to stdout
    public DTRGenerator() {
        logger = new Logger(Logger.getRootLogger(), "Generator");
        logdest = new LogStream_ostream(arc.getStdout());
        Logger.getRootLogger().addDestination(logdest);
        Logger.getRootLogger().setThreshold(LogLevel.DEBUG);
        cond = new SimpleCondition();
    }

    // Implementation of callback from DTRCallback
    public void receiveDTR(DTRPointer dtr) {
        // root logger is disabled in Scheduler thread so need to add it here
        Logger.getRootLogger().addDestination(logdest);
        logger.msg(LogLevel.INFO, "Received DTR " + dtr.get_id() + " in state " + dtr.get_status().str());
        Logger.getRootLogger().removeDestinations();
        cond.signal();
    }

    // Run the transfer and wait for the callback on completion
    private void run(final String source, final String dest) {
        // Set log level for DTR (must be done before starting Scheduler)
        DTR.setLOG_LEVEL(LogLevel.DEBUG);

        // Start Scheduler thread
        Scheduler scheduler = new Scheduler();
        scheduler.start();

        // UserConfig contains information such as the location of credentials
        UserConfig cfg = new UserConfig();

        // The ID can be used to group DTRs together
        String id = "1234";

        // Logger for DTRs
        DTRLogger dtrlog = arc.createDTRLogger(Logger.getRootLogger(), "DTR");
        dtrlog.addDestination(logdest);

        // Create a DTR
        UnixSystem unix = new UnixSystem();
        DTRPointer dtr = arc.createDTRPtr(source, dest, cfg, id, (int)unix.getUid(), dtrlog);
        logger.msg(LogLevel.INFO, "Created DTR "+ dtr.get_id());

        // Register this callback in order to receive completed DTRs
        dtr.registerCallback(this, StagingProcesses.GENERATOR);
        // This line must be here in order to pass the DTR to the Scheduler
        dtr.registerCallback(scheduler, StagingProcesses.SCHEDULER);

        // Push the DTR to the Scheduler
        DTR.push(dtr, StagingProcesses.SCHEDULER);

        // Wait until callback is called
        // Note: SimpleCondition.wait() is renamed to _wait() as wait() is a java.lang.Object method
        cond._wait();

        // DTR is finished, so stop Scheduler
        scheduler.stop();
    }

    public static void main(String[] args) {
        if (args.length != 2) {
            System.out.println("Usage: java DTRGenerator source destination");
            return;
        }
        DTRGenerator gen = new DTRGenerator();
        gen.run(args[0], args[1]);
    }
}
