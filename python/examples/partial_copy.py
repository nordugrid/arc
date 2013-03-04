import arc
import sys

if len(sys.argv) != 2:
    print "Usage: python partial_copy.py filename"
    sys.exit(1)
    
desired_size = 512
usercfg = arc.UserConfig()
url = arc.URL(sys.argv[1])
handle = arc.DataHandle(url,usercfg)
point = handle.__ref__()
point.SetSecure(False) # GridFTP servers generally do not have encrypted data channel
info = arc.FileInfo("")
point.Stat(info)
print "Name: ", info.GetName()
fsize = info.GetSize()
if fsize > desired_size:
    point.Range(fsize-desired_size,fsize-1)
buffer = arc.DataBuffer()
point.StartReading(buffer)
while True:
    n = 0
    length = 0
    offset = 0
    ( r, n, length, offset, buf) = buffer.for_write(True)
    if not r: break
    print "BUFFER: ", offset, ": ", length, " :", buf
    buffer.is_written(n);
point.StopReading()
