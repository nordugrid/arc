#!/usr/bin/env python
# TODO: Document how to use.
# TODO: Add list of the plugins which provides the mappings.
# TODO: Deal with multiple values.
# TODO: Deal with fixed values.
# TODO: Deal with conditional values.
# TODO: Deal with units
# TODO: Deal with expressions
# TODO: Deal with attributes in specialisation not mapped to library
# TODO: Deal with attributes in library which is not mapped to specialisation
#
#
# Usable commands and syntax:
# Use in library files:
# \mapdef <id> <name>\n<description-multi-lined>
# \mapdefattr <attribute-name> <prefix-reference>
# Use in specialisation files:
# \mapname <id> <name>\n<description-multi-lined>
# \mapattr <specialisation-attribute> {->|<-} <library-attribute> ["<note>"]
# \mapnote <note>

from __future__ import print_function

import sys, re

# File to write documentation to
outfilename = sys.argv[-1]

sourcefilename = sys.argv[1]

# Find files which contains documentation on mappings, i.e. specifies the \mapfile attribute.
mapfiles = sys.argv[2:-1]

mapdef = {"id" : "", "name" : "", "description" : "", "attributes" : [], "attributeprefixes" : []}

inMapdef = False
justAfterMapdef = False
# Go through library file
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
            print("ERROR: Wrong format of the \mapdefattr attribute in '%s' file on line %d" % (sourcefilename, i))
            sys.exit(1)
        mapdef["attributes"].append(regMatch.group(1))
        mapdef["attributeprefixes"].append(regMatch.group(2))
    elif line[0:8] == "\mapdef ":
        regMatch = re.match("(\w+)\s+(.+)", line[8:].lstrip())
        if not regMatch:
            print("ERROR: Wrong format of the \mapdef attribute in '%s' file on line %d" % (sourcefilename, i))
            sys.exit(1)
        mapdef["id"] = regMatch.group(1)
        mapdef["name"] = regMatch.group(2)
        inMapdef = True
        justAfterMapdef = True
        continue

sourcefile.close()    

# Go through specialisation files
mappings = []
for filename in mapfiles:
    m = {"id" : "", "name" : "", "description" : [], "notes" : [], "attributes" : {}}
    for attr in mapdef["attributes"]:
        m["attributes"][attr] = {}
        m["attributes"][attr]["in"]       = []
        m["attributes"][attr]["out"]      = []
        m["attributes"][attr]["in-note"]  = []
        m["attributes"][attr]["out-note"] = []
        
    f = open(filename, "r")
    justAfterMapName = False
    i = 0
    for line in f:
        i += 1
        line = line.strip()
        if line[0:3] != "///":
            justAfterMapName = False
            continue
            
        line = line[3:].lstrip()
        if line[0:9] == "\mapname ":
            regMatch = re.match("(\w+)\s+(.+)", line[9:].lstrip())
            if not regMatch:
                print("ERROR: Wrong format of the \mapname command in '%s' file on line %d" % (filename, i))
                sys.exit(1)
            
            m["id"] = regMatch.group(1)
            m["name"] = regMatch.group(2)
            justAfterMapName = True
        elif line[0:9] == "\mapnote ":
            justAfterMapdef = False
            m["notes"].append(line[9:].lstrip())
        elif line[0:9] == "\mapattr ":
            justAfterMapdef = False
            #     <specialisation-attr> -> <lib. attr>        ["<note>"]
            regMatch = re.match("(.+)\s+->\s+([^\s]+)(?:\s+\"([^\"]+)\")?", line[9:])
            if regMatch:
                if regMatch.group(2) not in m["attributes"]:
                    print("ERROR: The '%s' attribute present in file '%s' on line %d is not defined in file '%s'" % (regMatch.group(2), filename, i, sourcefilename))
                    sys.exit(1)
                m["attributes"][regMatch.group(2)]["in"].append(regMatch.group(1))
                if regMatch.group(3):
                    m["attributes"][regMatch.group(2)]["in-note"].append(regMatch.group(3))
                continue

            regMatch = re.match("(.+)\s+<-\s+([^\s]+)(?:\s+\"([^\"]+)\")?", line[9:])
            if regMatch:
                if regMatch.group(2) not in m["attributes"]:
                    print("ERROR: The '%s' attribute present in file '%s' on line %d is not defined in file '%s'" % (regMatch.group(2), filename, i, sourcefilename))
                    sys.exit(1)
                m["attributes"][regMatch.group(2)]["out"].append(regMatch.group(1))
                if regMatch.group(3):
                    m["attributes"][regMatch.group(2)]["out-note"].append(regMatch.group(3))
                continue
        elif justAfterMapName:
            m["description"].append(line)
    
    mappings.append(m)
    
    f.close()


