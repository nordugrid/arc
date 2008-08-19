from mod_python import apache
import os

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

    if os.path.isfile(req.filename):

        req_method = req.the_request[:3]

        if not (req_method == 'GET' or req_method == 'PUT'):
            raise apache.SERVER_RETURN, apache.HTTP_METHOD_NOT_ALLOWED

        if req_method == 'GET':
            f = open(req.filename, 'rb', CHUNK_SIZE)
        elif req_method == 'PUT':
            f = open(req.filename, 'wb', CHUNK_SIZE)

        os.remove(req.filename)

        if 'GET' in req.the_request:
            
            for chunk in fbuffer(f):
                req.write(chunk)

        elif 'PUT' in req.the_request:
            # the request req can also be used as file handle
            for chunk in fbuffer(req):
                f.write(chunk)


        f.close()
        
        return apache.OK
        
            

    else:
        return apache.DECLINED
