from ahashsetup import *

class AHashTester:

    def __init__(self, AHashURL, ssl_config):
        self.master_ahash = arc.URL(AHashURL)
        self.ahash = AHashClient(AHashURL, False, ssl_config)
        self._update_ahash_urls()

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
        
    def ahash_get(self, IDs, neededMetadata = []):
        """
        Wrapper for ahash.get updating ahashes and calling ahash.get
        """
        self._update_ahash_urls()
        ahash_urls = self.ahash.urls
        print [host.Host() for host in ahash_urls]
        ret = None
        try:
            ret = self.ahash.get(IDs)
            return ret
        except:
            # retry, updating ahash urls in case they are outdated
            ahash_urls = self.ahash.urls
            try:
                ret = self.ahash.get(IDs)
                return ret
            except:
                # make sure ahash.urls are restored even is change failed
                self.ahash.urls = ahash_urls
                raise

    def ahash_change(self, changes):
        """
        Wrapper for ahash.change replacing ahash.urls with master ahash, calling
        ahash.change, putting back ahash.urls
        ahash.change can only call master ahash
        """
        ahash_urls = self.ahash.urls
        self.ahash.urls = [self.master_ahash]
        ret = None
        try:
            ret = self.ahash.change(changes)
            self.ahash.urls = ahash_urls
            return ret
        except:
            # put back all known ahashes so we don't only ask the master
            # for an update
            self.ahash.urls = ahash_urls
            # retry, updating ahash urls in case master is outdated
            self._update_ahash_urls()
            ahash_urls = self.ahash.urls
            self.ahash.urls = [self.master_ahash]
            try:
                ret = self.ahash.change(changes)
                self.ahash.urls = ahash_urls
                return ret
            except:
                # make sure ahash.urls are restored even is change failed
                self.ahash.urls = ahash_urls
                raise

tester = AHashTester(AHashURL, ssl_config)
print tester.ahash_change({'entry':('entry','set','section','property','42',{})})
request_num=0
resp = None
start_time = time.time()
while time.time() < start_time+(150*100):
    f=open("ahashtest_started_%1.15g.log"%start_time,'a')
    request_num += 1
    get_start = time.time()
    try:
        resp = tester.ahash_get(['entry'])
    except:
        pass
    get_stop = time.time()
    if resp == {'entry': {('section', 'property'): '42'}}:
        f.write("%d %1.15g %1.15g\n"%(request_num, get_stop, get_stop-get_start))
        print request_num, get_stop, get_stop-get_start
    else:
        f.write("%d %1.15g request failed\n"%(request_num, get_stop))
        print request_num, get_stop, "request failed"
    resp = None
    f.close()
    # wait for 2 seconds
    time.sleep(random.random()*4)
