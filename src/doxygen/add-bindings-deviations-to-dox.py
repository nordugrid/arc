#!/usr/bin/env python
#
'''
Script for parsing Swig interface files (.i) and extracting renames (%rename)
and ignores (%s), and adding that information to the doxygen generated HTML.

Usage: add-bindings-deviations-to-dox.py <swig.i> <doxygen-html-dir>
E.g.: add-bindings-deviations-to-dox.py swig-interface.i dox/html

Limitations:
* Unable to handle #else or #elif statements.
* Unable to handle templates.
'''

import sys, re
from os.path import isfile

# Location of swig file
filename = sys.argv[1]
# Location of generated doxygen HTML documentation
sdkDocumentationLocation = sys.argv[2]

# Use list to deal with scoping of #if and #ifdef statements.
inIfdef = []

# Use dictionary below to group %rename and %ignore statements per HTML file.
expressionsFound = {}

f = open(filename, "r")
for line in f:
    line = line.strip()
    regMatch = re.match('\A#if(n?def)?\s+(\w+)', line)
    if regMatch:
        inIfdef.append(regMatch.group(2))
        #print " #ifdef %s" % inIfdef
        continue
    regMatch = re.search('\A#endif', line)
    if regMatch:
        #print " #endif // %s" % inIfdef
        inIfdef.pop()
        continue
    regMatch = re.match('%ignore\s+([^;]+)', line)
    if regMatch:
        ignoredName = regMatch.group(1)
        #print "Expression ignored: %s" % ignoredName
        regMatch = re.match('\A(Arc|ArcCredential|AuthN|DataStaging)::([^:<]+)(<([^:>]+(::[^:>]+)+)>)?::(.*)', ignoredName)
        if regMatch:
            namespaceName, className, _, templateParameters, _, methodName = regMatch.groups()
            if templateParameters:
                #print "Found template: %s::%s<%s>::%s" % (namespaceName, className, templateParameters, methodName)
                print "Error: Unable to handle template signatures %s" % ignoredName
                continue
            #print " Ignoring method '%s' in class '%s' in Arc namespace." % (methodName, className)
            sdkFNOfIgnoredInstance = sdkDocumentationLocation + '/class' + namespaceName + '_1_1' + className + '.html'
            if not expressionsFound.has_key(sdkFNOfIgnoredInstance):
                expressionsFound[sdkFNOfIgnoredInstance] = []
            ignoreScope = ["Python"] if "SWIGPYTHON" in inIfdef else ["Python"]
            expressionsFound[sdkFNOfIgnoredInstance].append({"text" : "Method is unavailable", "scope" : ignoreScope, "name" : methodName})
            continue
        print "Error: Couldn't parse ignore signature %s" % ignoredName
        continue
        
    regMatch = re.match('%rename\(([^)]+)\)\s+([^;]+)', line)
    if regMatch:
        #print "Expression '%s' renamed to '%s'" % (regMatch.group(2), regMatch.group(1))
        toName, renameFullName = regMatch.groups()
        regMatch = re.match('\A(Arc|ArcCredential|AuthN|DataStaging)::([^:<]+)(<([^:>]+(::[^:>]+)+)>)?::(.*)', renameFullName)
        if regMatch:
            namespaceName, className, _, templateParameters, _, methodName = regMatch.groups()
            if templateParameters:
                #print "Found template: %s::%s<%s>::%s" % (namespaceName, className, templateParameters, methodName)
                print "Error: Unable to handle template signatures %s" % renameFullName
                continue
            #print " Ignoring method '%s' in class '%s' in Arc namespace." % (methodName, className)
            sdkFNOfRenamedInstance = sdkDocumentationLocation + '/class' + namespaceName + '_1_1' + className + '.html'
            if not expressionsFound.has_key(sdkFNOfRenamedInstance):
                expressionsFound[sdkFNOfRenamedInstance] = []
            renameScope = ["Python"] if "SWIGPYTHON" in inIfdef else ["Python"]
            expressionsFound[sdkFNOfRenamedInstance].append({"text" : "Renamed to <tt>" + toName + "</tt>", "scope" : renameScope, "name" : methodName})
            continue
        print "Error: Couldn't parse rename signature %s" % renameFullName
        continue
f.close()

#print expressionsFound

for filename, v in expressionsFound.iteritems():
    if not isfile(filename):
        print "Error: No such file %s" % filename
        continue
    doxHTMLFile = open(filename, "r")
    doxHTMLFileLines = doxHTMLFile.readlines()
    doxHTMLFile.close()

    doxHTMLFile = open(filename, "w")
    i = 0
    while i < len(doxHTMLFileLines):
        doxHTMLFile.write(doxHTMLFileLines[i])
        regMatch = re.match('\s+<td class="memname">(.+)</td>', doxHTMLFileLines[i])
        if not regMatch:
            i += 1
            continue
        doxMethodName = regMatch.group(1).strip()
        #print doxMethodName

        for entry in v:
            regMatch = re.match("(operator\(\)|[^(]+)"  "(\(([^(]*)\))?"  "\s*(const)?", entry["name"])
            if regMatch:
              methodName, _, methodParameters, isConst = regMatch.groups()
              #print "Method name: '%s'; Parameters: '%s'; isConst: %s" % (methodName, methodParameters, str(bool(isConst)))
              #print "'%s\Z', %s" % (methodName.strip(), doxMethodName)
              doxMethodName = doxMethodName.replace("&gt;", ">")
              if doxMethodName.endswith(methodName.strip()):
                  #print "Method '%s' found in file '%s' as '%s'" % (methodName, filename, doxMethodName)

                  isInsideMemdocDiv = False
                  methodParameters = methodParameters.split(",") if methodParameters else []
                  while True:
                      i += 1
                      regMatch = re.match('\s+<td class="paramtype">(.+)</td>', doxHTMLFileLines[i])
                      if regMatch:
                          doxParam = regMatch.group(1).replace("&#160;", "").replace(" &amp;", "\s*&").strip()
                          doxParam = re.sub('</?a[^>]*>', '', doxParam) # Remove anchor tags  
                          if len(methodParameters) == 0:
                              if doxParam != "void": # Doesn't match that in HTML document
                                doxHTMLFile.write(doxHTMLFileLines[i])
                                break
                          elif re.match(doxParam, methodParameters[0]):
                              methodParameters.pop(0)
                      elif isInsideMemdocDiv and re.match('</div>', doxHTMLFileLines[i]):
                          if len(methodParameters) > 0: # Doesn't match that in HTML document
                              doxHTMLFile.write(doxHTMLFileLines[i])
                              break
                          for scope in entry["scope"]:
                            doxHTMLFile.write('<dl class="section attention"><dt>' + scope + ' interface deviation</dt><dd>' + entry["text"] + ' in ' + scope + ' interface</dd></dl>')
                          v.remove(entry)
                          doxHTMLFile.write(doxHTMLFileLines[i])
                          break
                      elif re.search('<div class="memdoc">', doxHTMLFileLines[i]):
                          isInsideMemdocDiv = True
                      doxHTMLFile.write(doxHTMLFileLines[i])
                  
                  break
            else:
              print "Error: Unable to parse method signature %s" % entry["name"]
        i += 1
    
    doxHTMLFile.close()
    
    if v:
        print "Error: The following methods was not found in the HTML file '%s':" % filename
        for entry in v:
            print "       %s" % entry["name"]
        print "??? => Is there a API description in the corresponding header file for these?"
    
    

