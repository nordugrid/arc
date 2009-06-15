import os, random, copy
from ahashsetup import *

def _usage():
    print "Usage: ./ahashkiller.py target_type"
    print "target_type can be one of 'master', 'client' or 'any'"

class AHashKiller:
    """
    Class for randomly restart remote A-Hash services; either master,
    client or any service
    Requires password free ssh login on remote servers
    """

    def __init__(self, AHashURL, ssl_config, target_type="client"):
        self.master_ahash = arc.URL(AHashURL)
        self.ahash = AHashClient(AHashURL, False, ssl_config)
        self.target_type = target_type

    def _update_ahash_urls(self):
        try:
            ahash_list = self.ahash.get(ahash_list_guid)[ahash_list_guid]
            if ahash_list:
                master = [url for type,url in ahash_list.items() 
                          if 'master' in type and type[1].endswith('online')]
                clients = [url for type,url in ahash_list.items() 
                           if 'client' in type and type[1].endswith('online')]
                self.master_ahash = arc.URL(master[0])
                self.ahash.urls = [arc.URL(url) for url in clients]
        except:
            pass

    def _call_remote(self, task):
        cmd = "ssh %s usr/etc/init.d/ahash "+task
        self._update_ahash_urls()
        print "master:", self.master_ahash.Host(), "target_type", self.target_type
        if self.target_type == "client":
            ahash_urls = [url for url in self.ahash.urls if url.Host() != self.master_ahash.Host()]
            # need to remove master twice since it's a client as well
        elif self.target_type == "master":
            ahash_urls = [self.master_ahash]
        elif self.target_type == "any":
            if len(self.ahash.urls)>1:
                ahash_urls = self.ahash.urls[1:]
            else:
                ahash_urls = self.ahash.urls
        else:
            _usage()
            raise Exception, "Wrong target_type"
        print [host.Host() for host in ahash_urls]
        random.shuffle(ahash_urls)
        f = open("ahashkiller_%s.log"%self.target_type,'a')
        try:
            host = ahash_urls[0].Host()
            f.write("%1.15g, %s ahash on %s\n"%(time.time(), task, host))
            print time.time(), task, "ahash on ", host
            os.system(cmd%host)
            f.write("%1.15g, %s ahash on %s\n"%(time.time(), task+"ed", host))
            print time.time(), task+'ed', "ahash on ", host
        except IndexError:
            print "No target available"
        except:
            f.close()
            raise Exception, cmd%host+" failed"
        f.close()
        
    def restart(self):
        self._call_remote("restart")

try:
    target_type = sys.argv[1]
except:
    _usage()
    sys.exit(1)
    
killer = AHashKiller(AHashURL, ssl_config, target_type)

for i in range(100):
    killer.restart()
    time.sleep(150)
