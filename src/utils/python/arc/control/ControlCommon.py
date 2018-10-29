import os
import logging
from arc.utils import config
from arc.paths import *

logger = logging.getLogger('ARCCTL.Common')


def get_parsed_arcconf(config_file):
    try:
        config.parse_arc_conf(config_file)
        arcconf = config
    except IOError:
        arcconf = None
    return arcconf


class ComponentControl(object):
    """ Common abstract class to ensure all implicit calls to methods are defined """
    def control(self, args):
        raise NotImplementedError()

    @staticmethod
    def register_parser(root_parser):
        raise NotImplementedError()

