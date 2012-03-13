###############################################################################
#   Copyright [2012] [gLite - Lightweight Middleware for Grid Computing]      #
#                                                                             #
#   Licensed under the Apache License, Version 2.0 (the "License");           #
#   you may not use this file except in compliance with the License.          #
#   You may obtain a copy of the License at                                   #
#                                                                             #
#       http://www.apache.org/licenses/LICENSE-2.0                            #
#                                                                             #
#   Unless required by applicable law or agreed to in writing, software       #
#   distributed under the License is distributed on an "AS IS" BASIS,         #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  #
#   See the License for the specific language governing permissions and       #
#   limitations under the License.                                            #
###############################################################################

from EncryptUtils import encrypt_message, decrypt_message, verify_message, \
    sign_message, verify_certificate, from_file, get_certificate_subject, \
    check_cert_key
from threading import Timer, Lock
import base64
import logging
import md5
import os
import stomp
import sys
import time
import zlib



################################################################################
# Constants that are unlikely to change

ACK_WAIT_TIME = 60
PING_TIMER = 120

NO_MSG_ID = 'noid'
MSG_REJECT = 'REJECTED'

SSM_ID_HEADER = 'ssm-msg-id'
SSM_REACK_HEADER = 'ssm-reack'

SSM_MSG_TYPE = 'ssm-msg-type'
 
SSM_NORMAL_MSG = 'msg'
SSM_ACK_MSG = 'ack'
SSM_PING_MSG = 'ping'
SSM_CERT_REQ_MSG = 'certreq'
SSM_CERT_RESP_MSG = 'certresp'

# End of constants
################################################################################

SSMLoggerID = 'SSM'

################################################################################
# Dummy classes used for configuration data

class Config:
    pass

class ProducerConfig:
    pass

class ConsumerConfig:
    pass

# 
################################################################################


################################################################################
#
# Secure Stomp Messenger class, implementing a *reliable* message transfer
# function over Stomp protocol. Listens to a nominated topic/queue for incoming
# messages, which are signed and encrypted. Messages are handled by a MessageDB
# object (see above for a basic model that uses the filesystem)
#
# Also implements an acknowledgment mechanism to ensure that message delivery
# has occurred at the consuming end, and a Re-Acknowledgment to allow a
# consumer to verify that an ack has been recieved by the producer, so we can
# stop worrying about duplicated messages arriving.

#

class SecureStompMessenger(object):
    def __init__(self, messagedb, configuration, producer_config, consumer_config):
        self._is_connected = False  # connection tracking
        self._received = 0          # message counter
        self._connection = None     # the connection object

        # this is the identifier of the last message that we ack'd
        self._last_acked = None

        # this is the md5 sum of the previous message that was ack'd, and also
        # the re-ack value we need to send as part of our next message.
        self._reack = None

        # a string representing the PEM encoded certificate of the consumer, we
        # use this certificate during the encryption process
        self._consumer_certificate = None
        
        # A list of the valid DNs we have available.  It should be used only 
        # when the lock below is acquired.
        self._valid_DNs = []
        
        # A Lock object so that the _valid_DNs List is never used by two threads 
        # at the same time.
        self._valid_DN_lock = Lock()
        # Timer objects running in different threads to do certain tasks every 
        # so often without interrupting control flow.
        self._ping_timer = None
        self._clear_cert_timer = None
        self._read_dn_timer = None

        if consumer_config == None and producer_config == None:
            raise Exception, 'No producer or consumer configuration supplied'

        self._messagedb       = messagedb
        self._configuration   = configuration
        self._producer_config = producer_config
        self._consumer_config = consumer_config

        # The SSM cannot work without a valid and matching cert and key.
        try:
            cert = from_file(configuration.certificate)
            key = from_file(configuration.key)
            if not check_cert_key(cert, key):
                raise Exception("Certificate and key don't match.")
        except Exception, e:
            log.error('Error retrieving cert or key file: ' + str(e))
            raise
         


    def startup(self):
        
        cfg = self._configuration
        self._connection = stomp.Connection([(cfg.host, cfg.port)], 
                                            reconnect_sleep_initial = 5)
        
        # You can set this in the constructor but only for stomppy version 3.
        # This works for stomppy 3 but doesn't break stomppy 2.
        self._connection.__reconnect_attempts_max = 3
        
        self._connection.set_listener('SecureStompMessenger', self)
        
        try:
            self._connection.start()
            self._connection.connect(wait = False)
        except Exception:
            error = "Couldn't connect to " + cfg.host + ":" +  str(cfg.port) + "."
            raise Exception(error)
    
        if self._consumer_config != None:
            topic = self._consumer_config.listen_to
            self._connection.subscribe(destination=topic, ack='auto')
            log.debug('Subscribing as a consumer to: '+topic)
            
            # Get the interval, then start the repeating check for valid DNs.
            self._read_dn_interval = self._consumer_config.read_valid_dn_interval
            self._read_valid_dn(self._consumer_config.valid_dn)

        if self._producer_config != None:
            cfg = self._producer_config
            self._connection.subscribe(destination=cfg.ack_queue, ack='auto')
            log.debug('I will be a producer, my ack queue is: '+cfg.ack_queue)

            # Get the interval, then start the repeating loop for clearing the cert.
            self._clear_cert_interval = self._producer_config.clear_cert_interval
