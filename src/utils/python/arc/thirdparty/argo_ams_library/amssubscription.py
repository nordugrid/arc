class AmsSubscription(object):
    """Abstraction of AMS subscription

       Subscription represents stream of messages that can be pulled from AMS
       service or pushed to some receiver. Supported methods are wrappers
       around methods defined in client class with preconfigured
       subscription name.
    """
    def _build_name(self, fullname):
        return fullname.split('/projects/{0}/subscriptions/'.format(self.init.project))[1]

    def __init__(self, fullname, topic, pushconfig, ackdeadline, init):
        self.acls = None
        self.init = init
        self.fullname = fullname
        self.topic = self.init.topics[topic]
        self.push_endpoint = ''
        self.retry_policy_type = ''
        self.retry_policy_period = ''
        if pushconfig['pushEndpoint']:
            self.push_endpoint = pushconfig['pushEndpoint']
            self.retry_policy_type = pushconfig['retryPolicy']['type']
            self.retry_policy_period = pushconfig['retryPolicy']['period']
        self.ackdeadline = ackdeadline
        self.name = self._build_name(self.fullname)

    def delete(self):
        """Delete subscription"""

        return self.init.delete_sub(self.name)

    def pushconfig(self, push_endpoint=None, retry_policy_type='linear',
                   retry_policy_period=300, **reqkwargs):
        """Configure Push mode parameters of subscription. When push_endpoint
           is defined, subscription will automatically start to send messages to it.

           Kwargs:
               push_endpoint (str): URL of remote endpoint that will receive messages
               retry_policy_type (str):
               retry_policy_period (int):
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.

        """

        return self.init.pushconfig_sub(self.name, push_endpoint=push_endpoint,
                                        retry_policy_type=retry_policy_type,
                                        retry_policy_period=retry_policy_period,
                                        **reqkwargs)

    def pull(self, num=1, retry=0, retrysleep=60, retrybackoff=None,
             return_immediately=False, **reqkwargs):
        """Pull messages from subscription

           Kwargs:
               num (int): Number of messages to pull
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts
               return_immediately (boolean): If True and if stream of messages is empty,
                                             subscriber call will not block and wait for
                                             messages
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
           Return:
               [(ackId, AmsMessage)]: List of tuples with ackId and AmsMessage instance
        """

        return self.init.pull_sub(self.name, num=num,
                                  return_immediately=return_immediately,
                                  **reqkwargs)

    def pullack(self, num=1, retry=0, retrysleep=60, retrybackoff=None,
                return_immediately=False, **reqkwargs):
        """Pull messages from subscription and acknownledge them in one call.

          If succesfull subscription pull immediately follows with failed
          acknownledgment (e.g. network hiccup just before acknowledgement of
          received messages), consume cycle will reset and start from
          begginning with new subscription pull.

           Kwargs:
               num (int): Number of messages to pull
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts
               return_immediately (boolean): If True and if stream of messages is empty,
                                             subscriber call will not block and wait for
                                             messages
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
           Return:
               [AmsMessage1, AmsMessage2]: List of AmsMessage instances
        """

        return self.init.pullack_sub(self.name, num=num,
                                     return_immediately=return_immediately,
                                     **reqkwargs)

    def time_to_offset(self, timestamp, **reqkwargs):
        """
           Retrieve the closest(greater than) available offset to the given timestamp.

           Args:
               timestamp(datetime.datetime): The timestamp of the offset we are looking for.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        return self.init.time_to_offset_sub(self.name, timestamp, **reqkwargs)

    def offsets(self, offset='all', move_to=0, **reqkwargs):
        """
           Retrieve the positions of min, max and current offsets or move
           current offset to new one.

           Args:
               offset (str): The name of the offset. If not specified, it will
                             return all three of them as a dict. Values that can
                             be specified are 'max', 'min', 'current' and 'all'.
               move_to (int): New position for current offset.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
           Return:
                 dict: A dictionary containing all 3 offsets. If move_to
                       is specified, current offset will be moved and updated.
                 int: The value of the specified offset.
        """
        avail_offsets = set(['max', 'min', 'current'])

        if (offset == 'all' or offset in avail_offsets) and move_to == 0:
            return self.init.getoffsets_sub(self.name, offset, **reqkwargs)
        elif move_to != 0:
            _ = self.init.modifyoffset_sub(self.name, move_to, **reqkwargs)
            return self.init.getoffsets_sub(self.name, offset, **reqkwargs)

    def acl(self, users=None, **reqkwargs):
        """Set or get ACLs assigned to subscription

           Kwargs:
               users (list): If list of users is specified, give those user
                             access to subscription. Empty list will reset access permission.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.

        """
        if users is None:
            return self.init.getacl_sub(self.name, **reqkwargs)
        else:
            return self.init.modifyacl_sub(self.name, users, **reqkwargs)

    def ack(self, ids, retry=0, retrysleep=60, retrybackoff=None, **reqkwargs):
        """Acknowledge receive of messages

           Kwargs:
               ids (list): A list of ackIds of the messages to acknowledge.
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts
        """

        return self.init.ack_sub(self.name, ids, **reqkwargs)
