from EncryptUtils import from_file
from SecureStompMessenger import message_hash
import glob
import os
import tempfile
import logging

################################################################################
#
# Message Database implementation, using the filesystem as the database
#
# An instance of this object is used by the SecureStompMessenger object to
# allow it to manage incoming and outgoing messages, and track acknowledgments
#
# If you want to create other implementations (e.g. using MySQL as a store) you
# need to implement the public functions in your own derived class.
#

class MessageDB:

    def __init__(self, basepath, test):
        
        self._test = test
        self._basepath=basepath
        self._outpath=basepath+'/outgoing/'
        self._inpath=basepath+'/incoming/'
        self._ackpath=basepath+'/ack/'
        self._rejectpath=basepath+'/reject/'

        # TODO: some checks/creating paths etc...
        #       clear the ack store
        # ...
        self._check_create(basepath)
        self._check_create(self._inpath)
        self._check_create(self._outpath)
        self._check_create(self._ackpath)
        self._check_create(self._rejectpath)
        
        # in test mode, incoming messages are retained
        if (self._test):
            self._acceptpath=basepath+'/accept/'
            self._check_create(self._acceptpath)


    # Create a new outgoing message

    def new_outgoing_message(self, msg_id, msg_data):
        msg_file = self._outpath+msg_id
        self._atomic_write_file(msg_file, msg_data)


    # Return the (id,message) of the first message in the message queue, or
    # (None, None) if there are no messsages. Call 'clear_outgoing_message' to
    # remove the message after it has been dealt with

    def get_outgoing_message(self):
        return self._get_message(self._outpath) 


    # Call this function after the message has been sent, to remove it from the
    # outgoing message queue

    def clear_outgoing_message(self, id):
        os.remove(self._outpath+id)


    # Call this function when a message has been received, to store the message
    # in the incoming message store. The message will be noted in the ack store
    # so we'll expect a future call to clear_message_ack with the hash of the
    # message. Returns True if the message is 'new' or False if the message has
    # been seen already

    def new_incoming_message(self, msg_id, producer_dn, msg_data):
        msg_file=self._inpath+msg_id
        if self._dup_check(msg_data, msg_file+'\t'+producer_dn+'\n'):

            # write out the signer DN to the signature file

            sigfile=open(msg_file+'.sig','w')
            sigfile.write(producer_dn+'\n')
            sigfile.close()

            self._atomic_write_file(msg_file, msg_data)

            return True
        else:
            return False


    # Return the (id,message,signer) of the first message in the incoming message
    # queue, or (None, None, None) if there are no messages. After processing,
    # call clear_incoming_message with the id to remove the message

    def get_incoming_message(self):
        (id,message) = self._get_message(self._inpath)
        if id != None:
            try:
                signer = from_file(self._inpath+id+'.sig')
            except:
                # most likely if the .sig file isn't present
                signer = None
        else:
            signer = None

        return (id, message, signer)


    # Call this function after the message has been dealt with and can be
    # removed from the incoming messages

    def clear_incoming_message(self, id):
        
        # in production mode, delete the messages
        if not self._test:
            try:
                os.remove(self._inpath+id)
            except: # file not found is unexpected
                raise
            try:
                os.remove(self._inpath+id+'.sig')
            except: # .sig file not found is possible
                log.warn("Error when trying to remove .sig file. " +
                        "Perhaps it didn't exist.")
        # in test mode, move messages to the accept directory
        else:
            try:
                msg=from_file(self._inpath+id)
                self._atomic_write_file(self._acceptpath+id, msg)
                os.remove(self._inpath+id)
            except: # file not found is unexpected
                raise
            try:
                signer=from_file(self._inpath+id+'.sig')
                self._atomic_write_file(self._acceptpath+id+'.sig', signer)
                os.remove(self._inpath+id+'.sig')
            except: # .sig file not found is possible
                log.warn("Error when trying to move .sig file. " +
                        "Perhaps it didn't exist.")


    # Call this when a message that has previously been acked has been re-acked
    # by the producer i.e. producer is acking our ack

    def clear_message_ack(self, msg_hash):
        ackfile=self._ackpath+msg_hash
        if os.path.exists(ackfile):
            os.remove(ackfile)


    # Reject the specified message from the incoming queue
    def reject_message(self, id, reason):
        try:
            msg=from_file(self._inpath+id)
            
            # write the rejected file to the rejected directory
            self._atomic_write_file(self._rejectpath+id, msg)
            # write the reason for rejection to another file, id.err
            self._atomic_write_file(self._rejectpath+id+'.err', reason)
            # delete the message
            os.remove(self._inpath+id)
        except:
            raise
        try:
            signer=from_file(self._inpath+id+'.sig')
            # write the signer to the rejected directory
            self._atomic_write_file(self._rejectpath+id+'.sig', signer)
            # delete the .sig file
            os.remove(self._inpath+id+'.sig')
        except: # sig file not found is possible
            log.warn("Error when trying to remove .sig file. " +
                        "Perhaps it didn't exist.")
            

    ########################################################################## 
    # Private functions from here onwards...
    
    
    # create directory if it doesn't exist
    def _check_create(self,path):
        if not os.access(path,os.F_OK):
            os.mkdir(path)

    # Check for and read the next message from the specified location

    def _get_message(self, path):
        msgs=glob.glob(path+'*')
        if len(msgs) == 0:
            return (None, None)
        
        # Ensure the file isn't being written to by another process.
        #### (Taken out for now, can't figure a sensible way to do this)
        #if not file_is_closed(msgs[0]):
            #return (None, None)

        msgs.sort()
        msg_id = os.path.basename(msgs[0]) 
        data=from_file(msgs[0])
        return (msg_id, data)


    # Checks if the message has already been received before, if not we make a
    # note of it in the ack store. Returns True if the message is not a
    # duplicate, False if it is

    def _dup_check(self, msg, msg_extra):
        ackfile = self._ackpath+message_hash(msg)
        if not os.path.exists(ackfile):
            md5file=open(ackfile,'w')
            md5file.write(msg_extra)
            md5file.close()
            return True
        else:
            return False


    # Atomically write a file; that is write it to a temp file prefixed by
    # '.tmp' in the target directory, and then rename the file; to a process
    # watching the directory, the file will 'appear' instantaneously

    def _atomic_write_file(self, filename, data):
        filepath=os.path.dirname(filename)
        tmpfd,tmpname = tempfile.mkstemp(prefix='.tmp', dir=filepath)
        os.write(tmpfd,data)
        os.close(tmpfd)
        os.rename(tmpname,filename)
        
log = logging.getLogger('SSM')
        