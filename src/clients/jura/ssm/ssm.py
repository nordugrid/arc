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
"""
Module containing the SecureStompMessenger class, used for communicating
using the STOMP protocol.
"""

from encrypt_utils import encrypt_message, decrypt_message, verify_message, \
    sign_message, verify_certificate, from_file, get_certificate_subject, \
    check_cert_key, message_hash

#import get_brokers

import stomp
import stomp.exception

import os
from threading import Thread, Lock
import socket
import base64
import logging
import time
import zlib

log = logging.getLogger('SSM')

################################################################################
# Constants that are unlikely to change

# Times in seconds
MSG_CHECK_TIME = 1
ACK_WAIT_TIME = 60
CONNECTION_TIMEOUT = 5
# Times in minutes
PING_TIMER = 2
REFRESH_BROKERS = 1440
CLEAR_CONSUMER_CERT = 1440

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

SSM_LOGGER_ID = 'SSM'

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

class SsmException(Exception):
    """
    Exception class for use by the SSM.
    """
    pass


class SecureStompMessenger(object):
    """
    Secure Stomp Messenger class, implementing a *reliable* message transfer
    function over Stomp protocol. Listens to a nominated topic/queue for incoming
    messages, which are signed and encrypted. Messages are handled by a MessageDB
    object (see message_db.py for a basic model that uses the filesystem).
    
    Also implements an acknowledgment mechanism to ensure that message delivery
    has occurred at the consuming end, and a Re-Acknowledgment to allow a
    consumer to verify that an ack has been recieved by the producer, so we can
    stop worrying about duplicated messages arriving.
    """
    
    def __init__(self, messagedb, configuration, producer_config, consumer_config):
        """
        Given a messagedb object and configuration objects, create an 
        SSM.
        """
        # The stomppy connection object
        self._connection = None
        self._received = 0          # message counter

        # this is the identifier of the last message that we ack'd
        self._last_acked = None

        # this is the md5 sum of the previous message that was ack'd, and also
        # the re-ack value we need to send as part of our next message.
        self._reack = None
        
        # how often the SSM will check the outgoing directory for messages
        # to check. This may be changed in the configuration.
        self.msg_check_time = MSG_CHECK_TIME

        # a string representing the PEM encoded certificate of the consumer, we
        # use this certificate during the encryption process
        self._consumer_certificate = None
        
        # A separate thread to do various regular tasks every 
        # so often without interrupting control flow.
        self._admin_thread = None
        # A list of the valid DNs we have available.  It should be used only 
        # when the lock below is acquired.
        self._valid_dns = []
        # A Lock object so that the _valid_dns list is never used by two threads 
        # at the same time.
        self._valid_dn_lock = Lock()
        # How often we reset the host certificate, meaning that it must be 
        # requested again.
        self._clear_cert_interval = CLEAR_CONSUMER_CERT
        # How often we reload the valid DNs file, so it can be updated separately
        # and a running SSM will pick up the changes.
        self._read_dn_interval = None
        # How often we request the list of brokers from the BDII.
        self._fetch_brokers_interval = REFRESH_BROKERS
        # List of (host, port) tuples of brokers found in the BDII
        self._network_brokers = []
        # A Lock object so that the _network_brokers list is never used by 
        # two threads at the same time.
        self._network_brokers_lock = Lock()
        # Host and port of the currently-used broker.
        self._broker_host = None
        self._broker_port = None

        self._messagedb = messagedb
        self._configuration = configuration
        self._producer_config = producer_config
        self._consumer_config = consumer_config
        
        self._check_config()
        
        # Whether the SSM will run as a daemon or not.
        self._daemon = configuration.daemon
    
        # Connection tracking.  This is set to True whenever on_connected()
        # is called.
        self._is_connected = False
        
        # TODO: need a more respectable name.
        self._dead = False
        self._death_exception = None
        
    
    def _check_config(self):
        """
        Call the check_ssm_config() method on this instance of the SSM.
        """
        check_ssm_config(self._configuration, self._consumer_config, 
                         self._producer_config)
        

    def startup(self):
        """
        Set up one-off configuration for the SSM.
        If daemon, start a separate thread to do checks every so often.
        
        Finally, start the SSM's connection to a broker via STOMP
        """ 
        if self._consumer_config is not None:
            self._read_valid_dns()
            self._read_dn_interval = self._consumer_config.read_valid_dn_interval
            
        try: 
            self.msg_check_time = self._producer_config.msg_check_time
        except AttributeError:
            # Keep default value
            pass
        
        self._fetch_brokers()
        # Connect
        self._handle_connection()
        
        # Finally, set the background thread going to run periodic checks.
        if self._daemon:
                        
            try:
                f = open(self._configuration.pidfile, "w")
                f.write(str(os.getpid()))
                f.close()
            except IOError, e:
                log.warn("Failed to create pidfile %s: %s" % (self._configuration.pidfile, e))
                
            self._admin_thread = Thread(target = self._run_admin_thread)
            # This daemon is separate to our daemon concept.  This thread
            # must always be set as a daemon so that it ends when the process
            # is killed.
            self._admin_thread.setDaemon(True)
            self._admin_thread.start()
            
            log.debug("Started the background thread.")

                
            
            
    def shutdown(self):
        """
        Close the connection.  This is important because it runs 
        in a separate thread, so it can outlive the main process 
        if it is not ended.
        
        If running as a daemon, remove the pid file.
        
        self._admin_thread MUST be set as a daemon, and so 
        does not need to be handled here.
        """
        if self._daemon:
            pidfile = self._configuration.pidfile
            try:
                if os.path.exists(pidfile):
                    os.remove(pidfile)
                else:
                    log.warn("pidfile %s not found." % pidfile)
            except IOError, e:
                log.warn("Failed to remove pidfile %s: %e" % (pidfile, e))
                log.warn("The SSM may not start again until it is removed.")
            
        try:
            self._connection.disconnect()
        except (stomp.exception.NotConnectedException, socket.error):
            self._connection = None
        except AttributeError:
            # AttributeError if self._connection is None already
            pass
            
        log.info("SSM connection ended.")
    
    
    def initialise_connection(self):
        '''
        Create the self._connection object with the appropriate properties,
        but don't try to start the connection.
        '''
        # abbreviation
        cfg = self._configuration
        ssl_key = None
        ssl_cert = None
        username = None
        pwd = None
        
        if cfg.use_ssl:
            ssl_key = cfg.key
            ssl_cert = cfg.certificate
            log.info("Connecting using SSL using key %s and cert %s." % (ssl_key, ssl_cert))
            
        if cfg.use_pwd:
            username = cfg.username
            pwd = cfg.password
            log.info("Connecting using password using username %s." % (username))
            
        self._connection = stomp.Connection([(self._broker_host, self._broker_port)], 
                                            reconnect_sleep_initial = 5,
                                            use_ssl = cfg.use_ssl,
                                            user = username,
                                            passcode = pwd,
                                            ssl_key_file = ssl_key,
                                            ssl_cert_file = ssl_cert)

        # You can set this in the constructor but only for stomppy version 3.
        # This works for stomppy 3 but doesn't break stomppy 2.
        self._connection.__reconnect_attempts_max = 1
        self._connection.__timeout = CONNECTION_TIMEOUT
        
        self._connection.set_listener('SecureStompMessenger', self)
        
    
    def start_connection(self):
        '''
        Once self._connection exists, attempt to start it and subscribe
        to the relevant topics.
        '''
        cfg = self._configuration
        
        if self._connection is None:
            raise SsmException("Called start_connection() before a connection \
            object was initialised.")
            
        self._connection.start()
        self._connection.connect(wait = False)
    
        if self._consumer_config is not None:
            log.info("The SSM will run as a consumer.")
            topic = self._consumer_config.listen_to
            self._connection.subscribe(destination=topic, ack='auto')
            log.debug('Subscribing as a consumer to: ' + topic)
        else:
            log.info("The SSM will not run as a consumer.")

        if self._producer_config is not None:
            log.info("The SSM will run as a producer.")
            cfg = self._producer_config
            
            self._connection.subscribe(destination=cfg.ack_queue, ack='auto')
            log.debug('I will be a producer, my ack queue is: '+cfg.ack_queue)
        else:
            log.info("The SSM will not run as a producer.")
            
        self._last_acked = None
        self._reack = None
        
        i = 0
        while not self._is_connected:
            time.sleep(0.1)
            if i > CONNECTION_TIMEOUT * 10:
                message = "Timed out while waiting for connection.  Check the username and password."
                raise SsmException(message)
            i += 1
            
            
    def _handle_connection(self):
        '''
        Assuming that the SSM has retrieved the details of the broker or 
        brokers it wants to connect to, connect to one.
        
        If more than one is in the list self._network_brokers, try to 
        connect to each in turn until successful.
        '''
        # If we've got brokers from the BDII.
        if len(self._network_brokers) > 0:
            for host, port in self._network_brokers:
                self._broker_host = host
                self._broker_port = port
                self.initialise_connection()
                try:
                    self.start_connection()
                    break
                except stomp.exception.ReconnectFailedException, e:
                    # Exception doesn't provide a message.
                    log.warn("Failed to connect to %s:%s." % (host, port))
                except SsmException, e:
                    log.warn("Failed to connect to %s:%s: %s" % (host, port, e.message))
        else:  # Broker host and port specified.
            self.initialise_connection()
            try:
                self.start_connection()
            except stomp.exception.ReconnectFailedException, e:
                # Exception doesn't provide a message.
                log.warn("Failed to connect to %s:%s." % (self._broker_host, self._broker_port))
            except SsmException, e:
                log.error("Failed to connect to %s:%s: %s" % (self._broker_host, self._broker_port, e.message))
                
        if not self._is_connected:
            raise SsmException("Attempts to start the SSM failed.  The system will exit.")
        
        
    def _handle_disconnection(self):
        '''
        When disconnected, attempt to reconnect for a sensible period,
        then shut down cleanly.
        
        There are two likely scenarios - the SSM may have a choice of brokers to 
        connect to, retrieved via a BDII, or it may have one, specified by
        host and port in the configuration file.
        '''
        self._is_connected = False
        # Shut down properly
        self.shutdown()
        
        # Sometimes the SSM will reconnect to the broker before it's properly 
        # shut down!  This prevents that.
        time.sleep(2)
        
        # Try again according to the same logic as the initial startup
        try:
            self._handle_connection()
        except SsmException:
            self._is_connected = False
            
        # If reconnection fails, admit defeat.
        if not self._is_connected:
            error_message = "Reconnection attempts failed and have been abandoned."
            self._death_exception = SsmException(error_message)
            self._dead = True
            
    
    def is_dead(self):
        '''
        Whether the 'dead' flag has been set to true.
        '''
        return self._dead
    
    
    def get_death_exception(self):
        '''
        If the 'dead' flag has been set, this should return the relevant 
        exception.
        '''
        return self._death_exception
            

    def on_connecting(self, host_and_port):
        '''
        Called by stomppy when a connection attempt is made.
        '''
        log.info('Connecting: ' + str(host_and_port))


    def on_connected(self, unused_headers, unused_body):
        '''
        Set connection tracking to connected.
        Called by stomppy when the connection is acknowledged by 
        the broker.
        '''
        log.info('Connected')
        self._is_connected = True
    
    
    def on_disconnected(self, unused_headers, unused_body):
        '''
        Attempt to handle the disconnection.
        Called by stomppy when a disconnect frame is sent.
        '''
        log.warn('Disconnected from broker.')
        self._handle_disconnection()


    def on_send(self, headers, body):
        """
        Called by stomppy when a message is sent.
        """
        pass


    def on_message(self, headers, body):
        """
        Process an incoming message. The message may contain extra header
        information causing us to handle it differently. The types of message we
        handle are normal messages or acknowledgements. Normal messages may
        contain a re-acknowledgement value, to let us know the producer got our
        previous ack.
        """
        
        log.debug('Receiving message from: ' + headers['destination'])
        self._received = (self._received + 1) % 1000

        if SSM_REACK_HEADER in headers.keys():

            # we got a re-ack from the producer, so we can safely remove the
            # ack-tracking information for the message

            log.debug('Got reack ' + headers[SSM_REACK_HEADER])
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
            if self._consumer_certificate is None:
                log.info('Certificate received')
                
                try:
                    check_crls = self._configuration.check_crls
                    capath = self._configuration.capath
                    if not verify_certificate(body, capath, check_crls):
                        raise SsmException, 'Certificate failed to verify'

                    if self._producer_config.consumerDN != get_certificate_subject(body):
                        log.error('Expected ' + self._producer_config.consumerDN + 
                                  ', but got ' + get_certificate_subject(body))
                        
                        raise SsmException, 'Certificate does not match consumerDN configuration'

                    log.debug(get_certificate_subject(body))
                    self._consumer_certificate = body
                except  Exception, err:
                    log.warning('Certificate not verified: ' + str(err))

            else:
                log.warning('Unexpected certificate - ignored')

            return

        if self._producer_config is not None and headers['destination'] == self._producer_config.ack_queue:

            # message is an ack to our previous message; the message ID will be
            # the md5sum of our previously sent message

            log.debug('Received ack for ' + headers[SSM_ID_HEADER])
            self._last_acked = headers[SSM_ID_HEADER]
            return

        if headers[SSM_MSG_TYPE] == SSM_NORMAL_MSG:
            self._decrypt_verify(headers, body)
            return
        
        # Finally, as it's not an expected message type, reject it:
        log.warn("Unexpected message type received: %s", headers[SSM_MSG_TYPE])
        log.warn("Message was ignored.")


    def on_error(self, unused_headers, unused_body):
        """
        Called by stomppy if an error frame is received.
        """
        log.warning('Error frame')


    def on_receipt(self, headers, unused_body):
        """
        Called by stomppy when a message is received by the broker.
        """
        self._received = self._received + 1
        log.debug('Broker received ' + headers['receipt-id'])


    
    def process_outgoing(self):
        """
        Pick the first message from the outgoing message queue and send it,
        waiting for acknowledgement before returning. Raises an exception if the
        message is not acked.
        
        Returns True if a message was sent, or False if no message was sent or if
        there are no messages to send.
        """

        if not self._is_connected:
            return False

        (msg_id, msg_data)= self._messagedb.get_outgoing_message()
        if msg_id is None:
            return False

        # Ensure we've got a valid certificate to encode with

        if self._consumer_certificate is None:
            log.info('No certificate, requesting')
            self._request_certificate()

        m5 = message_hash(msg_data)

        log.debug('Hash: ' + m5)
        # we also need to base64 encode the message, because OpenSSL SMIME changes
        # CR to CR/LFs in the messages which I can't figure out how to stop!
        # Base64 ensures that we get the same message at the other side, and works
        # for binary too.

        log.debug('Raw length: ' + str(len(msg_data)))
        msg64 = base64.encodestring(zlib.compress(msg_data))
        log.debug('Encoded length: ' + str(len(msg64)))

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
        encrypted = encrypt_message(signed, self._consumer_certificate)
        log.debug('Encrypted length: '+str(len(encrypted)))

        if self._send_message(self._producer_config.send_to, SSM_NORMAL_MSG, msg_id, encrypted):

            # We're now waiting for an acknowledgement; this arrives over
            # our acknowledgement topic/queue

            counter = ACK_WAIT_TIME / 0.2
            while counter and self._waiting_for_ack(m5):
                time.sleep(0.2)
                counter -= 1
            
            # Rejection logging happens only after the count has run down.
            # This is possibly not ideal.

            if self._last_acked == MSG_REJECT:
                log.error('Message rejected by consumer')
                raise SsmException, 'Message ' + msg_id + ' was rejected by consumer'

            if counter == 0:
                raise SsmException, 'No acknowledgement received for message ' + msg_id


            self._messagedb.clear_outgoing_message(msg_id)

            log.debug('Message ' + msg_id + ' acknowledged by consumer')
            self._reack = m5

            return True

        return False


    def _send_message(self, destination, msg_type, msg_id, msg):
        """
        Send msg to the destination topic/queue, using the msg_id value as the
        message identifier. You may use SSM_PING_MSG for this value if an empty
        (ping) message is to be sent.
        """
        
        if not self._is_connected:
            log.error('Failed to send message ID %s: not connected' % msg_id)
            return False

        log.debug('Sending ' + msg_id)

        # standard headers, which we'll add to as necessary...
        hdrs = {SSM_MSG_TYPE: msg_type, 'destination': destination, \
                'receipt': msg_id, SSM_ID_HEADER: msg_id}

        # We only need a reply-to header if we're a producer.
        if self._producer_config is not None:
            hdrs['reply-to'] = self._producer_config.ack_queue
            
        if self._reack is not None:

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
    

    def _send_ack(self, destination, msg_hash):
        """
        Send acknowledgment of the msg_id to the destination topic/queue.
        If the message is rejected, MSG_REJECT can be sent instead of the hash.
        """
        if self._is_connected:
            log.debug('Sending ack for ' + msg_hash)
            self._connection.send('', headers = {\
                'destination': destination, \
                'receipt': msg_hash, \
                SSM_MSG_TYPE: SSM_ACK_MSG,\
                SSM_ID_HEADER: msg_hash})


    def _waiting_for_ack(self, msg_id):
        '''
        Return False if the msg_id matches the self._last_acked id.
        i.e. if it's the same, we're no longer waiting for the ack.
        ''' 
        return (self._last_acked != msg_id)

    
    def _clear_certificate(self):
        '''Clears the stored certificate of the consumer to which we are 
        sending messages.  This just means that the next time we come to 
        send a message, we'll request the certificate first.
        '''
        self._consumer_certificate = None


    def _request_certificate(self):
        """
        Handle process of sending a request for the consumer's host certificate,
        so we can use it during the encryption phase of transmitting messages.
    
        Raises an exception if the certificate request is not answered, or
        returns True if the certificate is received

        NB The certificate response is handled asynchronously in on_message
        """
        self._clear_certificate()
        
        if self._send_message(self._producer_config.send_to, SSM_CERT_REQ_MSG, NO_MSG_ID, ''):

            counter = ACK_WAIT_TIME / 0.2
            # wait for an acknowledgement which should contain the certificate
            while counter and self._consumer_certificate == None:
                counter -= 1
                time.sleep(0.2)

            if counter == 0:
                raise SsmException, 'Timed out waiting for certificate'

            log.debug('Got certificate')
            return True

        raise SsmException, 'Failed to retrieve certificate'


    def _decrypt_verify(self, headers, body):
        """
        Message handler for normal messages, it will decrypt and then attempt to
        verify that the message is from a genuine producer. Assuming success, the
        message will be written to a directory for another process to read from.
        """

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
            decrypted = decrypt_message(body, cert, key, capath)
            signer, message64 = verify_message(decrypted, capath, check_crls)
            message = zlib.decompress(base64.decodestring(message64))

        except Exception, err:
            log.error('Failed decrypt or verify:%s\n' % str(err))

        else:

            if not self._valid_sender(signer):
                log.warning('Received from invalid DN: ' + signer)
                self._send_ack(headers['reply-to'], MSG_REJECT)
                return

            msg_hash = message_hash(message)
            msg_id = time.strftime('%Y%m%d-%H%M%S-') + str(self._received).zfill(3)
            if self._messagedb.new_incoming_message(msg_id, signer, message):
                log.debug('Saved new message ' + msg_id)
            else:
                log.warning('Already received, ignored ' + msg_id)

            # finally, ack the message. We don't need to sign the ack because the
            # ack contains the md5sum of the *decrypted* message, and only the
            # producer and consumer share this secret. 

            # It is possible that the ack might not get to the producer, and so
            # someone else could spoof the consumer into thinking the producer has
            # sent a re-ack, but this is very unlikely and won't result in data
            # loss - at worst it may cause a duplicated message may be received,
            # but again - unlikely.

            self._send_ack(headers['reply-to'], msg_hash)
            
    
    def _run_admin_thread(self):
        '''
        Method run by a background thread.  It does various repeating 
        tasks which need to be done periodically, but shouldn't interfere
        with the regular control flow of the SSM.  The tasks shouldn't take
        any significant time.
        
        This method needs to be called by a threading.Thread object with 
        daemon set to true; otherwise it will hang indefinitely even if the
        python process is killed.
        ''' 
        # Number of seconds per loop.
        interval = 60
        # Don't start the counter at 0 so all the methods don't get called 
        # first time round.
        counter = 1
        
        try: 
            while True:
                
                if counter % PING_TIMER == 0:
                    self._send_ping()
                    
                if counter % self._fetch_brokers_interval == 0:
                    self._fetch_brokers()
                    
                # Read valid DNs only if running as consumer
                if self._consumer_config is not None:
                    if counter % self._read_dn_interval == 0:
                        self._read_valid_dns()
                        
                # Clear consumer certificate only if running as producer
                if self._producer_config is not None: 
                    if counter % self._clear_cert_interval == 0:
                        self._reset_certificate()
                    
                counter = counter + 1 % 10080  # minutes in a week
                
                time.sleep(interval)
                
        except Exception, e:
            # Exceptions in the background thread lead to undefined behaviour,
            # and it's difficult to recover.  Best to die, but also to be 
            # careful with what the thread does to make sure it doesn't 
            # happen often.
            log.warn("Exception in the admin thread:")
            log.warn(type(e))
            log.warn(e) 
            self._death_exception = e
            self._dead = True
        
        
    def _send_ping(self):
        """
        Send a ping request.
        """
        if self._is_connected:
            log.debug('Pinging')
            
            destination = None
            try:
                destination = self._producer_config.send_to
            except AttributeError:
                destination = self._consumer_config.listen_to
                
            self._send_message(destination, SSM_PING_MSG, SSM_PING_MSG, '')


    def _read_valid_dns(self):
        """
        Read a list of valid DNs from the file.  We assume that all lines in the 
        file are valid DNs.
        Like _send_ping, this has got to repeat every so often, and it uses 
        a Timer which is another thread.  Because it needs access to the 
        valid_dns list it must get a lock.
        """
        
        filename = self._consumer_config.valid_dn
        log.debug("Loading valid DNs file: " + filename)
        
        # Get the lock for the valid_dns list
        self._valid_dn_lock.acquire()
        try:
            f = open(filename)
            lines = f.readlines()
            # strip the newlines ('\n') from each DN
            self._valid_dns = [dn.strip().lower() for dn in lines]
            f.close()
        except Exception, e:
            log.warn(type(e))
            log.warn("Exception while loading valid DNs file: " + str(e))
            log.warn("The valid DNs have not been updated.")
       
        # Always release the lock!
        self._valid_dn_lock.release()
    
        
    def _reset_certificate(self):
        """
        Set the consumer certificate to None. This is to 
        make sure that an expired host certificate doesn't cause problems - each 
        time this method is called it should be requested again.
        """
        log.info('Clearing consumer certificate.')
        self._clear_certificate()
    
    
    def _fetch_brokers(self):
        """
        If a BDII URL and a network name are supplied in the configuration, use 
        them to find the list of available brokers and return them all.  
        Otherwise, use the host and port provided.
        """
        # abbreviation
        cfg = self._configuration
        if cfg.bdii is not None and cfg.broker_network is not None:
            log.info("BDII URL: %s" % cfg.bdii)
            log.info("BDII broker network: %s" % cfg.broker_network)
            # Use the lock!
            self._network_brokers_lock.acquire()
            self._network_brokers = fetch_brokers(cfg.bdii, cfg.broker_network, cfg.use_ssl)
            # TODO: remove testing addition
