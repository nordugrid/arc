#!/usr/bin/python
# TODO: Document how to use.
# TODO: Add list of the plugins which provides the mappings.
# TODO: Deal with multiple values.
# TODO: Deal with fixed values.
# TODO: Deal with conditional values.
# TODO: Deal with units
# TODO: Deal with expressions
# TODO: Deal with attributes in specialisation not mapped to library
# TODO: Deal with attributes in library which is not mapped to by specialisation
# TODO: Add description of mapping specialisation
# TODO: Deal with attribute notes.

import sys, re

# File to write documentation to
outfilename = sys.argv[-1]

sourcefilename = sys.argv[1]

# Find files which contains documentation on mappings, i.e. specifies the \mapfile attribute.
mapfiles = sys.argv[2:-1]

mapdef = {"id" : "", "name" : "", "description" : "", "attributes" : [], "attributeprefixes" : []}

inMapdef = False
justAfterMapdef = False
sourcefile = open(sourcefilename, "r")
i = 0
for line in sourcefile:
    i += 1
    line = line.strip().lstrip("*").lstrip()
    if line[0:3] == "///":
        line = line.lstrip("/").lstrip()
    if justAfterMapdef:
        if line == "" or line == "/":
            justAfterMapdef = False
            continue

        mapdef["description"] += line + " "
        continue
    elif line[0:12] == "\mapdefattr ":
        regMatch = re.match("([^\s]+)\s+([^\s]+)", line[12:].lstrip())
        if not regMatch:
            print "ERROR: Wrong format of the \mapdefattr command in '%s' file on line %d" % (sourcefilename, i)
            sys.exit(1)
        mapdef["attributes"].append(regMatch.group(1))
        mapdef["attributeprefixes"].append(regMatch.group(2))
    elif line[0:8] == "\mapdef ":
        regMatch = re.match("(\w+)\s+(.+)", line[8:].lstrip())
        if not regMatch:
            print "ERROR: Wrong format of the \mapdef command in '%s' file on line %d" % (sourcefilename, i)
            sys.exit(1)
        mapdef["id"] = regMatch.group(1)
        mapdef["name"] = regMatch.group(2)
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
    i = 0
    for line in f:
        i += 1
        line = line.strip()
        if line[0:3] != "///":
            continue
            
        line = line[3:].lstrip()
        if line[0:9] == "\mapname ":
            regMatch = re.match("(\w+)\s+(.+)", line[9:].lstrip())
            if not regMatch:
                print "ERROR: Wrong format of the \mapname command in '%s' file on line %d" % (filename, i)
                sys.exit(1)
            
            m["id"] = regMatch.group(1)
            m["name"] = regMatch.group(2)
        elif line[0:9] == "\mapnote ":
            m["notes"].append(line[9:].lstrip())
        elif line[0:9] == "\mapattr ":
            regMatch = re.match("(.+)\s+->\s+(.+)", line[9:])
            if regMatch:
                if not m["attributes"].has_key(regMatch.group(2)):
                    print "ERROR: The '%s' attribute present in file '%s' on line %d is not defined in file '%s'" % (regMatch.group(2), filename, i, sourcefilename)
                    sys.exit(1)
                m["attributes"][regMatch.group(2)].append(regMatch.group(1))
    
    mappings.append(m)
    
    f.close()


outfile = open(outfilename, "w")
outfile.write("/** \n")

outfile.write("\\page {id} {name}\n{description}\n".format(**mapdef))
outfile.write("\\tableofcontents\n")

# Create mapping per attribute
outfile.write("\\section attr Grouped by libarccompute attributes\n")
for i in range(len(mapdef["attributes"])):
    outfile.write("\n\\subsection attr_{formatted_attr} {attr}\n".format(formatted_attr = re.sub('::', "_", mapdef["attributes"][i]), attr = mapdef["attributes"][i]))
    outfile.write("\\ref {prefix}::{attr} \"Attribute description\"\n\n".format(attr = mapdef["attributes"][i], prefix = mapdef["attributeprefixes"][i]))

    attributes_to_write_to_table = ""
    for m in mappings:
        if len(m["attributes"][mapdef["attributes"][i]]) > 0:
            attributes_to_write_to_table += "%s | %s\n" % (m["name"], ",<br/>".join(m["attributes"][mapdef["attributes"][i]]))
    if attributes_to_write_to_table:
      outfile.write("Specialisation | Value(s)\n")
      outfile.write("---------------|---------\n")
      outfile.write(attributes_to_write_to_table)
    else:
      outfile.write("No specialisations maps attributes to this field/value.\n")

# Create mapping per specialisation
outfile.write("\\section specialisation Grouped by plugin\n")
for m in mappings:
    outfile.write("\n\\subsection specialisation_{id} {name}\n".format(**m))
    if len(m["notes"]) > 0:
        outfile.write('<dl class="section attention">\n<dt>Note</dt>\n')
        for note in m["notes"]:
            outfile.write('<dd>' + note+ '</dd>\n')
        outfile.write('</dl>\n')
    outfile.write("From | To\n")
    outfile.write("-----|---\n")
    for attr, m_attrs in m["attributes"].iteritems():
        for m_attr in m_attrs:
            outfile.write('%s | \\ref Arc::%s "%s"\n' % (m_attr, attr, attr))

outfile.write("**/\n")
