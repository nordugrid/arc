#!/usr/bin/python
# TODO: Document how to use.
# TODO: Add list of the plugins which provides the mappings.
# TODO: Deal with multiple values.
# TODO: Deal with fixed values.
# TODO: Deal with conditional values.
# TODO: Deal with units
# TODO: Deal with expressions

import sys, re

# File to write documentation to
outfilename = sys.argv[-1]

sourcefilename = sys.argv[1]

# Find files which contains documentation on mappings, i.e. specifies the \mapfile attribute.
mapfiles = sys.argv[2:-1]

mapdef = {"type" : "", "description" : "", "attributes" : []}

inMapdef = False
justAfterMapdef = False
sourcefile = open(sourcefilename, "r")
for line in sourcefile:
    line = line.strip().lstrip("*").lstrip()
    if inMapdef:
        if line == "\endmapdef":
            break
        
        if justAfterMapdef:
            if line == "":
                justAfterMapdef = False
                continue
    
            mapdef["description"] += line + " "
            continue
            
        if line[0:12] == "\mapdefattr ":
            mapdef["attributes"].append(line[12:].lstrip())
    elif line[0:8] == "\mapdef ":
        mapdef["type"] = line[8:].lstrip()
        inMapdef = True
        justAfterMapdef = True
        continue

sourcefile.close()    

mappings = []
for filename in mapfiles:
    m = {"id" : "", "name" : "", "notes" : [], "attributes" : {}}
    for attr in mapdef["attributes"]:
        m["attributes"][attr] = []
        
    f = open(filename, "r")

    for line in f:
        line = line.strip()
        if line[0:3] != "///":
            continue
            
        line = line[3:].lstrip()
        if line[0:9] == "\mapname ":
            regMatch = re.match("(\w+)\s+(.+)", line[9:].lstrip())
            if not regMatch:
                print "ERROR: Wrong format of the \mapname command in '%s' file" % filename
                sys.exit(1)
            
            m["id"] = regMatch.group(1)
            m["name"] = regMatch.group(2)
        elif line[0:9] == "\mapnote ":
            m["notes"].append(line[9:].lstrip())
        elif line[0:9] == "\mapattr ":
            regMatch = re.match("(.+)\s+->\s+(.+)", line[9:])
            if regMatch:
                if not m["attributes"].has_key(regMatch.group(2)):
                    print "ERROR: The '%s' attribute present in file '%s' is not defined in file '%s'" % (regMatch.group(2), filename, sourcefilename)
                    sys.exit(1)
                m["attributes"][regMatch.group(2)].append(regMatch.group(1))
    
    mappings.append(m)
    
    f.close()


outfile = open(outfilename, "w")
outfile.write("/** \n")

outfile.write("\page {type}\n{description}\n".format(**mapdef))
outfile.write("\\tableofcontents\n")

# Create mapping per attribute
outfile.write("\section attr Grouped by libarccompute attributes\n")
for attr in mapdef["attributes"]:
    outfile.write("\n\subsection attr_{attr} {attr}\n".format(attr = attr))
    outfile.write("Specialisation | Value(s)\n")
    outfile.write("---------------|---------\n")
    for m in mappings:
        if len(m["attributes"][attr]) > 0:
            outfile.write("%s | %s\n" % (m["name"], ",<br/>".join(m["attributes"][attr])))

# Create mapping per specialisation
outfile.write("\section specialisation Grouped by plugin\n")
for m in mappings:
    outfile.write("\n\subsection specialisation_{id} {name}\n".format(**m))
    if len(m["notes"]) > 0:
        outfile.write('<dl class="section attention">\n<dt>Note</dt>\n')
        for note in m["notes"]:
            outfile.write('<dd>' + note+ '</dd>\n')
        outfile.write('</dl>\n')
    outfile.write("From | To\n")
    outfile.write("-----|---\n")
    for attr, m_attrs in m["attributes"].iteritems():
        for m_attr in m_attrs:
            outfile.write("%s | %s\n" % (m_attr, attr))

outfile.write("**/\n")
