#!/local/bin/python

# fuse stuff
from fuse import Fuse
import stat, os, sys, pwd
from errno import *
from stat import *
import fuse

if not hasattr(fuse, '__version__'):
    raise RuntimeError, \
        "your fuse-py doesn't know of fuse.__version__, probably it's too old."

fuse.fuse_python_api = (0,2)

# to avoid fuse to look in /lib on host fs
if os.environ.has_key('LD_LIBRARY_PATH'):
    LD_LIBRARY_PATH = os.environ['LD_LIBRARY_PATH']
    if LD_LIBRARY_PATH[0] == ':' or LD_LIBRARY_PATH[-1] == ':':
        print "LD_LIBRARY_PATH=%s ends and/or starts with :"%LD_LIBRARY_PATH
        print "Please fix your LD_LIBRARY_PATH!"

#fuse.feature_assert('stateful_files', 'has_init')

# arc storage stuff
from storage.client import ManagerClient, ByteIOClient
from storage.common import create_checksum, mkuid
from storage.common import false, true

import time

MNT=sys.argv[1]
try:
    MOUNT=sys.argv[2]
    del(sys.argv[2])
except:
    MOUNT="http://localhost:60000/Manager"

manager = ManagerClient(MOUNT, False)

PROTOCOL = 'byteio'
needed_replicas = 2

FUSETRANSFER = os.path.join(os.getcwd(),'fuse_transfer')
if not os.path.isdir(FUSETRANSFER):
    os.mkdir(FUSETRANSFER)

DEBUG = 1

debugfile = open('arcfsmessages','a') 


def debug_msg(msg, msg2=''):
    """
    Function to get readable debug output to file
    """
    if DEBUG:
        print >> debugfile, MOUNT+":", msg, msg2
        debugfile.flush()
    
debug_msg(sys.argv)

def flag2mode(flags):
    """
    Function to get correct mode from flags for os.fdopen
    """
    md = {os.O_RDONLY: 'r', os.O_WRONLY: 'w', os.O_RDWR: 'w+'}
    m = md[flags & (os.O_RDONLY | os.O_WRONLY | os.O_RDWR)]

    if flags | os.O_APPEND:
        m = m.replace('w', 'a', 1)

    return m


def sym2oct(symmode, symlink=False):
    """
    Hackish function to get octal mode bits
    Example
    input: '-rw-r--r--'
    output: 0644
    """
    if len(symmode)!=10:
        print "bad symmode",symmode
    type=symmode[0]
    owner=symmode[1:4]
    grp=symmode[4:7]
    all=symmode[7:10]
    o=g=a=s=0
    if 'r' in owner: o+=4 
    if 'w' in owner: o+=2 
    if 'x' in owner: o+=1
    if 'S' in owner: s+=4
    if 's' in owner: s+=4;o+=1
    if 'r' in grp: g+=4 
    if 'w' in grp: g+=2 
    if 'x' in grp: g+=1
    if 'S' in grp: s+=4
    if 's' in grp: s+=4;g+=1
    if 'r' in all: a+=4 
    if 'w' in all: a+=2 
    if 'x' in all: a+=1
    if 'T' in all: s+=4
    if 't' in all: s+=4;a+=1
    f=0;sl=0
    if type=='d': f=4
    elif type=='c': f=2
    elif type=='l': f=2; sl=1
    else:
        sl=1
        if symlink:
            f=2
    return sl*8**5+f*8**4+s*8**3+o*8**2+g*8+a


class GenStat(fuse.Stat):
    """
    Default class for stat attributes
    """
    def __init__(self):
        self.st_mode  = 0
        self.st_ino   = 0
        self.st_dev   = 0
        self.st_nlink = 0
        self.st_uid   = 0
        self.st_gid   = 0
        self.st_size  = 0
        self.st_atime = 0
        self.st_mtime = 0
        self.st_ctime = 0


class ARCInode:
    """
    Class used to store ARCFS inode
    """
    
    def __init__(self, mode, nlink, uid, gid, size, mtime, 
                 metadata, atime=0, ctime=0, closed=0, entries=[]):
        debug_msg("called ARCInode.__init__")
        self.st = GenStat()
        self.st.st_mode  = mode
        self.st.st_nlink = nlink
        self.st.st_uid   = uid
        self.st.st_gid   = gid
        self.st.st_size  = size
        self.st.st_mtime = mtime
        self.st.st_atime = atime
        self.st.st_ctime = ctime
        self.closed      = closed
        self.entries     = entries
        self.metadata    = metadata
        debug_msg("left ARCInode.__init__")
    

