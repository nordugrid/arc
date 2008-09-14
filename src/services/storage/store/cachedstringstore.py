import copy, os, base64

from storage.store.cachedpicklestore import CachedPickleStore

class CachedStringStore(CachedPickleStore):
    """ Class for storing object in a serialized python format. """

    def __init__(self, storecfg, non_existent_object = {}, log = None):
        """ Constructor of CachedStringStore.

        StringStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        CachedPickleStore.__init__(self, storecfg, non_existent_object, log)
        self.log('DEBUG', "CachedStringStore constructor called")
        self.log('DEBUG', "datadir:", self.datadir)
        self.store = {}
        self._load_storage()

    def _load_storage(self):
        filelist = self._list()
        for f in filelist:
            self.store[base64.b64decode(f)] = eval(file(os.path.join(self.datadir,f[:2],f), 'rb').read())

    def set(self, ID, object):
        """ Stores an object with the given ID..

        set(ID, object)

        'ID' is the ID of the object
        'object' is the object itself
        If there is already an object with this ID it will be overwritten completely.
        """
        if not ID:
            raise Exception, 'ID is empty'
        try:
            # generates a filename from the ID
            fn = self._filename(ID)
            tmp_fn = self._tmpfilename(ID)
            # if 'object' is empty, don't make file
            if object:
                # serialize the given list into tmp_fn
                f = file(tmp_fn,'wb')
                f.write(str(object))
                f.close()
                # try to rename the file
                try:
                    os.rename(tmp_fn,fn)
                except:
                    # try to create parent dir first, then rename the file
                    os.mkdir(os.path.dirname(fn))
                    os.rename(tmp_fn,fn)
                self.store[ID] = object
            elif os.path.isfile(fn):
                # object empty, file is not needed anymore
                os.remove(fn)
                del(self.store[ID])
        except:
            self.log()
