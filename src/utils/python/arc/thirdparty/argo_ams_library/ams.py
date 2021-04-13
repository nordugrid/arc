import json
import logging
import logging.handlers
import requests
import socket
import sys
import datetime
import time

from .amsexceptions import (AmsServiceException, AmsConnectionException,
                            AmsMessageException, AmsException,
                            AmsTimeoutException, AmsBalancerException)
from .amsmsg import AmsMessage
from .amstopic import AmsTopic
from .amssubscription import AmsSubscription

try:
    from collections import OrderedDict
except:
    from ordereddict import OrderedDict

log = logging.getLogger(__name__)


class AmsHttpRequests(object):
    """Class encapsulates methods used by ArgoMessagingService.

       Each method represent HTTP request made to AMS with the help of requests
       library. service error handling is implemented according to HTTP
       status codes returned by service and the balancer.
    """
    def __init__(self, endpoint):
        self.endpoint = endpoint
        # Create route list
        self.routes = {"topic_list": ["get", "https://{0}/v1/projects/{2}/topics?key={1}"],
                       "topic_get": ["get", "https://{0}/v1/projects/{2}/topics/{3}?key={1}"],
                       "topic_publish": ["post", "https://{0}/v1/projects/{2}/topics/{3}:publish?key={1}"],
                       "topic_create": ["put", "https://{0}/v1/projects/{2}/topics/{3}?key={1}"],
                       "topic_delete": ["delete", "https://{0}/v1/projects/{2}/topics/{3}?key={1}"],
                       "topic_getacl": ["get", "https://{0}/v1/projects/{2}/topics/{3}:acl?key={1}"],
                       "topic_modifyacl": ["post", "https://{0}/v1/projects/{2}/topics/{3}:modifyAcl?key={1}"],
                       "sub_create": ["put", "https://{0}/v1/projects/{2}/subscriptions/{4}?key={1}"],
                       "sub_delete": ["delete", "https://{0}/v1/projects/{2}/subscriptions/{4}?key={1}"],
                       "sub_list": ["get", "https://{0}/v1/projects/{2}/subscriptions?key={1}"],
                       "sub_get": ["get", "https://{0}/v1/projects/{2}/subscriptions/{4}?key={1}"],
                       "sub_pull": ["post", "https://{0}/v1/projects/{2}/subscriptions/{4}:pull?key={1}"],
                       "sub_ack": ["post", "https://{0}/v1/projects/{2}/subscriptions/{4}:acknowledge?key={1}"],
                       "sub_pushconfig": ["post", "https://{0}/v1/projects/{2}/subscriptions/{4}:modifyPushConfig?key={1}"],
                       "sub_getacl": ["get", "https://{0}/v1/projects/{2}/subscriptions/{3}:acl?key={1}"],
                       "sub_modifyacl": ["post", "https://{0}/v1/projects/{2}/subscriptions/{3}:modifyAcl?key={1}"],
                       "sub_offsets": ["get", "https://{0}/v1/projects/{2}/subscriptions/{3}:offsets?key={1}"],
                       "sub_mod_offset": ["post", "https://{0}/v1/projects/{2}/subscriptions/{3}:modifyOffset?key={1}"],
                       "auth_x509": ["get", "https://{0}:{1}/v1/service-types/ams/hosts/{0}:authx509"],
                       "sub_timeToOffset": ["get", "https://{0}/v1/projects/{2}/subscriptions/{3}:timeToOffset?key={1}&time={4}"]
                       }

        # HTTP error status codes returned by AMS according to:
        # http://argoeu.github.io/messaging/v1/api_errors/
        self.ams_errors_route = {"topic_create": ["put", set([409, 401, 403])],
                                 "topic_list": ["get", set([400, 401, 403,
                                                            404])],
                                 "topic_delete": ["delete", set([401, 403,
                                                                 404])],
                                 "sub_create": ["put", set([400, 409, 408, 401,
                                                            403])],
                                 "sub_ack": ["post", set([408, 400, 401, 403,
                                                          404])],
                                 "topic_get": ["get", set([404, 401, 403])],
                                 "topic_modifyacl": ["post", set([400, 401,
                                                                  403, 404])],
                                 "sub_get": ["get", set([404, 401, 403])],
                                 "topic_publish": ["post", set([413, 401,
                                                                403])],
                                 "sub_mod_offset": ["post", set([400, 401, 403,
                                                                 404])],
                                 "sub_pushconfig": ["post", set([400, 401, 403,
                                                                 404])],
                                 "auth_x509": ["post", set([400, 401, 403,
                                                            404])],
                                 "sub_pull": ["post", set([400, 401, 403,
                                                           404])],
                                 "sub_timeToOffset": ["get", set([400, 401,
                                                                  403, 404,
                                                                  409])]}
        # https://cbonte.github.io/haproxy-dconv/1.8/configuration.html#1.3
        self.balancer_errors_route = {"sub_ack": ["post", set([500, 502, 503, 504])],
                                      "sub_pull": ["post", set([500, 502, 503, 504])],
                                      "topic_publish": ["post", set([500, 502, 503, 504])]}

    def _error_dict(self, response_content, status):
        error_dict = dict()

        try:
            if (response_content and sys.version_info < (3, 6, ) and
                isinstance(response_content, bytes)):
                response_content = response_content.decode()
            error_dict = json.loads(response_content) if response_content else {}
        except ValueError:
            error_dict = {'error': {'code': status, 'message': response_content}}

        return error_dict

    def _gen_backoff_time(self, try_number, backoff_factor):
        for i in range(0, try_number):
            value = backoff_factor * (2 ** (i - 1))
            yield value

    def _retry_make_request(self, url, body=None, route_name=None, retry=0,
                            retrysleep=60, retrybackoff=None, **reqkwargs):
        """Wrapper around _make_request() that decides whether should request
        be retried or not.

           Two request retry modes are available:
               1) static sleep - fixed amount of seconds to sleep between
                  request attempts
               2) backoff - each next sleep before request attempt is
                  exponentially longer

           If enabled, request will be retried in the following occassions:
               * timeouts from AMS (HTTP 408) or load balancer (HTTP 408 and 504)
               * load balancer HTTP 502, 503
               * connection related problems in the lower network layers

           Default behaviour is no retry attempts. If both, retry and
           retrybackoff are enabled, retrybackoff will take precedence.

           Args:
               url: str. The final messaging service endpoint
               body: dict. Payload of the request
               route_name: str. The name of the route to follow selected from the route list
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        i = 1
        timeout = reqkwargs.get('timeout', 0)

        saved_exp = None
        if retrybackoff:
            try:
                return self._make_request(url, body, route_name, **reqkwargs)
            except (AmsBalancerException, AmsConnectionException,
                    AmsTimeoutException) as e:
                for sleep_secs in self._gen_backoff_time(retry, retrybackoff):
                    try:
                        return self._make_request(url, body, route_name, **reqkwargs)
                    except (AmsBalancerException, AmsConnectionException,
                            AmsTimeoutException) as e:
                        saved_exp = e
                        time.sleep(sleep_secs)
                        if timeout:
                            log.warning('Backoff retry #{0} after {1} seconds, connection timeout set to {2} seconds - {3}: {4}'.format(i, sleep_secs, timeout, self.endpoint, e))
                        else:
                            log.warning('Backoff retry #{0} after {1} seconds - {2}: {3}'.format(i, sleep_secs, self.endpoint, e))
                    finally:
                        i += 1
                else:
                    if saved_exp:
                        raise saved_exp
                    else:
                        raise e

        else:
            while i <= retry + 1:
                try:
                    return self._make_request(url, body, route_name, **reqkwargs)
                except (AmsBalancerException, AmsConnectionException, AmsTimeoutException) as e:
                    if i == retry + 1:
                        raise e
                    else:
                        time.sleep(retrysleep)
                        if timeout:
                            log.warning('Retry #{0} after {1} seconds, connection timeout set to {2} seconds - {3}: {4}'.format(i, retrysleep, timeout, self.endpoint, e))
                        else:
                            log.warning('Retry #{0} after {1} seconds - {2}: {3}'.format(i, retrysleep, self.endpoint, e))
                finally:
                    i += 1

    def _make_request(self, url, body=None, route_name=None, **reqkwargs):
        """Common method for PUT, GET, POST HTTP requests with appropriate
           service error handling by differing between AMS and load balancer
           erroneous behaviour.
        """
        m = self.routes[route_name][0]
        decoded = None
        try:
            # the get request based on requests.
            reqmethod = getattr(requests, m)
            r = reqmethod(url, data=body, **reqkwargs)

            content = r.content
            status_code = r.status_code

            if (content and sys.version_info < (3, 6, ) and isinstance(content,
                                                                       bytes)):
                content = content.decode()

            if status_code == 200:
                decoded = self._error_dict(content, status_code)

            # handle authnz related errors for all calls
            elif status_code == 401 or status_code == 403:
                raise AmsServiceException(json=self._error_dict(content,
                                                                status_code),
                                          request=route_name)

            elif status_code == 408 or (status_code == 504 and route_name in
                                        self.balancer_errors_route):
                raise AmsTimeoutException(json=self._error_dict(content,
                                                                status_code),
                                          request=route_name)

            # handle errors from AMS
            elif (status_code != 200 and status_code in
                  self.ams_errors_route[route_name][1]):
                raise AmsServiceException(json=self._error_dict(content,
                                                                status_code),
                                          request=route_name)

            # handle errors coming from load balancer
            elif (status_code != 200 and route_name in
                  self.balancer_errors_route and status_code in
                  self.balancer_errors_route[route_name][1]):
                raise AmsBalancerException(json=self._error_dict(content,
                                                                 status_code),
                                           request=route_name)

            # handle any other erroneous behaviour by raising exception
            else:
                raise AmsServiceException(json=self._error_dict(content,
                                                                status_code),
                                          request=route_name)

        except (requests.exceptions.ConnectionError, socket.error) as e:
            raise AmsConnectionException(e, route_name)

        else:
            return decoded if decoded else {}

    def do_get(self, url, route_name, **reqkwargs):
        """Method supports all the GET requests.

           Used for (topics, subscriptions, messages).

           Args:
               url: str. The final messaging service endpoint
               route_name: str. The name of the route to follow selected from the route list
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        # try to send a GET request to the messaging service.
        # if a connection problem araises a Connection error exception is raised.
        try:
            return self._retry_make_request(url, route_name=route_name,
                                            **reqkwargs)
        except AmsException as e:
            raise e

    def do_put(self, url, body, route_name, **reqkwargs):
        """Method supports all the PUT requests.

           Used for (topics, subscriptions, messages).

           Args:
               url: str. The final messaging service endpoint
               body: dict. Body the post data to send based on the PUT request.
                           The post data is always in json format.
               route_name: str. The name of the route to follow selected from
                           the route list
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        # try to send a PUT request to the messaging service.
        # if a connection problem araises a Connection error exception is raised.
        try:
            return self._retry_make_request(url, body=body,
                                            route_name=route_name, **reqkwargs)
        except AmsException as e:
            raise e

    def do_post(self, url, body, route_name, retry=0, retrysleep=60,
                retrybackoff=None, **reqkwargs):
        """Method supports all the POST requests.

           Used for (topics, subscriptions, messages).

           Args:
               url: str. The final messaging service endpoint
               body: dict. Body the post data to send based on the PUT request.
                     The post data is always in json format.
               route_name: str. The name of the route to follow selected from
                           the route list
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        # try to send a Post request to the messaging service.
        # if a connection problem araises a Connection error exception is raised.
        try:
            return self._retry_make_request(url, body=body,
                                            route_name=route_name, retry=retry,
                                            retrysleep=retrysleep,
                                            retrybackoff=retrybackoff,
                                            **reqkwargs)
        except AmsException as e:
            raise e

    def do_delete(self, url, route_name, **reqkwargs):
        """Delete method that is used to make the appropriate request.

           Used for (topics, subscriptions).

           Args:
               url: str. The final messaging service endpoint
               route_name: str. The name of the route to follow selected from the route list
               reqkwargs: keyword argument that will be passed to underlying python-requests library call.
        """
        # try to send a delete request to the messaging service.
        # if a connection problem araises a Connection error exception is raised.
        try:
            # the delete request based on requests.
            r = requests.delete(url, **reqkwargs)

            # JSON error returned by AMS
            if r.status_code != 200:
                errormsg = self._error_dict(r.content, r.status_code)
                raise AmsServiceException(json=errormsg, request=route_name)

            else:
                return True

        except (requests.exceptions.ConnectionError,
                requests.exceptions.Timeout) as e:
            raise AmsConnectionException(e, route_name)


