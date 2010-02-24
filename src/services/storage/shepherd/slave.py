from mod_python import apache
import os,sys

CHUNK_SIZE=2**20

# Generator to buffer file chunks
def fbuffer(f, chunk_size=10000):
   while True:
      chunk = f.read(chunk_size)
      if not chunk: break
      yield chunk


def handler(req):
    """
    Function handling apache slave mode
    Checks if requested hardlink exists, opens file, deletes
    hardlink from server and PUTs or GETs requested file
    """

    req.allow_methods(['M_PUT','M_GET'])

    if req.filename.endswith('.transfering'):
        raise apache.SERVER_RETURN, apache.HTTP_FORBIDDEN

    #for i in dir(req):
    #   exec "print >> sys.stderr, \"%s\", req.%s"%(i,i)
    #sys.stderr.flush()

    if os.path.isfile(req.filename) and not req.filename.endswith('.py'):

        req_method = req.the_request[:3]

        tmp_filename=req.filename+'.transfering'

        if req_method == 'GET':
            f = open(req.filename, 'rb', CHUNK_SIZE)
        elif req_method == 'PUT':
            f = open(req.filename, 'ab', CHUNK_SIZE)

        os.rename(req.filename, tmp_filename)

        if req_method == 'GET':
            
            for chunk in fbuffer(f):
                req.write(chunk)

        elif req_method == 'PUT':
            # the request req can also be used as file handle
            for chunk in fbuffer(req):
                f.write(chunk)

        
        if req_method == 'GET':
           if req.headers_in.get("range", "").startswith("bytes=%ld"%f.tell()):
              os.remove(tmp_filename)
           else:
              os.rename(tmp_filename, req.filename)
        elif req_method == 'PUT':
           if req.headers_in.get("Content-Range", "").endswith(str(f.tell())):
              os.remove(tmp_filename)
           else:
              os.rename(tmp_filename, req.filename)
        f.close()
        return apache.OK
        
            

    else:
        raise apache.SERVER_RETURN, apache.HTTP_NOT_FOUND