# Never return the SSM if uncommented this line
#            self._reset_certificate()
            
        self._last_acked = None
        self._reack = None
        
        # arm the ping-timer
# Never return the SSM if uncommented this line
#        self._send_ping() 

        try:
            while not self._is_connected:
                time.sleep(0.1)
        except KeyboardInterrupt:
            log.error('Interrupted while waiting for connect')
            sys.exit(1) 

    def stop(self):
        log.info('Disconnected')
        #self._connection.unsubscribe(destination=self._consumer_config.listen_to, id='auto')
        self._connection.remove_listener('SecureStompMessenger')
        self._connection.stop()
        #self._connection.disconnect()
        self._is_connected = False
        self._connection = None
        del self._messagedb

    def on_connecting(self, host_and_port):
        log.info('Connecting: '+str(host_and_port))


    def on_connected(self, headers, body):
        log.info('Connected')
        self._is_connected = True
    
    
    def on_disconnected(self,headers,body):
        log.info('Disconnected, attempting reconnection')
        self._is_connected = False
        self._connection = None
        self.startup()


    def on_send(self, headers, body):
        pass

    # Process an incoming message. The message may contain extra header
    # information causing us to handle it differently. The types of message we
    # handle are normal messages or acknowledgements. Normal messages may
    # contain a re-acknowledgement value, to let us know the producer got our
    # previous ack.

    def on_message(self, headers, body):
        log.debug('Receiving message from: '+headers['destination'])
        self._received = (self._received + 1) % 1000

        if SSM_REACK_HEADER in headers.keys():

            # we got a re-ack from the producer, so we can safely remove the
            # ack-tracking information for the message

            log.debug('Got reack '+headers[SSM_REACK_HEADER])
            self._messagedb.clear_message_ack(headers[SSM_REACK_HEADER])

        # don't need to do anything with a ping
        if headers[SSM_MSG_TYPE] == SSM_PING_MSG:
            return

        # handle certificate request by responding with our certificate
        if headers[SSM_MSG_TYPE] == SSM_CERT_REQ_MSG:
            log.debug('Certificate requested')
            self._send_message(headers['reply-to'], SSM_CERT_RESP_MSG, 
                               NO_MSG_ID, from_file(self._configuration.certificate))
            
            return
            
        # handle certificate response
        if headers[SSM_MSG_TYPE] == SSM_CERT_RESP_MSG:
            if self._consumer_certificate == None:
                log.info('Certificate received')
                
                try:
                    check_crls = self._configuration.check_crls
                    capath = self._configuration.capath
                    if not verify_certificate(body, capath, check_crls):
                        raise Exception, 'Certificate failed to verify'

                    if self._producer_config.consumerDN != get_certificate_subject(body):
                        log.error('Expected ' + self._producer_config.consumerDN + 
                                  ', but got ' + get_certificate_subject(body))
                        
                        raise Exception, 'Certificate does not match consumerDN configuration'

                    log.debug(get_certificate_subject(body))
                    self._consumer_certificate = body
                except  Exception, err:
                    log.warning('Certificate not verified: ' + str(err))

            else:
                log.warning('Unexpected certificate - ignored')

            return

        if headers['destination'] == self._producer_config.ack_queue:

            # message is an ack to our previous message; the message ID will be
            # the md5sum of our previously sent message

            log.debug('Received ack for '+headers[SSM_ID_HEADER])
            self._last_acked = headers[SSM_ID_HEADER]
            return

        # call the handler for this message, based on where it was sent
        #HANDLERS.get(headers['destination'], default_handler)(self, headers, body, self._received)
        self._decrypt_verify(headers, body)


    def on_error(self, headers, body):
        log.warning('Error frame')


    def on_receipt(self, headers, body):
        self._received = self._received + 1
        log.debug('Broker received '+headers['receipt-id'])


    # Pick the first message from the outgoing message queue and send it,
    # waiting for acknowledgement before returning. Raises an exception if the
    # message is not acked.
    #
    # Returns True if a message was sent, or False if no message was sent or if
    # there are no messages to send.
    
    def process_outgoing(self):

        if not self._is_connected:
            return False

        (msg_id, msg_data)= self._messagedb.get_outgoing_message()
        if msg_id == None:
            return False

        # Ensure we've got a valid certificate to encode with

        if self._get_certificate() == None:
            log.info('No certificate, requesting')
            self._request_certificate()

        m5 = message_hash(msg_data)

        log.debug('Hash: '+m5)
        # we also need to base64 encode the message, because OpenSSL SMIME changes
        # CR to CR/LFs in the messages which I can't figure out how to stop!
        # Base64 ensures that we get the same message at the other side, and works
        # for binary too.

        log.debug('Raw length: ' + str(len(msg_data)))
        msg64 = base64.encodestring(zlib.compress(msg_data))
        log.debug('Encoded length: '+str(len(msg64)))

        # send the signed/encrypted message to the topic

        cert = self._configuration.certificate
        key = self._configuration.key
        log.debug('Signing')
        signed = sign_message(msg64, cert, key)
        log.debug('Encrypting signed message of length ' + str(len(signed)))
        
        # Here, Kevin had compressed it again as well as encrypting.  
        # This meant that the content of the message sent was not ascii, 
        # and this was the cause of the conflict in versions of stomppy.
        # If we leave it as encrypted, the message will be ascii-only.
        encrypted = encrypt_message(signed, self._get_certificate())
        log.debug('Encrypted length: '+str(len(encrypted)))

        if self._send_message(self._producer_config.send_to, SSM_NORMAL_MSG, msg_id, encrypted):

            # We're now waiting for an acknowledgement; this arrives over
            # our acknowledgement topic/queue

            counter=ACK_WAIT_TIME / 0.2
            while counter and self._waiting_for_ack(m5):
                time.sleep(0.2)
                counter -= 1
            
            # Rejection logging happens only after the count has run down.
            # This is possibly not ideal.

            if self._last_acked == MSG_REJECT:
                log.error('Message rejected by consumer')
                raise Exception, 'Message ' + msg_id + ' was rejected by consumer'

            if counter == 0:
                raise Exception, 'No acknowledgement received for message ' + msg_id


            self._messagedb.clear_outgoing_message(msg_id)

            log.debug('Message ' + msg_id + ' acknowledged by consumer')
            self._reack = m5

            return True

        return False


    # Send msg to the destination topic/queue, using the msg_id value as the
    # message identifier. You may use SSM_PING_MSG for this value if an empty
    # (ping) message is to be sent.

    def _send_message(self, destination, type, msg_id, msg):
        if not self._is_connected:
            log.error('Failed to send message ID %s: not connected' % msg_id)
            return False

        log.debug('Sending '+msg_id)

        # standard headers, which we'll add to as necessary...
        hdrs={ SSM_MSG_TYPE : type, 'destination' : destination, 'receipt' : msg_id, \
                SSM_ID_HEADER : msg_id, 'reply-to' : self._producer_config.ack_queue }

        if self._reack != None:

            # send out our re-ack to tell the consumer that we've seen his ack
            # to our previous message, so the consumer can stop tracking the
            # fact it received the previous message

            hdrs[SSM_REACK_HEADER] = self._reack
            log.debug('Sending reack '+self._reack)
            self._reack = None

        self._connection.send(msg, headers=hdrs)
        
        # only track acks for genuine messages

        if msg_id != SSM_PING_MSG:
            self._last_acked = None

        return True


    # Send acknowledgment of the msg_id to the destination topic/queue.
    # If the message is rejected, MSG_REJECT can be sent instead of the hash.

    def _send_ack(self, destination, msg_hash):
        if self._is_connected:
            log.debug('Sending ack for '+msg_hash)
            self._connection.send('', headers= {\
                'destination' : destination, \
                'receipt' : msg_hash, \
                SSM_MSG_TYPE : SSM_ACK_MSG,\
                SSM_ID_HEADER : msg_hash})


    def _waiting_for_ack(self, msg_id):
        return (self._last_acked != msg_id)


    def _get_certificate(self):
        return self._consumer_certificate

    
    def _clear_certificate(self):
        self._consumer_certificate = None
        
    ##########################################################################
    # Set the consumer certificate to None, then call this method again after 
    # the number of seconds specified in _clear_cert_interval.  This is to 
    # make sure that an expired host certificate doesn't cause problems - each 
    # time this method is called it should be requested again.
    #
    def _reset_certificate(self):
        log.info('Clearing consumer certificate.')
        self._clear_certificate()
        # Now set a Timer to run this again in _clear_cert_interval seconds.
        self._read_dn_timer = Timer(self._clear_cert_interval, self._reset_certificate)
        self._read_dn_timer.start()


    # Handle process of sending a request for the consumer's host certificate,
    # so we can use it during the encryption phase of transmitting messages.
    #
    # Raises an exception if the certificate request is not answered, or
    # returns True if the certificate is received
    #
    # NB The certificate response is handled asynchronously in on_message

    def _request_certificate(self):

        self._clear_certificate()
        if self._send_message(self._producer_config.send_to, SSM_CERT_REQ_MSG, NO_MSG_ID, ''):

            counter = ACK_WAIT_TIME / 0.2
            # wait for an acknowledgement which should contain the certificate
            while counter and self._get_certificate() == None:
                counter -= 1
                time.sleep (0.2)

            if counter == 0:
                raise Exception, 'Timed out waiting for certificate'

            log.debug('Got certificate')
            return True

        raise Exception, 'Failed to retrieve certificate'

    # Message handler for normal messages, it will decrypt and then attempt to
    # verify that the message is from a genuine producer. Assuming success, the
    # message will be written to a directory for another process to read from.

    def _decrypt_verify(self, headers, body):

        # Try to decrypt the message, and then verify the producer of the
        # message.  Assuming success, the message will be written out to the
        # message database

        log.debug('Decrypt/verify message length '+str(len(body)))
        log.debug(headers)

        cert = self._configuration.certificate
        key = self._configuration.key
        capath = self._configuration.capath
        check_crls = self._configuration.check_crls
        
        try:

            # Note - here I've removed the additional compression.
            decrypted = decrypt_message(body, cert,key,capath)
            signer, message64 = verify_message(decrypted, capath, check_crls)
            message = zlib.decompress(base64.decodestring(message64))

        except  Exception, err:
            log.error('Failed decrypt or verify:%s\n' % str(err))

        else:

            if not self._valid_sender(signer):
                log.warning('Received from invalid DN: '+signer)
                self._send_ack(headers['reply-to'], MSG_REJECT)
                return

            hash=message_hash(message)
            msg_id=time.strftime('%Y%m%d-%H%M%S-')+str(self._received).zfill(3)
            if self._messagedb.new_incoming_message(msg_id, signer, message):
                log.debug('Saved new message '+msg_id)
            else:
                log.warning('Already received, ignored '+msg_id)

            # finally, ack the message. We don't need to sign the ack because the
            # ack contains the md5sum of the *decrypted* message, and only the
            # producer and consumer share this secret. 

            # It is possible that the ack might not get to the producer, and so
            # someone else could spoof the consumer into thinking the producer has
            # sent a re-ack, but this is very unlikely and won't result in data
            # loss - at worst it may cause a duplicated message may be received,
            # but again - unlikely.

            self._send_ack(headers['reply-to'],hash)
        
    # Send a ping request, and re-arm the timer
    # The Timer actually runs in a separate thread, but doesn't use shared 
    # resources so it's not likely to cause any problems.
    def _send_ping(self):
        if self._is_connected:
            log.debug('Pinging')
            self._send_message(self._producer_config.send_to, SSM_PING_MSG, SSM_PING_MSG, '')

        self._ping_timer=Timer(PING_TIMER, self._send_ping)
        self._ping_timer.start()

    # Read a list of valid DNs from the file.  We assume that all lines in the 
    # file are valid DNs.
    # Like _send_ping, this has got to repeat every so often, and it uses 
    # a Timer which is another thread.  Because it needs
    # access to the valid_DNs list it must get a lock.
    def _read_valid_dn(self, filename):
        
        log.debug("Loading valid DNs file: " + filename)
        
        # Get the lock for the valid_DNs list
        self._valid_DN_lock.acquire()
        try:
            f = open(filename)
            self._valid_DNs = f.readlines()
            # strip the newlines ('\n') from each DN
            self._valid_DNs = [dn.strip().lower() for dn in self._valid_DNs]
            f.close()
        except Exception, e:
            log.warn("Exception while loading valid DNs file: " + str(e))
       
        # Always release the lock!
        self._valid_DN_lock.release()
    
        # Now set a Timer to run this again in _read_dn_interval seconds.
        self._read_dn_timer = Timer(self._read_dn_interval, self._read_valid_dn, [filename])
        self._read_dn_timer.start()

    # check if the sender's DN is in the allowed list.  _read_valid_dn() must 
    # be run first
    def _valid_sender(self, sender):
        # Acquire lock before reading from list.
        self._valid_DN_lock.acquire()
        
        valid = False
        try: 
            if sender.lower() in self._valid_DNs:
                log.info('Valid sender: ' + sender)
                valid = True
            else:
                log.info('Invalid sender: ' + sender)
        except Exception, e:
            log.warning("Error reading internal list of valid DNs: "
                         + str(e) + " - program will continue.")
        
        # Always release the lock!
        self._valid_DN_lock.release()
        
        return valid

################################################################################
# compute md5sum of the message

def message_hash(msg):
    m5 = md5.new()
    m5.update(msg)
    return m5.hexdigest()


################################################################################

def file_is_closed(filename):
    return (os.system('/usr/sbin/lsof -a ' + filename + ' > /dev/null 2>&1') != 0)


################################################################################
# Logging settings.

log = logging.getLogger('SSM')
