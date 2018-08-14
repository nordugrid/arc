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

# module-wise data structures to store parsed config
__parsed_config = {}
__parsed_blocks = []


# define parsing function
def parse_arc_conf(conf_f='/etc/arc.conf'):
    __regexes = {
        'block': re.compile(r'^\s*\[(?P<block>[^:\[\]]+(?P<block_name>:[^\[\]]+)?)\]\s*$'),
        'skip': re.compile(r'^(?:#.*|\s*)$'),
        # includes support for the following cases:
        #   'option=value'
        #   'option = value' (just in case we will decide to allow spaces)
        #   'option=' or 'option'
        'option': re.compile(r'^\s*(?P<option>[^=\[\]\n\s]+)\s*(?:=|(?=\s*$))\s*(?P<value>.*)\s*$')
    }
    with open(conf_f, 'rt') as arcconf:
        block_id = None
        for ln, confline in enumerate(arcconf):
            if re.match(__regexes['skip'], confline):
                continue
            block_match = re.match(__regexes['block'], confline)
            if block_match:
                block_dict = block_match.groupdict()
                block_id = block_dict['block']
                __parsed_config[block_id] = {}
                __parsed_blocks.append(block_id)
                if block_dict['block_name'] is not None:
                    __parsed_config[block_id]['__block_name'] = block_dict['block_name'][1:]
                continue
            option_match = re.match(__regexes['option'], confline)
            if option_match:
                if block_id is None:
                    logger.error('Option definition comes before block definition and will be ignored - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    continue
                option = option_match.groupdict()['option']
                value = option_match.groupdict()['value']
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]
                if option in __parsed_config[block_id]:
                    if isinstance(__parsed_config[block_id][option], list):
                        value = __parsed_config[block_id][option] + [value]
                    else:
                        value = [__parsed_config[block_id][option], value]
                __parsed_config[block_id][option] = value
                continue
            logger.warning("Failed to parse line #%s: %s", ln + 1, confline.strip('\n'))


def _config_subset(blocks=None):
    if blocks is None:
        blocks = []
    blocks_dict = {}
    for cb in __parsed_config:
        if cb in blocks:
            blocks_dict[cb] = __parsed_config[cb]
    return blocks_dict


def _blocks_list(blocks=None):
    if blocks is None:
        blocks = []
    # ability to pass single block name as a string
    if isinstance(blocks, (bytes, str)):
        blocks = [blocks]
    return blocks


def export_json(blocks=None, subsections=False):
    if blocks is not None:
        if subsections:
            blocks = get_subblocks(blocks)
        return json.dumps(_config_subset(blocks))
    return json.dumps(__parsed_config)


def export_bash(blocks=None, subsections=False):
    # it does not make sense to export entire configuration this way
    # if blocks chain is not specified - default is to parse just [common]
    if blocks is None:
        blocks = ['common']
    if subsections:
        blocks = get_subblocks(blocks, is_reversed=True)
    else:
        blocks.reverse()
    blocks_dict = _config_subset(blocks)
    bash_config = {}
    for b in blocks:
        if b not in blocks_dict:
            continue
        for k, v in blocks_dict[b].items():
            if isinstance(v, list):
                bash_config['CONFIG_' + k] = '__array__'
                for i, vi in enumerate(v):
                    bash_config['CONFIG_{}_{}'.format(k, i)] = vi
            else:
                bash_config['CONFIG_' + k] = v
    eval_str = ''
    for k, v in bash_config.items():
        eval_str += k + '="' + v.replace('"', '\\"') + '"\n'
    return eval_str


def get_value(option, blocks=None, force_list=False, bool_yesno=False):
    for b in _blocks_list(blocks):
        if b in __parsed_config:
            if option in __parsed_config[b]:
                value = __parsed_config[b][option]
                if bool_yesno:
                    if value == 'yes':
                        value = True
                    elif value == 'no':
                        value = False
                if force_list and not isinstance(value, list):
                    value = [value]
                return value
    return None


def get_subblocks(blocks=None, is_reversed=False, is_sorted=False):
    if blocks is None:
        blocks = []
    subblocks = []
    for b in blocks:
        bsb = []
        for cb in __parsed_blocks:
            if re.search(r'^' + b + r'[/:]', cb):
                bsb.append(cb)
        if is_sorted:
            bsb.sort()
        parent_block = [b] if b in __parsed_blocks else []
        if is_reversed:
            subblocks = bsb + parent_block + subblocks
        else:
            subblocks += parent_block + bsb
    return subblocks


def check_blocks(blocks=None, and_logic=True):
    result = and_logic
    for b in _blocks_list(blocks):
        if and_logic:
            result = (b in __parsed_config) and result
        else:
            result = (b in __parsed_config) or result
    return result
