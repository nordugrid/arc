#!/usr/bin/python2

"""Usage: DGLog2XML.py [-h] [-d|debug] [-c inifile|--config=inifile]

   -h,--help
         Print this information

   -d,--debug
         enable debug printing (default: 0)

   -c inifile,--config=inifile
         Specify the ARC config (default: /etc/arc/service.ini)

"""

# Set to 1 to enable debug messages on stdout
debug_level = 0

def debug(msg):
    """Print debug messages. The global variable debug_level controls
       the behavior. Currently only 0 and 1 is used
    """
    global debug_level
    if debug_level == 1:
       print(msg)

def readlog(logfile):
    """Read whole logfile - deprecated"""
    f = open(logfile)
    oldlog = f.readlines()
    f.close
    newlog = []
    for line in oldlog:
       newlog.append(line.strip())
    return newlog

def line2dict(line):
    """Convert a log line to a dictionary
       It is assumed that lines are of the form attr1=val1 attr2=val2 ...
    """
    import sys
    fields = line.split(" ")
    f = {}
    for i in fields:
        pair =  i.split("=",1)
        if len(pair) != 2:
            print("""Error parsing field: "%s" in line:\n%s"""%(i,line))
            sys.exit(1)
        (attr,val) = i.split("=")
        f[attr] = val
    return f

def line2xml(line):
    """Convert a log line to XML - ignoring empty lines"""
    line = line.strip()
    if line == "":
        return ""
    else:
        return dict2xml(line2dict(line))

def dict2xml(line):

    """Translation of log between log in dictionary format to xml"""
    # This dictionary/arrayrepresents valid elements as well as the order
    order = ['dt',
             'event',
             'bridge_id',
             'job_id',
             'job_id_bridge',
             'status',
             'application',
             'input_grid_name',
            ]

    emap = {
             'dt' : 'dt',
             'event' : 'event',
             'job_id' : 'job_id',
             'job_id_bridge' : 'job_id_bridge',
             'status' : 'status',
             'application' : 'application',
             'input_grid_name' : 'input_grid_name',
             'bridge_id' : 'output_grid_name',
           }

    # Fix date 
    #pos = line['dt'].find('_')
    #line['dt'] = line['dt'][0:pos] + ' ' + line['dt'][pos+1:]
    line['dt'] = line['dt'].replace('_',' ')

    xml = ""
    for key in order:
        if key in line:
            xml += "    <%s>%s</%s>\n" %(emap[key],line[key],emap[key])
    xml = "  <metric_data>\n%s  </metric_data>\n" %xml
    return xml

def getstatus(status_file):
    """Read status file and return tuple of last log file name and index line"""
    try:
        f = open(status_file)
    except:
        debug("Problem reading %s"%status_file)
        return log_start

    for l in f.readlines():
        # Valid lines starts with 2
        if l[0] == '2':
            result = l.strip().split(' ')
            if len(result) == 2:
                f.close()
                return result
            print("Badly formatted line: %s"%l)
    f.close()
    return ["",0]

def parseini(inifile):
    """Get required info from the ARC ini file (eg. /etc/arc/service.ini)"""
    import os
    import ConfigParser
    config = ConfigParser.RawConfigParser()
    config.read(inifile)

    stage_path = config.get('common', 'dgbridge_stage_dir')
    xmldir = os.path.join(stage_path,"monitor")

    controldir = config.get('arex', 'controldir')
    logdir = os.path.join(controldir,"3gbridge_logs")

    return xmldir,logdir

def writexml(timestamp,xmldir,xml):
    """Write the log report in XML"""
    import os
    xmlfile = os.path.join(xmldir,"report_%s.xml"%str(timestamp))
    debug("Writing xml to %s"%xmlfile)
    x = open(xmlfile,"w")
    x.write(xml)
    x.close()
    return

def updatestatus(status_file,lastlog,lastline):
    """Update status file
       The file is updated with every call even though the content does not change
    """
    debug("Updating status %(lastlog)s %(lastline)s in %(status_file)s" %locals())
    f = open(status_file,"w")
    f.write("%s %s"%(lastlog,lastline))
    f.close()
    return

def main():
    """Main routine"""
    import os,time,sys
    import getopt

    global debug_level

    inifile = "/etc/arc/service.ini"

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hc:d", ["help","config","debug"])
    except getopt.error, msg:
        print msg
        print "for help use --help"
        sys.exit(2)

    for o, a in opts:
        if o in ("-h", "--help"):
            print __doc__
            sys.exit(0)
        if o in ("-c", "--config"):
            inifile = a
        if o in ("-d", "--debug"):
            debug_level=1


    xmldir,logdir = parseini(inifile)

    if not os.path.isdir(xmldir):
        debug("No such directory: %s"%xmldir)
        debug("Please create %s such that xml log files can be produced"%xmldir)
        sys.exit(1)

    status_name = "status"
    status_file = os.path.join(logdir,status_name)

    xml = ""

    entries = os.listdir(logdir)
    entries.sort()

    # Check for status file
    log_start = ["",0]
    if status_name in entries:
        log_start = getstatus(status_file)

    # Find index of the first logfile in entries
    first_logfile = 0
    if log_start[0] != "":
        first_logfile = entries.index(log_start[0])

    # Find relevant log files 
    for i in range(first_logfile,len(entries)):

        e = entries[i]
        # Only accept logs from this millenia
        if e[0] == '2':
            f = open(os.path.join(logdir,e))
            n = 0
            # If this is the first logfile skip lines
            if log_start[0] == e and log_start[1] > 0:
                for i in range(int(log_start[1])):
                    line = f.readline()
                n = int(log_start[1])

            line = f.readline()
            if line != "":
                xml += line2xml(line)

            while line:
                line = f.readline()
                n += 1
                xml += line2xml(line)
            f.close()
            lastlog = e
            lastline = n 

    timestamp = int(time.mktime(time.localtime()))

    if not xml == "":
        xml = """<report timestamp="%s" timezone="UTC">\n"""% timestamp+xml+"""</report>""" 
        writexml(timestamp,xmldir,xml)
    else:
        xml = """"""
        writexml(timestamp,xmldir,xml)

    if 'lastlog' in locals() and 'lastline' in locals():
        updatestatus(status_file,lastlog,lastline)

if __name__ == "__main__":
    main()
