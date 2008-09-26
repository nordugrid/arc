import arc
import copy, os, base64
import cPickle as pickle

from storage.store.basestore import BaseStore

class PickleStore(BaseStore):
    """ Class for storing object in a serialized python format. """

    def __init__(self, storecfg, non_existent_object = {}, log = None):
        """ Constructor of PickleStore.

        PickleStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        BaseStore.__init__(self, storecfg, non_existent_object, log)
        self.log.msg(arc.DEBUG, "PickleStore constructor called")
        self.log.msg(arc.DEBUG, "datadir:", self.datadir)

    def _filename(self, ID):
        """ Creates a filename from an ID.

        _filename(ID)

        'ID' is the ID of the given object.
        The filename will be 
        datadir/<two first letters of base64 encoded form of the ID>/<base64 encoded form of the ID>.
        """
        name = base64.b64encode(ID)
        return os.path.join(self.datadir, name[:2], name)

    def _tmpfilename(self, ID):
        """ Creates a tmpfilename from an ID.

        _tmpfilename(ID)

        'ID' is the ID of the given object.
        The filename will be datadir/<base64 encoded form of the ID>.
        The corresponding file to _tmpfilename should always be moved to _filename(ID)
        """
        name = base64.b64encode(ID)
        return os.path.join(self.datadir, name)

    def _list(self):
        """ List all the existing files.
        
        _list()
        
        Returns a list of filenames.
        """
        names = []
        # list the contects of each subdirectory withtin the data directory
        for subdir in os.listdir(self.datadir):
            if os.path.isdir(os.path.join(self.datadir,subdir)):
                names.extend(os.listdir(os.path.join(self.datadir, subdir)))
        return names

    def list(self):
        """ List the IDs of the existing entries.
        
        list()
        
        Returns a list of IDs.
        """
        IDs = []
        # get all the filenames
        for name in self._list():
            try: # decode the filename
                ID = base64.b64decode(name)
                IDs.append(ID)
            except:
                self.log.msg()
        return IDs

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            # generates a filename from the ID
            # then use pickle to load the previously serialized data
            return pickle.load(file(self._filename(ID), 'rb'))
        except IOError:
            # don't print 'file not found' if there is no such ID
            pass
        except EOFError:
            # TODO: find out what causes this problem
            pass
        except:
            # print whatever exception happened
            self.log.msg()
            self.log.msg(arc.ERROR, "filename:", self._filename(ID))
        # if there was an exception, return the given non_existent_object
        return copy.deepcopy(self.non_existent_object)

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
                pickle.dump(object, file(tmp_fn,'wb'))
                # try to rename the file
                try:
                    os.rename(tmp_fn,fn)
                except:
                    # try to create parent dir first, then rename the file
                    os.mkdir(os.path.dirname(fn))
                    os.rename(tmp_fn,fn)
            elif os.path.isfile(fn):
                # object empty, file is not needed anymore
                os.remove(fn)
        except:
            self.log.msg()