# Write mapping to doxygen formatted file.
outfile = open(outfilename, "w")
outfile.write("/** \n")

outfile.write("\\page {id} {name}\n{description}\n".format(**mapdef))
outfile.write("\\tableofcontents\n")

# Create mapping per lib. attribute
outfile.write("\\section attr Grouped by libarccompute attributes\n")
for i in range(len(mapdef["attributes"])):
    outfile.write("\n\\subsection attr_{formatted_attr} {attr}\n".format(formatted_attr = re.sub('::', "_", mapdef["attributes"][i]), attr = mapdef["attributes"][i]))
    outfile.write("\\ref {prefix}::{attr} \"Attribute description\"\n\n".format(attr = mapdef["attributes"][i], prefix = mapdef["attributeprefixes"][i]))

    has_input = has_output = False
    attributes_to_write_to_table = ""
    for m in mappings:
        has_input  = has_input  or m["attributes"][mapdef["attributes"][i]]["in"]
        has_output = has_output or m["attributes"][mapdef["attributes"][i]]["out"]

    notes = []
    for m in mappings:
        attr = m["attributes"][mapdef["attributes"][i]]
        if attr["in"] or attr["out"]:
            attributes_to_write_to_table += "| %s |" % (m["name"])
            if has_input:
                attributes_to_write_to_table += " %s" % (",<br/>".join(attr["in"]))
                if attr["in-note"]:
                    attributes_to_write_to_table += "<sup>[%s]</sup>" % ("][".join(str(x) for x in range(len(notes)+1, len(notes)+1+len(attr["in-note"]))))
                    notes += attr["in-note"]
                attributes_to_write_to_table += " |" if has_output else ""
            if has_output:
                attributes_to_write_to_table += " %s" % (",<br/>".join(attr["out"]))
                if attr["out-note"]:
                    attributes_to_write_to_table += "<sup>[%s]</sup>" % ("][".join(str(x) for x in range(len(notes)+1, len(notes)+1+len(attr["out-note"]))))
                    notes += attr["out-note"]
            attributes_to_write_to_table += " |\n"
    if attributes_to_write_to_table and (has_input or has_output):
        table_header  = "| Specialisation"
        table_header += " | Input"  if has_input  else ""
        table_header += " | Output" if has_output else ""
        outfile.write(table_header + " |\n")
        outfile.write(re.sub(r'[ \w]', '-', table_header) + " |\n")
        outfile.write(attributes_to_write_to_table)
        if notes:
            outfile.write("<b>Notes:</b><ol><li>%s</li></ol>" % ("</li><li>".join(notes)))
    else:
        outfile.write("No specialisations maps attributes to this field/value.\n")

# Create mapping per specialisation
outfile.write("\\section specialisation Grouped by plugin\n")
for m in mappings:
    outfile.write("\n\\subsection specialisation_{id} {name}\n".format(**m))
    if m["description"]:
        outfile.write(" ".join(m["description"]) + "\n")
    if len(m["notes"]) > 0:
        outfile.write('<dl class="section attention">\n<dt>Note</dt>\n')
        for note in m["notes"]:
            outfile.write('<dd>' + note+ '</dd>\n')
        outfile.write('</dl>\n')
    has_input = has_output = False
    for attr, m_attrs in m["attributes"].items():
        has_input  = has_input  or bool(m_attrs["in"])
        has_output = has_output or bool(m_attrs["out"])
    table_header  = "| Input "  if has_input  else ""
    table_header += "| Lib. attr. |"
    table_header += " Output |" if has_output else ""
    outfile.write(table_header + "\n")
    outfile.write(re.sub(r'[. \w]', '-', table_header) + "\n")
    
    notes = []
    for attr, m_attrs in m["attributes"].items():
        if not m_attrs["in"] and not m_attrs["out"]:
            continue
        line = ""
        if has_input:
            line += "| %s" % (", ".join(m_attrs["in"]))
            if m_attrs["in-note"]:
                line += "<sup>[%s]</sup>" % ("][".join(str(x) for x in range(len(notes)+1, len(notes)+1+len(m_attrs["in-note"]))))
                notes += m_attrs["in-note"]
            line += " "
        line += "| \\ref Arc::" + attr + ' "' + attr + '" |'
        if has_output:
            line += " %s" % (", ".join(m_attrs["out"]))
            if m_attrs["out-note"]:
                line += "<sup>[%s]</sup>" % ("][".join(str(x) for x in range(len(notes)+1, len(notes)+1+len(m_attrs["out-note"]))))
                notes += m_attrs["out-note"]
            line += " |"
        outfile.write(line + '\n')
    if notes:
        outfile.write("<b>Notes:</b><ol><li>%s</li></ol>" % ("</li><li>".join(notes)))
    

outfile.write("**/\n")
