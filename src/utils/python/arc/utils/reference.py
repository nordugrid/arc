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

__regexes = {
    'block': re.compile(r'^#\s*\[(?P<block>[^:\[\]]+(?P<block_name>:[^\[\]]+)?)\]\s*$'),
    'default': re.compile(r'^##\s*default:(?P<default>.*)\s*$'),
    'skip': re.compile(r'^(?:## .*|\s*)$'),
    'option': re.compile(r'^#(?P<option>[^=\[\]\n\s]+)=\s*(?P<value>.*)\s*$')
}


def parse_defaults(defaults_f):
    _parse_config(defaults_f, __default_config, __default_blocks)


def process_reference(reference_f, print_defaults=False, print_reference=False):
    with open(reference_f, 'rb') as ref_f:
        block_id = None
        default_value = None
        default_line = None
        header = True
        for ln, confline in enumerate(ref_f):
            if header:
                if print_reference:
                    sys.stdout.write(confline)
                if confline.startswith('#example_config_option=56'):
                    header = False
                continue
            # block name
            block_match = __regexes['block'].match(confline)
            if block_match:
                block_dict = block_match.groupdict()
                block_id = block_dict['block']
                if ':' in block_id:
                    block_id = block_id.split(':')[0].strip()
                _reference_config[block_id] = {}
                logger.info('Found [%s] configuration block definition', block_id)
                if print_defaults:
                    sys.stdout.write('\n[{0}]\n'.format(block_id))
                if print_reference:
                    sys.stdout.write(confline)
                continue
            # default value in .reference
            default_match = __regexes['default'].match(confline)
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
                        sys.stdout.write('## default: {0}\n'.format(
                            __default_config[block_id][_reference_default_lines[ln]]))
                        continue
            # option in .reference
            option_match = __regexes['option'].match(confline)
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
                        sys.stdout.write('{0}={1}\n'.format(option, default_value))
                    default_value = None
                    default_line = None
            if print_reference:
                sys.stdout.write(confline)


def get_option_description(reference_f, reqblock, reqoption):
    if ':' in reqblock:
        reqblock = reqblock.split(':')[0].strip()

    option_re_start = re.compile('^##\s+{0}\s+=\s+'.format(reqoption))

    with open(reference_f, 'rb') as referecnce:
        in_correct_block = False
        is_option_descr = False
        for confline in referecnce:
            if not in_correct_block:
                block_match = __regexes['block'].match(confline)
                if block_match:
                    block_dict = block_match.groupdict()
                    block_id = block_dict['block']
                    if ':' in block_id:
                        block_id = block_id.split(':')[0].strip()
                    if block_id == reqblock:
                        in_correct_block = True
                        continue
            else:
                if not is_option_descr:
                    if option_re_start.match(confline):
                        is_option_descr = True
                        yield confline
                else:
                    if not confline.startswith('##'):
                        break
                    yield confline


def is_multivalued(reference_f, reqblock, reqoption):
    for line in get_option_description(reference_f, reqblock, reqoption):
        if line.startswith('## multivalued'):
            return True
    return False


def allowed_values(reference_f, reqblock, reqoption):
    for line in get_option_description(reference_f, reqblock, reqoption):
        if line.startswith('## allowedvalues:'):
            return line[17:].split()
    return None

#
# reStrucutredText conversion code
#

__rst_top_header = """\
ARC Configuration Reference Document
************************************

Configuration structure
=======================

"""

__rst_blocks_header = """\
Configuration blocks
===================="""

__rst_skiplist = [
    '## WARNING: this file will not work as a configuration template',
    '## NEVER USE THIS DOCUMENT AS A CONFIGURATION FILE',
]

__rst_replacelist = [
    ('configuration file consists of the following blocks', 'configuration file consists of the following blocks::\n'),
    ('for configuration lines:', 'for configuration lines::\n'),
    ('and for block headers:', '\nand for block headers::\n'),
    ('arc.conf', '``arc.conf``'),
    ('CHANGE', '\n\n.. warning::\n\n  CHANGE'),
    ('TODO', '\n\n.. warning::\n\n  TODO')
]