class ARCFS(Fuse):
    """
    Class for ARC FUSE file system. Interface to FUSE and ARC Storage System 
    Usage:
    
    - instantiate

    - add options to `parser` attribute (an instance of `FuseOptParse`)
    
    - call `parse`
    
    - call `main`    
    """

    def __init__(self, *args, **kw):
        """
        init Fuse, manager is ManagerClient(MOUNT, False)
        """
        Fuse.__init__(self, *args, **kw)
        debug_msg('fuse initiated')
        self.manager = manager 
        debug_msg('left ARCFS.__init__')


    def getattr(self, path):
        """
        get stat from path
        gets inode and returns inode.stat
        """
        debug_msg('called getattr',path)
        inod = self.getinode(path)
        if inod:
            return inod.st
        else: return -ENOENT

        
    def mkinod(self, path):
        """
        Function to get metadata of path from Manager and parse it to ARCInode
        Returns ARCInode
        """
        debug_msg('mkinod called:', (path))
        request = {'0':path}
        metadata = self.manager.stat(request)['0']
        if not isinstance(metadata,dict):
            # no metadata
            debug_msg('mkinod left, no metadata', (path))
            return None
        is_dir = metadata[('catalog', 'type')] == 'collection'
        is_file = metadata[('catalog', 'type')] == 'file'
        if is_file:
            nlink = 1
            size = long(metadata[('states','size')])
            if len([status for (section, location), status in 
                    metadata.items() if 
                    section == 'locations' and status == 'alive']) == \
                    int(metadata[('states', 'neededReplicas')]):
                status = 'alive'
            else:
                status = 'not ready'
            # we don't know the mode so we set something
            mode = sym2oct('-rw-r--r--')
            closed = 0
            entries = []
        elif is_dir:
            # + 2 for . and ..
            nlink = len([guid for (section, file), guid in 
                         metadata.items() if section == 'entries'])+2 
            closed = int(metadata[('states', 'closed')])
            mode = sym2oct('drwxr-xr-x',False)
            size = 0
            entries = [fname for (section, fname), guid in metadata.items()
                       if section == 'entries']
        else:
            debug_msg('mkinod left, nothing', (path))
            # neither file nor collection
            return None
        if metadata.has_key(('timestamps', 'created')):
            ctime = float(metadata[('timestamps', 'created')])
        else:
            ctime = 0
        if metadata.has_key(('timestamps', 'modified')):
            mtime = float(metadata[('timestamps', 'modified')])
        else:
            mtime = ctime
        atime = mtime
        # no way to get uid and gid in ARC storage (yet) so we use users uid
        uid = os.getuid()
        gid = os.getgid()
        debug_msg('mkinod left:', (path))

        return ARCInode(mode, nlink, uid, gid, size, mtime, 
                        metadata, atime, ctime, closed, entries)


    def readdir(self, path, offset):
        """
        function yielding Direntries
        calls getinode once for directory, then uses entries list
        in dir inode
        """
        debug_msg('readdir called:', path)
        for r in '.','..':
            yield fuse.Direntry(r)
        dirnode = self.getinode(path)
        for fname in dirnode.entries:
            yield fuse.Direntry(fname)


    def mknod(self, path, mode, dev):
        """
        Function called if new resource will be created
        Check if mode is reasonable and if it already exists
        else put path in self.linked
        """
        debug_msg('mknod called:', (path, mode, dev))
        if not (S_ISREG(mode) | S_ISFIFO(mode) | S_ISSOCK(mode)):
            return -EINVAL
        if self.manager.stat({'0':path})['0']:
            return -EEXIST
        debug_msg('mknod left', (path, mode, dev))


    def unlink(self, path):
        """
        Function called when removing file/link
        """
        debug_msg('unlink called:', path)
        response = self.manager.delFile({'0':path})
        debug_msg('delFile responded', response['0'])
        debug_msg('unlink left:', path)


    def link(self, path, path1):
        debug_msg('link called:', (path, path1))
        request = {'0' : (path, path1, True)}
        response = self.manager.move(request)
        debug_msg('link left', response)


    def rename(self, path, path1):
        debug_msg('rename called:', (path, path1))
        request = {'0' : (path, path1, False)}
        response = self.manager.move(request)
        debug_msg('rename left', response)


    def mkdir(self, path, mode):
        debug_msg('mkdir called:', path)
        request = {'0': (path, {('states', 'closed') : false})}
        response = self.manager.makeCollection(request)
        debug_msg('mkdir left:', path)


