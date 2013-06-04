#!/usr/bin/python 
#
'''
Script for adding note to SDK documentation for Java pecularity.
Add note to member field documentation that in the Java bindings the member
fields are not directly accessible, but getter and setter methods should be
used to access the member field. E.g. getX(), setX(...).
'''

import sys, os, re

# Location of generated doxygen HTML documentation
sdkDocumentationLocation = sys.argv[1]

files = [f for f in os.listdir(sdkDocumentationLocation) if os.path.isfile(sdkDocumentationLocation + '/' + f) and (f[0:5] == "class" or f[0:6] == "struct") and f[-5:] == ".html"]

for filename in files:
    doxHTMLFile = open(sdkDocumentationLocation + '/' + filename, "r")
    doxHTMLFileLines = doxHTMLFile.readlines()
    doxHTMLFile.close()

    doxHTMLFile = open(sdkDocumentationLocation + '/' + filename, "w")
    inFieldDocumentation = False
    i = 0
    while i < len(doxHTMLFileLines):
        doxHTMLFile.write(doxHTMLFileLines[i])
        if doxHTMLFileLines[i][0:] == '<h2 class="groupheader">Field Documentation</h2>\n':
            i += 1
            inFieldDocumentation = True
            continue
        
        regMatch = re.match('<h2 class="groupheader">.*', doxHTMLFileLines[i])
        if regMatch:
            i += 1
            inFieldDocumentation = False
            continue
        
        if doxHTMLFileLines[i] == '<div class="memitem">\n':
            fieldName = None
            i += 1
            while i < len(doxHTMLFileLines):
                regMatch = re.match('\s+<td class="memname">.*::([A-Za-z0-9_]+)</td>', doxHTMLFileLines[i])
                if regMatch:
                    fieldName = regMatch.group(1)
                elif doxHTMLFileLines[i] == '</div>\n' and fieldName:
                    doxHTMLFile.write('<dl class="section attention"><dt>Java interface deviation</dt><dd>The member is only accessible through the <tt>get' + fieldName + '</tt> and <tt>set' + fieldName + '</tt> methods</dd></dl>')
                    doxHTMLFile.write(doxHTMLFileLines[i])
                    break
                doxHTMLFile.write(doxHTMLFileLines[i])
                i += 1
            continue
                
        i += 1
    
    doxHTMLFile.close()