def __rst_infotype(infostr):
    sys.stdout.write('\n*{0}:* '.format(infostr))


def reference2rst(reference_f):
    new_option_re = re.compile(r'^##\s+\*?([a-zA-Z0-9_]+)\s+=\s+')
    block_head_re = re.compile(r'^### The \[([^\]]+)\]')

    with open(reference_f, 'rb') as ref_f:
        in_example = False
        in_skip = False
        in_list = False
        in_codeblock = False
        new_option = False
        sys.stdout.write(__rst_top_header)
        for confline in ref_f:
            # skip lines
            sline = confline.strip()
            for skipline in __rst_skiplist:
                if sline.startswith(skipline):
                    sline = ''
                    # continue
            if sline.startswith('## Below we give a detailed description of all the configuration options'):
                in_skip = True
            if in_skip:
                if sline.startswith('#example_config_option=56'):
                    sys.stdout.write(__rst_blocks_header)
                    in_skip = False
                continue
            # replace lines
            for repline, repval in __rst_replacelist:
                if repline in sline:
                    sline = sline.replace(repline, repval)

            # block name
            block_match = block_head_re.match(confline)
            if block_match:
                block_headstr = '[{0}] block'.format(block_match.group(1))
                sys.stdout.write('\n' + block_headstr + '\n')
                sys.stdout.write('-' * len(block_headstr) + '\n\n')
                continue

            # scip other ### lines
            if sline.startswith('###'):
                continue

            # empty lines before new options
            if sline == '':
                sys.stdout.write('\n')
                new_option = True
                in_example = False
                continue

            # new option headers
            if new_option:
                nopt = new_option_re.match(sline)
                if nopt:
                    optname = nopt.group(1)
                    sys.stdout.write(optname + '\n')
                    sys.stdout.write('~' * len(optname) + '\n\n')
                    # synopsis and string separated by dash
                    dash_pos = sline.find('-')
                    __rst_infotype('Synopsis')
                    sys.stdout.write('``{0}``\n'.format(sline[3:dash_pos].strip()))
                    __rst_infotype('Description')
                    sline = '##' + sline[dash_pos+1:]
                new_option = False

            # example lines
            option_match = __regexes['option'].match(sline)
            if option_match:
                if not in_example:
                    sys.stdout.write('\n*Example*::\n\n')
                    in_example = True
                sys.stdout.write('  ' + sline[1:] + '\n')

            # multivalued
            if sline.startswith('## multivalued'):
                sys.stdout.write('\nThis option in **multivalued**.\n')
                continue

            # allowedvalues
            if sline.startswith('## allowedvalues:'):
                __rst_infotype('Allowed values')
                sys.stdout.write('``{0}``\n'.format('``, ``'.join(sline[17:].split())))
                continue

            # default value in .reference
            default_match = __regexes['default'].match(confline)
            if default_match:
                default_value = default_match.groupdict()['default'].strip()
                __rst_infotype('Default')
                sys.stdout.write('``{0}``\n\n'.format(default_value))
                continue

            # print description lines as is
            if sline == '##':
                sys.stdout.write('\n')
            if sline.startswith('## '):
                shhline = sline[3:]
                # if not shhline.startswith(' '):
                if not in_codeblock:
                    shhline = shhline.replace('[', '``[')\
                        .replace(']', ']``')\
                        .replace('*', '\*')\
                        .replace('"', '``')\
                        .replace('``[#]``', '[#]')
                if not shhline.startswith(' '):
                    if in_codeblock:
                        in_codeblock = False
                    if in_list:
                        in_list = False
                        sys.stdout.write('\n')
                else:
                    if not in_list:
                        in_list = True
                        sys.stdout.write('\n')

                sys.stdout.write(shhline + '\n')

                if shhline.endswith('::\n'):
                    in_codeblock = True
