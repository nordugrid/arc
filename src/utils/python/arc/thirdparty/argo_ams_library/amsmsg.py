import sys
import json
import inspect
from base64 import b64encode, b64decode
from collections import Callable
from .amsexceptions import AmsMessageException

class AmsMessage(Callable):
    """Abstraction of AMS Message

       AMS Message is constituted from mandatory data field
       and arbitrary number of attributes. Data is Base64
       encoded prior dispatching message to AMS service and
       Base64 decoded when it is being pulled from service.
    """
    def __init__(self, b64enc=True, attributes='', data=None,
                 messageId='', publishTime=''):
        self._attributes = attributes
        self._messageId = messageId
        self._publishTime = publishTime
        self.set_data(data, b64enc)

    def __call__(self, **kwargs):
        if 'attributes' not in kwargs:
            kwargs.update({'attributes': self._attributes})
        self.__init__(b64enc=True, **kwargs)

        return self.dict()

    def _has_dataattr(self):
        if not getattr(self, '_data', False) and not self._attributes:
            raise AmsMessageException('At least data field or one attribute needs to be defined')

        return True

    def set_attr(self, key, value):
        """Set attributes of message

           Args:
               key (str): Key of the attribute
               value (str): Value of the attribute
        """
        self._attributes.update({key: value})

    def set_data(self, data, b64enc=True):
        """Set data of message

           Default behaviour is to Base64 encode data prior sending it by the
           publisher. Method is also used internally when pull from subscription
           is being made by subscriber and in that case, data is already Base64 encoded

           Args:
               data (str): Data of the message
           Kwargs:
               b64enc (bool): Control whether data should be Base64 encoded
        """
        if b64enc and data:
            try:
                if sys.version_info < (3, ):
                    self._data = b64encode(data)
                else:
                    if isinstance(data, bytes):
                        self._data = str(b64encode(data), 'utf-8')
                    else:
                        self._data = str(b64encode(bytearray(data, 'utf-8')), 'utf-8')
            except Exception as e:
                raise AmsMessageException('b64encode() {0}'.format(str(e)))
        elif data:
            self._data = data

    def dict(self):
        """Construct python dict from message"""

        if self._has_dataattr():
            d = dict()
            for attr in ['attributes', 'data', 'messageId', 'publishTime']:
                if getattr(self, '_{0}'.format(attr), False):
                    v = eval('self._{0}'.format(attr))
                    d.update({attr: v})
            return d

    def get_data(self):
        """Fetch the data of the message and Base64 decode it"""

        if self._has_dataattr():
            try:
                return b64decode(self._data)
            except Exception as e:
                raise AmsMessageException('b64decode() {0}'.format(str(e)))

    def get_msgid(self):
        """Fetch the message id of the message"""

        return self._messageId

    def get_publishtime(self):
        """Fetch the publish time of the message set by the AMS service"""

        return self._publishTime

    def get_attr(self):
        """Fetch all attributes of the message"""

        if self._has_dataattr():
            return self._attributes

    def json(self):
        """Return JSON interpretation of the message"""

        return json.dumps(self.dict())

    def __str__(self):
        return str(self.dict())
