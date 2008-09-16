import copy, os, base64
import cPickle as pickle

from storage.store.picklestore import PickleStore


class CachedPickleStore(PickleStore):
    """ Class for storing object in a serialized python format. """

    def __init__(self, storecfg, non_existent_object = {}, log = None):
        """ Constructor of CachedPickleStore.

        CachedPickleStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        PickleStore.__init__(self, storecfg, non_existent_object, log)
        self.log('DEBUG', "PickleStore with datadir '%s' is a CachedPickleStore" % self.datadir)
        self.store = {}
        self._load_storage()

    def _load_storage(self):
        filelist = self._list()
        for f in filelist:
            self.store[base64.b64decode(f)] = pickle.load(file(os.path.join(self.datadir,f[:2],f), 'rb'))
            
    def list(self):
        """ List the IDs of the existing entries.
        
        list()
        
        Returns a list of IDs.
        """
        return self.store.keys()
            
    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            # return a copy of the in-memory stored object
            return copy.deepcopy(self.store[ID])
        except KeyError:
            # don't print 'KeyError' if there is no such ID
            pass
        except:
            # print whatever exception happened
            self.log()
            self.log("ERROR", "ID", ID)
        # if there was an exception, return the given non_existent_object
        return copy.deepcopy(self.non_existent_object)

    def set(self, ID, obj):
        """ Stores an object with the given ID..

        set(ID, obj)

        'ID' is the ID of the object
        'obj' is the object itself
        If there is already an object with this ID it will be overwritten completely.
        """
        if not ID:
            raise Exception, 'ID is empty'
        try:
            # generates a filename from the ID
            fn = self._filename(ID)
            tmp_fn = self._tmpfilename(ID)
            # if 'obj' is empty, don't make file
            if obj:
                # TODO: what if something happens between the disk write and the in-memory store update, and only the disk gets updated?
                # serialize the given list into tmp_fn
                pickle.dump(obj, file(tmp_fn,'wb'))
                # try to rename the file
                try:
                    os.rename(tmp_fn,fn)
                except:
                    # try to create parent dir first, then rename the file
                    os.mkdir(os.path.dirname(fn))
                    os.rename(tmp_fn,fn)
                # store a copy of the object
                self.store[ID] = copy.deepcopy(obj)
            elif os.path.isfile(fn):
                # object empty, file is not needed anymore
                os.remove(fn)
                del(self.store[ID])
        except:
            self.log()
