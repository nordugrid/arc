import os,sys

sum=[]

client =sys.argv[1]
numthread = int(sys.argv[2])
numfile = 0 
list = os.listdir('multiClient/client-'+client)
for filename in list: 
    if 'client' in filename:	
        file = open('multiClient/client-'+client+'/'+filename)
        ind = 0
        numfile = numfile +1
        tmp = 0
        while 1:
            line = file.readline()
            if not line:
                break
    #if '- done ' in line:
            print line[2:10]
            tmp = tmp+float(line[2:10])	
            ind = ind + 1
        sum.append(tmp)
print sum
ss = 0 
print 'total number of files ',numfile
for val in sum:
    ss = ss + val
#print 'average ', ss/len(sum)    

#############################

#list = os.listdir('multiClient/client-'+clientnum)
#for filename in list:
#    file = open('multiClient/client-'+clientnum+'/'+filename)
#    ind = 0
#    tmp = 0.0
#    while 1:
#        line = file.readline()
#        if not line:
#            break
    #if '- done ' in line:
#        print line[2:10]
#        tmp = tmp+float(line[2:10])
#        ind = ind + 1
#    file.close()
#    sum.append(tmp)
#    print 'total values', ind 
#ss = 0
#for val in sum:
#    ss = ss + val
#print sum

print 'No. of clients :\t Mim-time :\t Max-Time :\t Avg-Time :\t PerCall-Time  \n' 
print  '\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t', max(sum)/(numthread*(ind)), '\n'

exist  = os.path.exists('multiClient/multiClient.txt')
print exist
if exist == True:
    print '\n file exist:'
    f = open('multiClient/multiClient.txt', 'a')     
    f.write('\t'+str(numthread)+'\t\t'+str(min(sum))+'\t'+str(max(sum))+'\t'+str(ss/len(sum))+'\t'+str(max(sum)/(numthread*(ind)))+'\n')
    f.close()
    print 'results are appended to the file: multiClient/multiClient.txt'
else:
    print 'new file created:'
    f = open('multiClient/multiClient.txt', 'w')
    f.write('No. of clients :\t Mim-time :\t Max-Time :\t Avg-Time :\t PerCall-Time  \n')
    f.write('\t'+str(numthread)+'\t\t'+str(min(sum))+'\t'+str(max(sum))+'\t'+str(ss/len(sum))+'\t'+str(max(sum)/(numthread*(ind)))+'\n') 
    f.close()
    print 'results are written to the file: multiClient/multiClient.txt'

#############################
