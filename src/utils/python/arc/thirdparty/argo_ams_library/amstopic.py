from .amsexceptions import AmsException

class AmsTopic(object):
    """Abstraction of AMS Topic

       Topic represents named bucket on AMS service that can hold messages.
       Supported methods are wrappers around the methods defined in client
       class with preconfigured topic name.
    """

    def _build_name(self, fullname):
        return fullname.split('/projects/{0}/topics/'.format(self.init.project))[1]

    def __init__(self, fullname, init):
        self.acls = None
        self.init = init
        self.fullname = fullname
        self.name = self._build_name(self.fullname)

    def delete(self):
        """Delete subscription

           Return:
               True: succesfull topic deletion
        """

        return self.init.delete_topic(self.name)

    def subscription(self, sub, ackdeadline=10, **reqkwargs):
        """Create a subscription for the topic.

           It's wrapper around few methods defined in client class. Method will
           ensure that AmsSubscription object is returned either by fetching
           existing one or creating a new one in case it doesn't exist.

           Args:
               sub (str): Name of the subscription
           Kwargs:
               ackdeadline (int): Time window in which the AMS service expects
                                  acknowledgement received for pulled messages
               reqkwargs: Keyword arguments that will be passed to underlying
                          python-requests library call.
           Return:
               object (AmsSubscription)

        """
        try:
            if self.init.has_sub(sub, **reqkwargs):
                return self.init.get_sub(sub, retobj=True, **reqkwargs)
            else:
                return self.init.create_sub(sub, self.name, ackdeadline=ackdeadline, retobj=True, **reqkwargs)

        except AmsException as e:
            raise e

    def acl(self, users=None, **reqkwargs):
        """Set or get ACLs assigned to topic

           Kwargs:
               users (list): If list of users is specified, give those user
                             access to topic. Empty list will reset access permission.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.

        """
        if users is None:
            return self.init.getacl_topic(self.name, **reqkwargs)
        else:
            return self.init.modifyacl_topic(self.name, users, **reqkwargs)

    def iter_subs(self):
        """Generator method that can be used to iterate over subscriptions
           associated with the topic
        """

        for s in self.init.iter_subs(topic=self.name):
            yield s

    def publish(self, msg, retry=0, retrysleep=60, retrybackoff=None, **reqkwargs):
        """Publish message to topic

           Args:
               msg (list, dict): One or list of dictionaries representing AMS
                                 Message
           Kwargs:
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts

               reqkwargs: Keyword argument that will be passed to underlying
                          python-requests library call.

           Returns:
               dict: Dictionary with messageIds of published messages
        """

        return self.init.publish(self.name, msg, retry=retry,
                                 retrysleep=retrysleep,
                                 retrybackoff=retrybackoff, **reqkwargs)