class ArgoMessagingService(AmsHttpRequests):
    """Class is entry point for client code.

       Class abstract Argo Messaging Service by covering all available HTTP API
       calls that are wrapped in series of methods.
    """
    def __init__(self, endpoint, token="", project="", cert="", key="", authn_port=8443):
        super(ArgoMessagingService, self).__init__(endpoint)
        self.authn_port = authn_port
        self.token = ""
        self.endpoint = endpoint
        self.project = project
        self.assign_token(token, cert, key)
        self.pullopts = {"maxMessages": "1",
                         "returnImmediately": "false"}
        # Containers for topic and subscription objects
        self.topics = OrderedDict()
        self.subs = OrderedDict()

    def assign_token(self, token, cert, key):
        """Assign a token to the ams object

           Args:
               token(str): a valid ams token
               cert(str): a path to a valid certificate file
               key(str): a path to the associated key file for the provided certificate
        """

        # check if a token has been provided
        if token != "":
            self.token = token
            return

        try:
            # otherwise use the provided certificate to retrieve it
            self.token = self.auth_via_cert(cert, key)
        except AmsServiceException as e:
            # if the request send to authn didn't contain an x509 cert, that means that there was also no token provided
            # when initializing the ArgoMessagingService object, since we only try to authenticate through authn
            # when no token was provided

            if e.msg == 'While trying the [auth_x509]: No certificate provided.':
                refined_msg = "No certificate provided. No token provided."
                errormsg = self._error_dict(refined_msg, e.code)
                raise AmsServiceException(json=errormsg, request="auth_x509")
            raise e

    def auth_via_cert(self, cert, key, **reqkwargs):
        """Retrieve an ams token based on the provided certificate

            Args:
                cert(str): a path to a valid certificate file
                key(str): a path to the associated key file for the provided certificate

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        if cert == "" and key == "":
            errord = self._error_dict("No certificate provided.", 400)
            raise AmsServiceException(json=errord, request="auth_x509")

        # create the certificate tuple needed by the requests library
        reqkwargs = {"cert": (cert, key)}

        route = self.routes["auth_x509"]

        # Compose url
        url = route[1].format(self.endpoint, self.authn_port)
        method = getattr(self, 'do_{0}'.format(route[0]))

        try:
            r = method(url, "auth_x509", **reqkwargs)
            # if the `token` field was not found in the response, raise an error
            if "token" not in r:
                errord = self._error_dict("Token was not found in the response. Response: " + str(r), 500)
                raise AmsServiceException(json=errord, request="auth_x509")
            return r["token"]
        except (AmsServiceException, AmsConnectionException) as e:
            raise e

    def _create_sub_obj(self, s, topic):
        self.subs.update({s['name']: AmsSubscription(s['name'], topic,
                                                     s['pushConfig'],
                                                     s['ackDeadlineSeconds'],
                                                     init=self)})

    def _delete_sub_obj(self, s):
        del self.subs[s['name']]

    def _create_topic_obj(self, t):
        self.topics.update({t['name']: AmsTopic(t['name'], init=self)})

    def _delete_topic_obj(self, t):
        del self.topics[t['name']]

    def getacl_topic(self, topic, **reqkwargs):
        """Get access control lists for topic

           Args:
               topic (str): The topic name.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        topicobj = self.get_topic(topic, retobj=True)

        route = self.routes["topic_getacl"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "topic_getacl", **reqkwargs)

        if r:
            self.topics[topicobj.fullname].acls = r['authorized_users']
            return r
        else:
            self.topics[topicobj.fullname].acls = []
            return []

    def modifyacl_topic(self, topic, users, **reqkwargs):
        """Modify access control lists for topic

           Args:
               topic (str): The topic name.
               users (list): List of users that will have access to topic.
                             Empty list of users will reset access control list.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        topicobj = self.get_topic(topic, retobj=True)

        route = self.routes["topic_modifyacl"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = None
        try:
            msg_body = json.dumps({"authorized_users": users})
            r = method(url, msg_body, "topic_modifyacl", **reqkwargs)

            if r is not None:
                self.topics[topicobj.fullname].acls = users

            return True

        except (AmsServiceException, AmsConnectionException) as e:
            raise e

    def getacl_sub(self, sub, **reqkwargs):
        """Get access control lists for subscription

           Args:
               sub (str): The subscription name.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        subobj = self.get_sub(sub, retobj=True)

        route = self.routes["sub_getacl"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, sub)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "sub_getacl", **reqkwargs)

        if r:
            self.subs[subobj.fullname].acls = r['authorized_users']
            return r
        else:
            self.subs[subobj.fullname].acls = []
            return []

    def getoffsets_sub(self, sub, offset='all', **reqkwargs):
        """Retrieve the current positions of min,max and current offsets.

           Args:
               sub (str): The subscription name.
               offset(str): The name of the offset.If not specified, it will return all three of them as a dict.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_offsets"]

        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, sub)
        method = getattr(self, 'do_{0}'.format(route[0]))
        r = method(url, "sub_offsets", **reqkwargs)
        try:
            if offset != 'all':
                return r[offset]
            return r
        except KeyError as e:
            errormsg = {'error': {'message': str(e) + " is not valid offset position"}}
            raise AmsServiceException(json=errormsg, request="sub_offsets")

    def time_to_offset_sub(self, sub, timestamp, **reqkwargs):
        """Retrieve the closest(greater than) available offset to the given timestamp.

           Args:
               sub (str): The subscription name.
               timestamp(datetime.datetime): The timestamp of the offset we are looking for.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_timeToOffset"]
        method = getattr(self, 'do_{0}'.format(route[0]))

        time_in_string = ""

        if isinstance(timestamp, datetime.datetime):
            if timestamp.microsecond != 0:
                time_in_string = timestamp.isoformat()[:-3] + "Z"
            else:
                time_in_string = timestamp.strftime("%Y-%m-%d %H:%M:%S.000Z")

        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, sub, time_in_string)

        try:
            r = method(url, "sub_timeToOffset", **reqkwargs)
            return r["offset"]
        except AmsServiceException as e:
            raise e

    def modifyoffset_sub(self, sub, move_to, **reqkwargs):
        """Modify the position of the current offset.

           Args:
               sub (str): The subscription name.
               move_to(int): Position to move the offset.

           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_mod_offset"]
        method = getattr(self, 'do_{0}'.format(route[0]))

        if not isinstance(move_to, int):
            move_to = int(move_to)

        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, sub)

        # Request body
        data = {"offset": move_to}
        try:
            r = method(url, json.dumps(data), "sub_mod_offset", **reqkwargs)
            return r
        except AmsServiceException as e:
            raise e

    def modifyacl_sub(self, sub, users, **reqkwargs):
        """Modify access control lists for subscription

           Args:
               sub (str): The subscription name.
               users (list): List of users that will have access to subscription.
                             Empty list of users will reset access control list.
           Kwargs:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        subobj = self.get_sub(sub, retobj=True)

        route = self.routes["sub_modifyacl"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, sub)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = None
        try:
            msg_body = json.dumps({"authorized_users": users})
            r = method(url, msg_body, "sub_modifyacl", **reqkwargs)

            if r is not None:
                self.subs[subobj.fullname].acls = users

            return True

        except (AmsServiceException, AmsConnectionException) as e:
            raise e

    def pushconfig_sub(self, sub, push_endpoint=None, retry_policy_type='linear', retry_policy_period=300, **reqkwargs):
        """Modify push configuration of given subscription

           Args:
               sub: shortname of subscription
               push_endpoint: URL of remote endpoint that should receive messages in push subscription mode
               retry_policy_type:
               retry_policy_period:
               reqkwargs: keyword argument that will be passed to underlying python-requests library call.
        """
        if push_endpoint:
            push_dict = {"pushConfig": {"pushEndpoint": push_endpoint,
                                        "retryPolicy": {"type": retry_policy_type,
                                                        "period": retry_policy_period}}}
        else:
            push_dict = {"pushConfig": {}}

        msg_body = json.dumps(push_dict)
        route = self.routes["sub_pushconfig"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))
        p = method(url, msg_body, "sub_pushconfig", **reqkwargs)

        subobj = self.subs.get('/projects/{0}/subscriptions/{1}'.format(self.project, sub), False)
        if subobj:
            subobj.push_endpoint = push_endpoint
            subobj.retry_policy_type = retry_policy_type
            subobj.retry_policy_period = retry_policy_period

        return p

    def iter_subs(self, topic=None, **reqkwargs):
        """Iterate over AmsSubscription objects

           Args:
               topic: Iterate over subscriptions only associated to this topic name
        """
        self.list_subs(**reqkwargs)

        try:
            values = self.subs.copy().itervalues()
        except AttributeError:
            values = self.subs.copy().values()

        for s in values:
            if topic and topic == s.topic.name:
                yield s
            elif not topic:
                yield s

    def iter_topics(self, **reqkwargs):
        """Iterate over AmsTopic objects"""

        self.list_topics(**reqkwargs)

        try:
            values = self.topics.copy().itervalues()
        except AttributeError:
            values = self.topics.copy().values()

        for t in values:
            yield t

    def list_topics(self, **reqkwargs):
        """List the topics of a selected project

           Args:
               reqkwargs: keyword argument that will be passed to underlying
               python-requests library call
        """
        route = self.routes["topic_list"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "topic_list", **reqkwargs)

        for t in r['topics']:
            if t['name'] not in self.topics:
                self._create_topic_obj(t)

        if r:
            return r
        else:
            return []

    def has_topic(self, topic, **reqkwargs):
        """Inspect if topic already exists or not

           Args:
               topic: str. Topic name
        """
        try:
            self.get_topic(topic, **reqkwargs)
            return True

        except AmsServiceException as e:
            if e.code == 404:
                return False
            else:
                raise e

        except AmsConnectionException as e:
            raise e

    def get_topic(self, topic, retobj=False, **reqkwargs):
        """Get the details of a selected topic.

           Args:
               topic: str. Topic name.
               retobj: Controls whether method should return AmsTopic object
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["topic_get"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "topic_get", **reqkwargs)

        if r['name'] not in self.topics:
            self._create_topic_obj(r)

        if retobj:
            return self.topics[r['name']]
        else:
            return r

    def publish(self, topic, msg, retry=0, retrysleep=60, retrybackoff=None, **reqkwargs):
        """Publish a message or list of messages to a selected topic.

           If enabled (retry > 0), multiple topic publishes will be tried in
           case of problems/glitches with the AMS service. retry* options are
           eventually passed to _retry_make_request()

           Args:
               topic (str): Topic name.
               msg (list): A list with one or more messages to send.
                           Each message is represented as AmsMessage object or python
                           dictionary with at least data or one attribute key defined.
           Kwargs:
               retry: int. Number of request retries before giving up. Default
                           is 0 meaning no further request retry will be made
                           after first unsuccesfull request.
               retrysleep: int. Static number of seconds to sleep before next
                           request attempt
               retrybackoff: int. Backoff factor to apply between each request
                             attempts
               reqkwargs: keyword argument that will be passed to underlying
                       python-requests library call.
           Return:
               dict: Dictionary with messageIds of published messages
        """
        if not isinstance(msg, list):
            msg = [msg]
        if all(isinstance(m, AmsMessage) for m in msg):
            msg = [m.dict() for m in msg]
        try:
            msg_body = json.dumps({"messages": msg})
        except TypeError as e:
            raise AmsMessageException(e)

        route = self.routes["topic_publish"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        return method(url, msg_body, "topic_publish", retry=retry,
                      retrysleep=retrysleep, retrybackoff=retrybackoff,
                      **reqkwargs)

    def list_subs(self, **reqkwargs):
        """Lists all subscriptions in a project with a GET request.

           Args:
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_list"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "sub_list", **reqkwargs)

        for s in r['subscriptions']:
            if s['topic'] not in self.topics:
                self._create_topic_obj({'name': s['topic']})
            if s['name'] not in self.subs:
                self._create_sub_obj(s, self.topics[s['topic']].fullname)

        if r:
            return r
        else:
            return []

    def get_sub(self, sub, retobj=False, **reqkwargs):
        """Get the details of a subscription.

           Args:
               sub: str. The subscription name.
               retobj: Controls whether method should return AmsSubscription object
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_get"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "sub_get", **reqkwargs)

        if r['topic'] not in self.topics:
            self._create_topic_obj({'name': r['topic']})
        if r['name'] not in self.subs:
            self._create_sub_obj(r, self.topics[r['topic']].fullname)

        if retobj:
            return self.subs[r['name']]
        else:
            return r

    def has_sub(self, sub, **reqkwargs):
        """Inspect if subscription already exists or not

           Args:
               sub: str. The subscription name.
        """
        try:
            self.get_sub(sub, **reqkwargs)
            return True

        except AmsServiceException as e:
            if e.code == 404:
                return False
            else:
                raise e

        except AmsConnectionException as e:
            raise e

    def pull_sub(self, sub, num=1, return_immediately=False, retry=0,
                 retrysleep=60, retrybackoff=None, **reqkwargs):
        """This function consumes messages from a subscription in a project
           with a POST request.

           If enabled (retry > 0), multiple subscription pulls will be tried in
           case of problems/glitches with the AMS service. retry* options are
           eventually passed to _retry_make_request()

           Args:
               sub: str. The subscription name.
               num: int. The number of messages to pull.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """

        wasmax = self.get_pullopt('maxMessages')
        wasretim = self.get_pullopt('returnImmediately')

        self.set_pullopt('maxMessages', num)
        self.set_pullopt('returnImmediately', str(return_immediately).lower())
        msg_body = json.dumps(self.pullopts)

        route = self.routes["sub_pull"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))
        r = method(url, msg_body, "sub_pull", retry=retry,
                   retrysleep=retrysleep, retrybackoff=retrybackoff,
                   **reqkwargs)
        msgs = r['receivedMessages']

        self.set_pullopt('maxMessages', wasmax)
        self.set_pullopt('returnImmediately', wasretim)

        return list(map(lambda m: (m['ackId'], AmsMessage(b64enc=False, **m['message'])), msgs))

    def ack_sub(self, sub, ids, **reqkwargs):
        """Acknownledgment of received messages

           Messages retrieved from a pull subscription can be acknowledged by
           sending message with an array of ackIDs. The service will retrieve
           the ackID corresponding to the highest message offset and will
           consider that message and all previous messages as acknowledged by
           the consumer.

           Args:
              sub: str. The subscription name.
              ids: list(str). A list of ids of the messages to acknowledge.
              reqkwargs: keyword argument that will be passed to underlying python-requests library call.
        """

        msg_body = json.dumps({"ackIds": ids})

        route = self.routes["sub_ack"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))
        method(url, msg_body, "sub_ack", **reqkwargs)

        return True

    def pullack_sub(self, sub, num=1, return_immediately=False, retry=0,
                    retrysleep=60, retrybackoff=None, **reqkwargs):
        """Pull messages from subscription and acknownledge them in one call.

           If enabled (retry > 0), multiple subscription pulls will be tried in
           case of problems/glitches with the AMS service. retry* options are
           eventually passed to _retry_make_request().

           If succesfull subscription pull immediately follows with failed
           acknownledgment (e.g. network hiccup just before acknowledgement of
           received messages), consume cycle will reset and start from
           beginning with new subscription pull. This ensures that ack deadline
           time window is moved to new start period, that is the time when the
           second pull was initiated.

           Args:
               sub: str. The subscription name.
               num: int. The number of messages to pull.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        while True:
            try:
                ackIds = list()
                messages = list()

                for id, msg in self.pull_sub(sub, num,
                                             return_immediately=return_immediately,
                                             retry=retry,
                                             retrysleep=retrysleep,
                                             retrybackoff=retrybackoff,
                                             **reqkwargs):
                    ackIds.append(id)
                    messages.append(msg)

            except AmsException as e:
                raise e

            if messages and ackIds:
                try:
                    self.ack_sub(sub, ackIds, **reqkwargs)
                    break
                except AmsException as e:
                    log.warning('Continuing with sub_pull after sub_ack: {0}'.format(e))
                    pass
            else:
                break

        return messages

    def set_pullopt(self, key, value):
        """Function for setting pull options

           Args:
               key: str. The name of the pull option (ex. maxMessages, returnImmediately). Messaging specific
                    names are allowed.
               value: str or int. The name of the pull option (ex. maxMessages,
                      returnImmediately). Messaging specific names are allowed.
        """

        self.pullopts.update({key: str(value)})

    def get_pullopt(self, key):
        """Function for getting pull options

           Args:
               key: str. The name of the pull option (ex. maxMessages,
                    returnImmediately). Messaging specific names are allowed.

           Returns:
               str. The value of the pull option
        """
        return self.pullopts[key]

    def create_sub(self, sub, topic, ackdeadline=10, push_endpoint=None,
                   retry_policy_type='linear', retry_policy_period=300, retobj=False, **reqkwargs):
        """This function creates a new subscription in a project with a PUT request

           Args:
               sub: str. The subscription name.
               topic: str. The topic name.
               ackdeadline: int. It is a custom "ack" deadline (in seconds) in
                               the subscription. If your code doesn't
                               acknowledge the message in this time, the
                               message is sent again. If you don't specify
                               the deadline, the default is 10 seconds.
               push_endpoint: URL of remote endpoint that should receive
                              messages in push subscription mode
               retry_policy_type:
               retry_policy_period:
               retobj: Controls whether method should return AmsSubscription object
               reqkwargs: keyword argument that will be passed to underlying
               python-requests library call.
        """
        topic = self.get_topic(topic, retobj=True, **reqkwargs)

        msg_body = json.dumps({"topic": topic.fullname.strip('/'),
                               "ackDeadlineSeconds": ackdeadline})

        route = self.routes["sub_create"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, msg_body, "sub_create", **reqkwargs)

        if push_endpoint:
            ret = self.pushconfig_sub(sub, push_endpoint, retry_policy_type, retry_policy_period, **reqkwargs)
            r['pushConfig'] = {"pushEndpoint": push_endpoint,
                               "retryPolicy": {"type": retry_policy_type,
                                               "period": retry_policy_period}}

        if r['name'] not in self.subs:
            self._create_sub_obj(r, topic.fullname)

        if retobj:
            return self.subs[r['name']]
        else:
            return r

    def delete_sub(self, sub, **reqkwargs):
        """This function deletes a selected subscription in a project

           Args:
               sub: str. The subscription name.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["sub_delete"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, "", sub)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "sub_delete", **reqkwargs)

        sub_fullname = "/projects/{0}/subscriptions/{1}".format(self.project, sub)
        if sub_fullname in self.subs:
            self._delete_sub_obj({'name': sub_fullname})

        return r

    def topic(self, topic, **reqkwargs):
        """Function create a topic in a project.

           It's wrapper around few methods defined in client class. Method will
           ensure that AmsTopic object is returned either by fetching existing
           one or creating a new one in case it doesn't exist.

           Args:
               topic (str): The topic name
           Kwargs:
            reqkwargs: keyword argument that will be passed to underlying
                       python-requests library call.
           Return:
               object (AmsTopic)
        """
        try:
            if self.has_topic(topic, **reqkwargs):
                return self.get_topic(topic, retobj=True, **reqkwargs)
            else:
                return self.create_topic(topic, retobj=True, **reqkwargs)

        except AmsException as e:
            raise e

    def create_topic(self, topic, retobj=False, **reqkwargs):
        """This function creates a topic in a project

           Args:
               topic: str. The topic name.
               retobj: Controls whether method should return AmsTopic object
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["topic_create"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, '', "topic_create", **reqkwargs)

        if r['name'] not in self.topics:
            self._create_topic_obj(r)

        if retobj:
            return self.topics[r['name']]
        else:
            return r

    def delete_topic(self, topic, **reqkwargs):
        """This function deletes a topic in a project

           Args:
               topic: str. The topic name.
               reqkwargs: keyword argument that will be passed to underlying
                          python-requests library call.
        """
        route = self.routes["topic_delete"]
        # Compose url
        url = route[1].format(self.endpoint, self.token, self.project, topic)
        method = getattr(self, 'do_{0}'.format(route[0]))

        r = method(url, "topic_delete", **reqkwargs)

        topic_fullname = "/projects/{0}/topics/{1}".format(self.project, topic)
        if topic_fullname in self.topics:
            self._delete_topic_obj({'name': topic_fullname})

        return r