# rmdir requires manager.unmakeCollection which does not exist yet
#     def rmdir(self, path):
#         debug_msg('rmdir called:', path)
#         request = {'0': path}
#         response = self.manager.unmakeCollection(request)['0']
#         if status == 'collection not empty':
#             return -ENOTEMPTY
#         debug_msg('rmdir left:', path)


    def getinode(self,path):
        debug_msg('getinode called:', path)
        try:
            inod = self.mkinod(path)
            return inod
        except:
            return None


    def fsinit(self):
        """
        Does whatever needed to get the FS up and running
        """
        debug_msg('fsinit called')
        # make sure we have root collection
        self.manager.makeCollection({'0':('/', {('states', 'closed'):false})})
        debug_msg('fsinit left')


    class ARCFSFile(object):
        """
        Class taking care of file spesific stuff like write and read,
        lock and unlock (or rather __init__ and release)
        """
        
        def __init__(self, path, flags, *mode):
            """
            Initiate file type object
            asks manager to get file, and if not found open a new file
            """
            debug_msg('Called ARCFSFile.__init__',(path,flags,mode))
            self.manager = manager
            self.transfer = FUSETRANSFER
            self.path = path
            self.tmp_path = os.path.join(self.transfer, mkuid())
            request = {'0' : (path, [PROTOCOL])}
            success, turl, protocol = self.manager.getFile(request)['0']
            if success == 'not found':
                self.creating = True
                # to make sure the file is unique:
                self.file = os.fdopen(os.open(self.tmp_path, flags, *mode),
                                      flag2mode(flags))
                self.fd = self.file.fileno()
                # notify storage about file
                # set size and neededReplicas to 0, checksum to ''
                # correct values will be set in release
                metadata = {('states', 'size') : 0, ('states', 'checksum') : '',
                            ('states', 'checksumType') : 'md5', ('states', 'neededReplicas') : 0}
                request = {'0': (self.path, metadata, [PROTOCOL])}
                response = self.manager.putFile(request)
                success, turl, protocol = response['0']
                debug_msg('__init__',response['0'])
            else:
                debug_msg('success', success)
                self.creating = False
                # read method needs read/write mode
                self.file = file(self.tmp_path,'wb+')
                self.fd = self.file.fileno()
            self.turl = turl
            self.success = success
            self.direct_io = True
            debug_msg('left ARCFSFile.__init__',success)


        def read(self, size, offset):
            """
            read file
            gets file from manager. If file has no valid replica,
            we'll ask manager again till replica is ready
            read will only read file from ByteIOClient on first block,
            then write to local file, which used for the rest of the 
            life of this ARCFSFile object
            """
            debug_msg('read called:', (size, offset))
            
            if self.file.tell() > 0:
                # file is already read
                self.file.seek(offset)
                debug_msg('read left fancy:', (size, offset))
                return self.file.read(size)
                
            success = self.success
            turl = self.turl
            request = {'0' : (self.path, [PROTOCOL])}
            while success == 'file has no valid replica':
                # try again
                time.sleep(0.1)
                success, turl, _ = self.manager.getFile(request)['0']
            self.turl = turl
            self.success = success
            if success == 'is not a file':
                return -EISDIR
            if success != 'done':
                return -ENOENT
            rbuf = ByteIOClient(turl).read()
            self.file.write(rbuf)
            self.file.seek(offset+size)
            slen = len(rbuf)
            if offset < slen:
                if offset + size > slen:
                    size = slen - offset
                buf = rbuf[offset:offset+size]
            else:
                buf = ''
            debug_msg('read left:', (size, offset))
            return buf


        def write(self, buf, offset):
            """
            write to self.file
            note that write to ByteIOClient is done only on call to release
            """
            debug_msg('write called:', (len(buf), offset))
            self.file.seek(offset)
            self.file.write(buf)
            debug_msg('write left:', (len(buf), offset))
            return len(buf)


        def release(self, flags):
            """
            release file and write to ByteIOClient
            """
            debug_msg('release called:', (flags))
            size = self.file.tell()
            self.file.close()
            if self.creating:
                f = file(self.tmp_path,'rb')
                checksum = create_checksum(f, 'md5')
                f.close()
                # set size and checksum now that we have it
                request = {'size':[self.path, 'set', 'states', 'size', size],
                           'checksum':[self.path, 'set', 'states', 'checksum', checksum],
                           'replicas':[self.path, 'set', 'states', 'neededReplicas', needed_replicas]}
                modify_success = self.manager.modify(request)
                if modify_success['size'] == 'set' and \
                   modify_success['replicas'] == 'set' and \
                   modify_success['checksum'] == 'set':
                    f = file(self.tmp_path,'rb')
                    parent = os.path.dirname(self.path)
                    child = os.path.basename(self.path)
                    GUID = self.manager.list({'0':parent})['0'][0][child][0]
                    response = self.manager.addReplica({'release': GUID}, [PROTOCOL])
                    success, turl, protocol = response['release']
                    if success == 'done':
                        ByteIOClient(turl).write(f)
                f.close()
            os.remove(self.tmp_path)
            debug_msg('release left:', (flags))


        # stolen from fuse-python xmp.py
        def _fflush(self):
            debug_msg('_fflush called')
            if 'w' in self.file.mode or 'a' in self.file.mode:
                self.file.flush()
            debug_msg('_fflush left')


        # stolen from fuse-python xmp.py
        def fsync(self, isfsyncfile):
            debug_msg('fsync called')
            self._fflush()
            if isfsyncfile and hasattr(os, 'fdatasync'):
                os.fdatasync(self.fd)
            else:
                os.fsync(self.fd)
            debug_msg('fsync left')


        # stolen from fuse-python xmp.py
        def flush(self):
            debug_msg('flush called')
            self._fflush()
            # cf. xmp_flush() in fusexmp_fh.c
            os.close(os.dup(self.fd))
            debug_msg('flush left')


        def fakeinod(self):
            """
            Function to fake inode before write is released
            """
            debug_msg('fakeinod called')
            mode = sym2oct('-rw-r--r--')
            nlink = 1
            uid = os.getuid()
            gid = os.getgid() 
            size = 0
            mtime = atime = ctime = time.time()
            metadata = {}
            
            debug_msg('leaving fakeinod')
            return ARCInode(mode, nlink, uid, gid, size, mtime,   
                            metadata, atime=atime, ctime=ctime)

        def mkfinod(self):
            """
            Function to get metadata from Manager and parse it to ARCInode
            Returns ARCInode
            """
            debug_msg('mkfinod called')
            request = {'0':self.path}
            metadata = self.manager.stat(request)['0']
            if not isinstance(metadata,dict):
                debug_msg('mkfinod left, no metadata')
                return None
            if not metadata[('catalog', 'type')] == 'file':
                debug_msg('mkfinod left, not a file')
                return None
            nlink = 1
            size = long(metadata[('states','size')])
            mode = sym2oct('-rw-r--r--')
            closed = 0

            if metadata.has_key(('timestamps', 'created')):
                ctime = float(metadata[('timestamps', 'created')])
            else:
                ctime = 0
            if metadata.has_key(('timestamps', 'modified')):
                mtime = float(metadata[('timestamps', 'modified')])
            else:
                mtime = ctime
            atime = mtime
            # no way to get uid and gid in ARC storage (yet) so we use users uid
            uid = os.getuid()
            gid = os.getgid()

            debug_msg('mkfinod left')
            return ARCInode(mode, nlink, uid, gid, size, mtime, metadata, 
                            atime, ctime)

        def fgetinode(self):
            """
            Functio to get file inode. If self.creating is true
            manager does not know about this file yet ('cause it's not
            written to system yet), so we return a fake inode instead
            """
            debug_msg('fgetinode called')
            if self.creating:
                return self.fakeinod()
            try:
                return self.mkfinod()
            except:
                return None
            

        def fgetattr(self):
            """
            get file attributes
            """
            debug_msg('fgetattr called')
            inod = self.fgetinode()
            if inod:
                return inod.st
            else: 
                return -ENOENT


    def main(self, *a, **kw):

        self.file_class = self.ARCFSFile

        return Fuse.main(self, *a, **kw)


def main():
    usage = """
    Userspace mount of ARC Storage.
    
    """ + Fuse.fusage

    server = ARCFS(version="%prog " + fuse.__version__,
                  usage=usage,
                  dash_s_do='setsingle')

    server.parser.add_option(mountopt="root", metavar="PATH",
                             default=MOUNT,
                             help="mirror arc filesystem from under PATH [default: %default]")

    server.parse(values=server, errex=1)
    server.main()

if __name__ == '__main__':
    main()



debugfile.close()
