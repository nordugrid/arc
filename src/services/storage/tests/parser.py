import os,sys

sum=[]

client =sys.argv[1]
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
print  '\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start))

exist  = os.path.exists('multiClient/multiClient.txt')
if exit == 'True':
    f = open('multiClient/multiClient.txt', 'a')     
    f.write('\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start)))
    f.close()
    print 'results are appended to the file: multiClient/multiClient.txt'
else:
    f = open('multiClient/multiClient.txt', 'w')
    f.write('No. of clients :\t Mim-time :\t Max-Time :\t Avg-Time :\t PerCall-Time  \n')
    f.write('\t', numthread, ':\t\t', min(sum), ':\t', max(sum), ':\t', ss/len(sum), ':\t',max(sum)/(numthread*(stop-start)))  
    f.close()
    print 'results are written to the file: multiClient/multiClient.txt'

#############################
