import logging
import re
import sys
from config import _parse_config

# init module logger
logger = logging.getLogger('ARC.ConfigReference')
logger.setLevel(logging.WARNING)
log_handler_stderr = logging.StreamHandler()
log_handler_stderr.setFormatter(
    logging.Formatter('[%(asctime)s] [%(name)s] [%(levelname)s] [%(process)d] [%(message)s]'))
logger.addHandler(log_handler_stderr)

_reference_config = {}
_reference_default_lines = {}
__default_config = {}
__default_blocks = []


def parse_defaults(defaults_f):
    _parse_config(defaults_f, __default_config, __default_blocks)


def process_reference(reference_f, print_defaults=False, print_reference=False):
    __regexes = {
        'block': re.compile(r'^#\s*\[(?P<block>[^:\[\]]+(?P<block_name>:[^\[\]]+)?)\]\s*$'),
        'default': re.compile(r'^##\s*default:(?P<default>.*)\s*$'),
        'skip': re.compile(r'^(?:## .*|\s*)$'),
        'option': re.compile(r'^#(?P<option>[^=\[\]\n\s]+)=\s*(?P<value>.*)\s*$')
    }
    with open(reference_f, 'rb') as referecnce:
        block_id = None
        default_value = None
        default_line = None
        header = True
        for ln, confline in enumerate(referecnce):
            if header:
                if print_reference:
                    sys.stdout.write(confline)
                if confline.startswith('#config_option_name=56'):
                    header = False
                continue
            # block name
            block_match = re.match(__regexes['block'], confline)
            if block_match:
                block_dict = block_match.groupdict()
                block_id = block_dict['block']
                if ':' in block_id:
                    block_id = block_id.split(':')[0].strip()
                _reference_config[block_id] = {}
                logger.info('Found [%s] configuration block definition', block_id)
                if print_defaults:
                    sys.stdout.write('\n[{}]\n'.format(block_id))
                if print_reference:
                    sys.stdout.write(confline)
                continue
            # default value in .reference
            default_match = re.match(__regexes['default'], confline)
            if default_match:
                if block_id is None:
                    logger.error('Default value definition comes before block definition - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    if print_reference:
                        sys.stdout.write(confline)
                    continue
                default_value = default_match.groupdict()['default'].strip()
                default_line = ln
                if print_reference:
                    if default_line not in _reference_default_lines:
                        logger.error('Failed to find parameter to print default value for at line #%s: %s',
                                     ln + 1, confline.strip('\n'))
                        sys.stdout.write('## default: not defined\n')
                    else:
                        sys.stdout.write('## default: {}\n'.format(
                            __default_config[block_id][_reference_default_lines[ln]]))
                        continue
            # option in .reference
            option_match = re.match(__regexes['option'], confline)
            if option_match:
                if block_id is None:
                    logger.error('Option definition comes before block definition - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    if print_reference:
                        sys.stdout.write(confline)
                    continue
                option = option_match.groupdict()['option']
                if option not in _reference_config[block_id]:
                    if default_value is None:
                        logger.error('There is no default value for option definition - line #%s: %s',
                                     ln + 1, confline.strip('\n'))
                        default_value = 'no default value'
                    _reference_config[block_id].update({option: default_value})
                    _reference_default_lines.update({default_line: option})
                    if print_defaults:
                        sys.stdout.write('{}={}\n'.format(option, default_value))
                    default_value = None
                    default_line = None
            if print_reference:
                sys.stdout.write(confline)
