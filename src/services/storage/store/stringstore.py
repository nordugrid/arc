import arc
import copy, os, base64

from storage.store.picklestore import PickleStore

from arcom.logger import Logger
log = Logger(arc.Logger(arc.Logger_getRootLogger(), 'Storage.StringStore'))

class StringStore(PickleStore):
    """ Class for storing string objects. """

    def __init__(self, storecfg, non_existent_object = {}):
        """ Constructor of PickleStore.

        PickleStore(storecfg)

        'storecfg' is an XMLNode with a 'DataDir'
        'non_existent_object' will be returned if an object not found
        """
        PickleStore.__init__(self, storecfg, non_existent_object)
        log.msg(arc.DEBUG, "StringStore constructor called")
        log.msg(arc.DEBUG, "datadir:", self.datadir)

    def get(self, ID):
        """ Returns the object with the given ID.

        get(ID)

        'ID' is the ID of the requested object.
        If there is no object with this ID, returns the given non_existent_object value.
        """
        try:
            # generates a filename from the ID
            # then use pickle to load the previously serialized data
            return eval(file(self._filename(ID),'rb').read())
        except IOError:
            # don't print 'file not found' if there is no such ID
            pass
        except EOFError:
            # TODO: find out what causes this problem
            pass
        except:
            # print whatever exception happened
            log.msg()
            log.msg(arc.ERROR, "filename:", self._filename(ID))
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
            elif os.path.isfile(fn):
                # object empty, file is not needed anymore
                os.remove(fn)
        except:
            log.msg()
