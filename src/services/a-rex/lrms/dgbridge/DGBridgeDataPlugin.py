#!/usr/bin/python
import sys
import arc
import time
import re

#get cmdline args
if not len(sys.argv)==3:
  print("Number of arguments must be 3 was: " + str(len(sys.argv)))
  sys.exit(-1)
controldir=sys.argv[1]
jobid=sys.argv[2]
filename=controldir+'/job.'+jobid+'.description'
locfile=controldir+'/job.'+jobid+'.3gdata'
inpfile=controldir+'/job.'+jobid+'.input'

print("starting 3GDataPlugin\n"+filename+"\n"+locfile+"\n\n")
#read in file
f=open(filename,'r')
rawdesc=f.read()
f.close()

jobdesc=arc.JobDescription()
if not jobdesc.Parse(rawdesc):
   sys.exit(-1)

#extract staging info
dselements=[]
while jobdesc.DataStaging.File.size()>0:
  dselements.append(jobdesc.DataStaging.File.pop())

inputfiles=""
locelements=[]
#filter staging
for dsel in dselements:
  if dsel.Source.size()==0: #not an input file
    jobdesc.DataStaging.File.append(dsel)
  else: #check for attic or http
    proto=dsel.Source[0].URI.Protocol()
    url=dsel.Source[0].URI.str()
    matchlist = re.findall(".*?;md5=.*?:size=.*?$",url) #check whether url has md5 and size set
    if (proto=='attic' or proto=='http') and len(matchlist)==1 and matchlist[0]==url : #we can put http here also, but we'll need to check for md5 and size
      locelements.append(dsel)
    else:
      jobdesc.DataStaging.File.append(dsel)
      if not dsel.Source[0].URI.Protocol()=='file':
        inputfiles = inputfiles +  dsel.Name + " " + dsel.Source[0].URI.str() + "\n"
      else:
        inputfiles = inputfiles + dsel.Name + "\n"

 
#write modified input
f=open(inpfile,'w')
f.write(inputfiles)
f.close()

print(inputfiles)
print(jobdesc.UnParse()+"\n\n")
print("attic type files\n")
#append attic and http info to .3gdata
f=open(locfile,'a')
outstr=""
for elem in locelements:
  outstr=outstr + elem.Name+ " " + elem.Source[0].URI.str() + "\n"

print(outstr)
f.write(outstr)

f.close()

sys.exit(0)
