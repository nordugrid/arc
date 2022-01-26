from __future__ import absolute_import

import logging
import re
import sys
from .config import _parse_config

# init module logger
logger = logging.getLogger('ARC.ConfigReference')
logger.setLevel(logging.WARNING)
log_handler_stderr = logging.StreamHandler()
log_handler_stderr.setFormatter(
    logging.Formatter('[%(asctime)s] [%(name)s] [%(levelname)s] [%(process)d] [%(message)s]'))
logger.addHandler(log_handler_stderr)

_reference_config = {}
_reference_default_lines = {}
# An ordered list of blocks. With python3-only the keys of _reference_config can be used instead
_reference_blocks = []
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
    with open(reference_f, 'rt') as ref_f:
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
                _reference_blocks.append(block_id)
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

    option_re_start = re.compile(r'^##\s+{0}\s+=\s+'.format(reqoption))

    with open(reference_f, 'rt') as referecnce:
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

def blocks_ordered(reference_f):
    if not _reference_blocks:
        process_reference(reference_f)
    return _reference_blocks

#
# reStrucutredText conversion code
#

__rst_top_header = """\
ARC Configuration Reference Document
************************************

General configuration structure
===============================

"""

__rst_blocks_header = """\
Configuration blocks and options
================================

"""

__rst_skiplist = [
    '## WARNING: this file will not work as a configuration template',
    '## NEVER USE THIS DOCUMENT AS A CONFIGURATION FILE',
]

__rst_replacelist = [
    (': \n', '::\n'),
    ('arc.conf', '``arc.conf``'),
    ('CHANGE', '\n\n.. warning::\n\n  CHANGE'),
    ('TODO', '\n\n.. warning::\n\n  TODO'),
    ('NOTE that', '\n\n.. note::\n\n ')
]


def __rst_infotype(infostr):
    sys.stdout.write('\n*{0}:* '.format(infostr))


def reference2rst(reference_f, headers=True, block_label_prefix=''):
    new_option_re = re.compile(r'^##\s+\*?([a-zA-Z0-9_]+)\s+=\s+')
    block_head_re = re.compile(r'^### The \[([^\]]+)\]')

    with open(reference_f, 'rt') as ref_f:
        in_example = False
        in_list = False
        in_codeblock = False
        new_option = False
        block_name = None
        if headers:
            sys.stdout.write(__rst_top_header)
        for confline in ref_f:
            # replace lines
            for repline, repval in __rst_replacelist:
                if repline in confline:
                    confline = confline.replace(repline, repval)
            # skip lines
            sline = confline.strip()
            for skipline in __rst_skiplist:
                if sline.startswith(skipline):
                    sline = ''
                    # continue

            # example before blocks
            if block_name is None:
                if sline.startswith('## example_config_option'):
                    sys.stdout.write('example_config_option\n');
                    sys.stdout.write('~~~~~~~~~~~~~~~~~~~~~\n');
                    __rst_infotype('Synopsis')
                    sys.stdout.write('``example_config_option = value [optional values]``\n')
                    __rst_infotype('Description')
                    sline = sline.replace('example_config_option = value [optional values] - ', '')
                elif sline.startswith('## A block configures an ARC service'):
                    sys.stdout.write('[block]\n');
                    sys.stdout.write('-------\n');


            # start of blocks
            if sline.startswith('### The [common] block') and headers:
                sys.stdout.write(__rst_blocks_header)

            # block name
            block_match = block_head_re.match(confline)
            if block_match:
                block_name = block_match.group(1)
                block_headstr = '[{0}] block'.format(block_name)
                block_label = '\n.. _reference_{0}{1}:\n\n'.format(block_label_prefix, block_name.split(':')[0])
                sys.stdout.write(block_label.replace('/', '_'))
                sys.stdout.write(block_headstr + '\n')
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
                    opt_label = '\n.. _reference_{0}_{1}:\n\n'.format(block_name.split(':')[0], optname)
                    sys.stdout.write(opt_label.replace('/', '_'))
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

            # sequenced
            if sline.startswith('## sequenced'):
                sys.stdout.write('\nThis is **sequenced** option.\n')
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
                if not shhline.startswith(' '):
                    if in_codeblock:
                        in_codeblock = False
                    if in_list:
                        in_list = False
                        sys.stdout.write('\n')
                if shhline.startswith(' ') or shhline.startswith('\n\n.. '):
                    if not in_list:
                        in_list = True
                        sys.stdout.write('\n')

                if not in_codeblock:
                    shhline = shhline.replace('[', '``[')\
                        .replace(']', ']``')\
                        .replace('"', '``')\
                        .replace('``[#]``', '[#]')\
                        .replace('``]', ']')\
                        .replace('````', '``')\
                        .replace(' *', ' \\*')

                sys.stdout.write(shhline + '\n')

                if shhline.endswith('::'):
                    in_codeblock = True
