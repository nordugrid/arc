#!/usr/bin/python2
import logging
import json
import re

# init module logger
logger = logging.getLogger('ARC.ConfigParserPy')
logger.setLevel(logging.WARNING)
log_handler_stderr = logging.StreamHandler()
log_handler_stderr.setFormatter(
    logging.Formatter('[%(asctime)s] [%(name)s] [%(levelname)s] [%(process)d] [%(message)s]'))
logger.addHandler(log_handler_stderr)

# module-wise dict to store parsed config
__cached_config = {}


# define parsing function
def parse_arc_conf(conf_f='/etc/arc.conf'):
    __regexes = {
        'block': re.compile(r'^\[(?P<block>[-\w/]+)\]\s*$'),
        'skip': re.compile(r'^(?:#.*|\s*)$'),
        # includes support for the following cases:
        #   'option=value'
        #   'option = value' (just in case we will decide to allow spaces)
        #   'option=' or 'option'
        'option': re.compile(r'^(?P<option>[^=\[\]\n\s]+)\s*(?:=|(?=\s*$))\s*(?P<value>.*)\s*$')
    }
    with open(conf_f, 'rb') as arcconf:
        block_name = None
        for ln, confline in enumerate(arcconf):
            if re.match(__regexes['skip'], confline):
                continue
            block_match = re.match(__regexes['block'], confline)
            if block_match:
                block_name = block_match.groupdict()['block']
                __cached_config[block_name] = {}
                continue
            option_match = re.match(__regexes['option'], confline)
            if option_match:
                if block_name is None:
                    logger.error('Option definition comes before block definition and will be ignored - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    continue
                option = option_match.groupdict()['option']
                value = option_match.groupdict()['value']
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]
                if option in __cached_config[block_name]:
                    if isinstance(__cached_config[block_name][option], list):
                        value = __cached_config[block_name][option] + [value]
                    else:
                        value = [__cached_config[block_name][option], value]
                __cached_config[block_name][option] = value
                continue
            logger.warning("Failed to parse line #%s: %s", ln + 1, confline.strip('\n'))


def _config_subset(blocks=None, subsections=False):
    if blocks is None:
        blocks = []
    blocks_dict = {}
    for b in blocks:
        for s in __cached_config.keys():
            # add block if it exactly match requested name
            if s == b:
                blocks_dict[s] = __cached_config[s]
                continue
            # if requested - check for sub-blocks match
            if s.startswith(b + '/') and subsections is True:
                blocks_dict[s] = __cached_config[s]
    return blocks_dict


def export_json(blocks=None, subsections=False):
    if blocks is not None:
        return json.dumps(_config_subset(blocks, subsections))
    return json.dumps(__cached_config)


def export_bash(blocks=None, subsections=False):
    # it does not make sense to export entire configuration this way
    # if blocks chain is not specified - default is to parse just [common]
    if blocks is None:
        blocks = ['common']
    bash_config = {}
    block_dict = _config_subset(blocks, subsections)
    for b in reversed(block_dict.keys()):
        for k, v in block_dict[b].iteritems():
            if isinstance(v, list):
                bash_config['CONFIG_' + k] = '__array__'
                for i, vi in enumerate(v):
                    bash_config['CONFIG_{}_{}'.format(k, i)] = vi
            else:
                bash_config['CONFIG_' + k] = v
    eval_str = ''
    for k, v in bash_config.iteritems():
        eval_str += k + '="' + v.replace('"', '\\"') + '"\n'
    return eval_str


def get_value(option, blocks=None):
    if blocks is None:
        blocks = []
    # ability to pass single block name as a string
    if isinstance(blocks, basestring):
        blocks = [blocks]
    # return value
    for b in blocks:
        if b in __cached_config:
            if option in __cached_config[b]:
                return __cached_config[b][option]
    return None
