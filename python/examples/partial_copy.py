import arc
import sys

if len(sys.argv) != 2:
    sys.stdout.write("Usage: python partial_copy.py filename\n")
    sys.exit(1)

desired_size = 512
usercfg = arc.UserConfig()
url = arc.URL(sys.argv[1])
handle = arc.DataHandle(url,usercfg)
point = handle.__ref__()
point.SetSecure(False) # GridFTP servers generally do not have encrypted data channel
info = arc.FileInfo("")
point.Stat(info)
sys.stdout.write("Name: %s\n"%str(info.GetName()))
fsize = info.GetSize()
if fsize > desired_size:
    point.Range(fsize-desired_size,fsize-1)
databuffer = arc.DataBuffer()
point.StartReading(databuffer)
while True:
    n = 0
    length = 0
    offset = 0
    ( r, n, length, offset, buf) = databuffer.for_write(True)
    if not r: break
    sys.stdout.write("BUFFER: %d :  %d  : %s\n"%(offset, length, str(buf)))
    databuffer.is_written(n)
point.StopReading()
