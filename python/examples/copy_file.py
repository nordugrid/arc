#!/usr/bin/env python

import sys
import arc

# Copies a file from source to destination

# Wait for all the background threads to exit before exiting
import atexit
@atexit.register
def wait_exit():
    arc.ThreadInitializer().waitExit()

def usage():
    sys.stdout.write(' Usage: copy_file.py source destination\n')
    
if len(sys.argv) != 3:
    usage()
    sys.exit(1)
    
# Logging to stdout
root_logger = arc.Logger_getRootLogger()
stream = arc.LogStream(sys.stdout)
root_logger.addDestination(stream)
# Set threshold to VERBOSE or DEBUG for more information
root_logger.setThreshold(arc.ERROR)

# User configuration - paths to proxy certificates etc can be set here
# With no arguments default values are used
cfg = arc.UserConfig()

# Convert the arguments to DataPoint objects
source = arc.datapoint_from_url(sys.argv[1], cfg)
if source is None:
    root_logger.msg(arc.ERROR, "Can't handle source "+sys.argv[1])
    sys.exit(1)
    
destination = arc.datapoint_from_url(sys.argv[2], cfg)
if destination is None:
    root_logger.msg(arc.ERROR, "Can't handle destination "+sys.argv[2])
    sys.exit(1)

# DataMover does the transfer
mover = arc.DataMover()
# Show transfer progress
mover.verbose(True)
# Don't attempt to retry on error
mover.retry(False)
# Do the transfer
status = mover.Transfer(source, destination, arc.FileCache(), arc.URLMap())

# Print the exit status of the transfer
sys.stdout.write("%s\n"%str(status))