#            self._network_brokers.insert(0, ("apel-dev.esc.rl.ac.uk", 61613))
            self._network_brokers_lock.release()
            for item in self._network_brokers:
                log.debug("Found broker in BDII: %s:%d" % item)
            
        else:
            log.info("Using broker details supplied: %s:%d" % (cfg.host, cfg.port)) 
            self._broker_host = cfg.host
            self._broker_port = cfg.port
        

    def _valid_sender(self, sender):
        """
        Check if the sender's DN is in the allowed list.  _read_valid_dns() must 
        be run before this method.
        """
        # Acquire lock before reading from list.
        self._valid_dn_lock.acquire()
        
        valid = False
        try: 
            if sender.lower() in self._valid_dns:
                log.info('Valid sender: ' + sender)
                valid = True
            else:
                log.info('Invalid sender: ' + sender)
        except Exception, e:
            log.warning("Error reading internal list of valid DNs: "
                         + str(e) + " - program will continue.")
        
        # Always release the lock!
        self._valid_dn_lock.release()
        
        return valid


def check_ssm_config(config, consumer_config, producer_config):
    """
    Make basic checks on the contents of the configuration file to make
    sure they are not inconsistent.
    """
    
    if consumer_config is None and producer_config is None:
        raise SsmException, 'No producer or consumer configuration supplied'
    
    
    if consumer_config is not None and not config.daemon:
        raise SsmException("If a consumer is configured, " + \
            "the ssm must be run with the -d flag as a daemon.")
    
    if config.daemon:
        if producer_config.msg_check_time is None:
            log.info("[producer]->msg-check-time not specified. Default value" + \
                     " will be used.")
            
    # The SSM cannot work without a valid and matching cert and key.
    try:
        cert = from_file(config.certificate)
        key = from_file(config.key)
        if not check_cert_key(cert, key):
            raise SsmException("Certificate and key don't match.")
    except Exception, e:
        log.error('Error retrieving cert or key file: ' + str(e))
        raise

    # If the pidfile exists, don't start up.
    if os.path.exists(config.pidfile):
        log.warn("A pidfile %s already exists." % config.pidfile)
        log.warn("Check that the SSM is not running, then remove the file.")
        raise SsmException("SSM cannot start while pidfile exists.")
    

def fetch_brokers(bdii, network, use_ssl):
    """
    Given the URL of a BDII and the name of a broker network, retrieve
    (host, port) tuples for all the stomp brokers in the network.
    """
    broker_getter = get_brokers.StompBrokerGetter(bdii)
    if use_ssl:
        service_type = get_brokers.STOMP_SSL_SERVICE
    else:
        service_type = get_brokers.STOMP_SERVICE
    return broker_getter.get_broker_hosts_and_ports(service_type, network)
