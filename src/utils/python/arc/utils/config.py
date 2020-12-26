import logging
import json
import re
import sys
import subprocess
from arc.paths import *

# init module __logger
__logger = logging.getLogger('ARC.Config')

# module-wise data structures to store parsed configs
__parsed_config = {}
__parsed_config_admin_defined = {}
__parsed_blocks = []
__default_config = {}
__default_blocks = []

# processing constants and regexes
__def_path_arcconf = ARC_CONF
__def_path_defaults = ARC_DATA_DIR + '/arc.parser.defaults'
__def_path_runconf = ARC_RUN_DIR + '/arc.runtime.conf'

# defaults parsing constants
__no_default = 'undefined'
__var_re = re.compile(r'\$VAR\{(?:\[(?P<block>[^\[\]]+)\])?(?P<option>[^{}[\]]+)\}')
__exec_re = re.compile(r'\$EXEC\{(?P<command>[^{}]+)\}')
__eval_re = re.compile(r'\$EVAL\{(?P<evalstr>[^{}]+)\}')

# regexes to parse arc.conf structure
__arcconf_re = {
    'block': re.compile(r'^\s*\[(?P<block>[^:\[\]]+(?P<block_name>:[^\[\]]+)?)\]\s*$'),
    'skip': re.compile(r'^(?:#.*|\s*)$'),
    # includes support for the following cases:
    #   'option=value'
    #   'option = value'
    #   'option=' or 'option'
    'option': re.compile(r'^\s*(?P<option>[^=\[\]\n\s]+)\s*(?:=|(?=\s*$))\s*(?P<value>.*)\s*$')
}

# block name input
__blockname_re = re.compile(r'(?P<block_id>[^:]+):\s*(?P<block_name>.*[^\s])\s*$')


# arc.conf parsing function
def parse_arc_conf(conf_f=__def_path_arcconf, defaults_f=__def_path_defaults):
    """General entry point for parsing arc.conf (with defaults substitutions if defined)"""
    # parse /etc/arc.conf first
    _parse_config(conf_f, __parsed_config, __parsed_blocks)
    # pre-processing of values
    _process_config()
    # save parsed dictionary keys that holds arc.conf values without defaults
    for block, options_dict in __parsed_config.items():
        __parsed_config_admin_defined.update({block: options_dict.keys()})
    # defaults processing
    if defaults_f is not None:
        if os.path.exists(defaults_f):
            # parse defaults
            _parse_config(defaults_f, __default_config, __default_blocks)
            # merge defaults to parsed arc.conf
            _merge_defults()
        else:
            __logger.error('There is no defaults file at %s. Default values will not be substituted.', defaults_f)
    # evaluate special values
    _evaluate_values()


def yield_arc_conf():
    """Yield stored config dict line by line"""
    for b in __parsed_blocks:
        yield '[{0}]\n'.format(b)
        for opt, val in zip(__parsed_config[b]['__options'], __parsed_config[b]['__values']):
            yield '{0}={1}\n'.format(opt, val)
        yield '\n'


def dump_arc_conf(conf_f=__def_path_runconf):
    """Dump stored config dict to file"""
    with open(conf_f, 'wt') as arcconf:
        for confline in yield_arc_conf():
            arcconf.write(confline)


def save_run_config(runconf_f=__def_path_runconf):
    """Ensure directory exists and dump stored config"""
    try:
        runconf_dir = '/'.join(runconf_f.split('/')[:-1])
    except AttributeError:
        __logger.error('Failed to save runtime configuration. Runtime path %s is invalid.', runconf_f)
        sys.exit(1)
    try:
        if not os.path.exists(runconf_dir):
            __logger.debug('Making %s directory', runconf_dir)
            os.makedirs(runconf_dir, mode=0o755)
        dump_arc_conf(runconf_f)
    except IOError as e:
        __logger.error('Failed to save runtime configuration to %s. Error: %s', runconf_f, e.strerror)
        sys.exit(1)


def load_run_config(runconf_f=__def_path_runconf):
    """Load runing configuration (without parsing defaults)"""
    if not os.path.exists(runconf_f):
        __logger.error('There is no running configuration in %s. Dump it first.', runconf_f)
        sys.exit(1)
    _parse_config(runconf_f, __parsed_config, __parsed_blocks)


def defaults_defpath():
    """Return default path for defaults file"""
    return __def_path_defaults


def runconf_defpath():
    """Return default path for running configuration"""
    return __def_path_runconf


def arcconf_defpath():
    """Return default path for arc.conf"""
    return __def_path_arcconf


def _conf_substitute_exec(optstr):
    """Search for $EXEC{} values in parsed config structure and returns substituted command output"""
    subst = False
    execr = __exec_re.finditer(optstr)
    for ev in execr:
        try:
            exec_sp = subprocess.Popen(ev.groupdict()['command'].split(' '), stdout=subprocess.PIPE)
            value = exec_sp.stdout.readline().decode('utf-8').strip()
        except OSError:
            __logger.error('Failed to find %s command to substitute in %s. Config is not usable, terminating.',
                         ev.groupdict()['command'], ev.group(0))
            sys.exit(1)
        __logger.debug('Substituting command output %s value: %s', ev.group(0), value)
        optstr = optstr.replace(ev.group(0), value)
        subst = True
    return subst, optstr


def _conf_substitute_var(optstr, current_block):
    """Search for $VAR{} values in parsed config structure and returns substituted variable values list"""
    subst = False
    subst_optstr = [optstr]
    valr = __var_re.finditer(optstr)
    for v in valr:
        subst = True
        refvar = v.groupdict()['option']
        block = v.groupdict()['block']
        if block is None:
            block = current_block
        # references is already in arc.conf dict
        subst_value = None
        if block in __parsed_blocks:
            subst_value = _config_list_values(block, refvar)
        # but there are cases with missing block
        elif block in __default_blocks:
            __logger.warning('Reference variable for %s is in the block not defined in arc.conf', v.group(0))
            subst_value = _config_list_values(block, refvar)
            if subst_value is not None:
                if subst_value[0] == __no_default:
                    subst_value = None
        if subst_value is None:
            __logger.debug('Value for %s is not defined in arc.conf and no default value set.', v.group(0))
            subst_optstr = None
            break
        else:
            newopstr = []
            for opt in subst_optstr:
                for value in subst_value:
                    __logger.debug('Substituting variable %s value: %s', v.group(0), value)
                    newopstr.append(opt.replace(v.group(0), value))
            subst_optstr = newopstr
    return subst, subst_optstr


def _conf_substitute_eval(optstr):
    """Search for $EVAL{} values in parsed config structure and returns substituted evaluation value"""
    subst = False
    evalr = __eval_re.finditer(optstr)
    for ev in evalr:
        try:
            value = str(eval(ev.groupdict()['evalstr']))
            __logger.debug('Substituting evaluation %s value: %s', ev.group(0), value)
            optstr = optstr.replace(ev.group(0), value)
            subst = True
        except SyntaxError:
            __logger.error('Wrong syntax to evaluate %s', ev.group(0))
        except NameError as err:
            __logger.error('Wrong identifiers to evaluate %s: %s', ev.group(0), str(err))
    return subst, optstr


def _process_config():
    """Process configuration options and conditionally modify values"""
    # Allow to specify name-based loglevel values in arc.conf and replace them by numeric to be used by ARC services
    str_loglevels = {'DEBUG': '5', 'VERBOSE': '4', 'INFO': '3', 'WARNING': '2', 'ERROR': '1', 'FATAL': '0'}
    for block in __parsed_blocks:
        if 'loglevel' in __parsed_config[block]['__options']:
            loglevel_idx = __parsed_config[block]['__options'].index('loglevel')
            loglevel_value = __parsed_config[block]['__values'][loglevel_idx]
            if loglevel_value in str_loglevels.keys():
                loglevel_num_value = str_loglevels[loglevel_value]
                __logger.debug('Replacing loglevel %s with numeric value %s in [%s].',
                             loglevel_value, loglevel_num_value, block)
                __parsed_config[block]['__values'][loglevel_idx] = loglevel_num_value


def _merge_defults():
    """Add not specified config options from defaults file to parsed config dict (defined blocks only)"""
    for block in __parsed_blocks:
        dblock = block
        if ':' in block:
            dblock = block.split(':')[0].strip()
        if dblock in __default_blocks:
            for opt, val in zip(__default_config[dblock]['__options'], __default_config[dblock]['__values']):
                if opt not in __parsed_config[block]['__options']:
                    # add option from default file
                    if val != __no_default:
                        __parsed_config[block]['__options'].append(opt)
                        __parsed_config[block]['__values'].append(val)
        else:
            __logger.warning('Configuration block [%s] is not in the defaults file.', dblock)


def _evaluate_values():
    """Process substitutions of $EXEC{} $VAR{} and $EVAL{} for values in parsed dictionary"""
    #
    # NOTICE: Substitution runs ONLY ONCE in the DEFINED ORDER!
    #         More complex substitutions (e.g. EXEC depends on EVAL value) is not supported
    #         and not needed to achieve current arc.conf needs.
    # NOTICE: Substitutions are evaluated ONLY FOR the values that comes from DEFAULTS!
    #
    # Evaluate substitutions: EXEC
    # e.g. $EXEC{hostname -f}
    for block in __parsed_blocks:
        for (i, opt), val in zip(enumerate(__parsed_config[block]['__options']), __parsed_config[block]['__values']):
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            if '$EXEC' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval = _conf_substitute_exec(val)
                if subst:
                    __parsed_config[block]['__values'][i] = subval
    # Evaluate substitutions: VAR
    # e.g. $VAR{[common]globus_tcp_port_range}
    for block in __parsed_blocks:
        idx_shift = 0
        for (i, opt), val in zip(enumerate(__parsed_config[block]['__options'][:]),
                                 __parsed_config[block]['__values'][:]):
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            if '$VAR' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval_list = _conf_substitute_var(val, block)
                if subst:
                    if subval_list is not None:
                        for sv in subval_list:
                            __parsed_config[block]['__options'].insert(i + idx_shift, opt)
                            __parsed_config[block]['__values'].insert(i + idx_shift, sv)
                            idx_shift += 1
                    # remove original
                    __parsed_config[block]['__options'].pop(i+idx_shift)
                    __parsed_config[block]['__values'].pop(i+idx_shift)
                    idx_shift -= 1
    # Evaluate substitutions: EVAL
    # e.g. $EVAL{$VAR{bdii_provider_timeout} + $VAR{infoproviders_timelimit} + ${wakeupperiod}}
    for block in __parsed_blocks:
        for (i, opt), val in zip(enumerate(__parsed_config[block]['__options']), __parsed_config[block]['__values']):
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            if '$EVAL' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval = _conf_substitute_eval(val)
                if subst:
                    __parsed_config[block]['__values'][i] = subval


def _canonicalize_blockid(block):
    # nothing to do with blocks without names
    if ':' not in block:
        return block
    # get name wthout spaces
    re_match = __blockname_re.match(block)
    if re_match:
        re_dict = re_match.groupdict()
        return '{block_id}:{block_name}'.format(**re_dict)
    return block


def _parse_config(conf_f, parsed_confdict_ref, parsed_blockslist_ref):
    """Parse arc.conf-formatted configuration file to specified data structures"""
    with open(conf_f, 'rt') as arcconf:
        block_id = None
        for ln, confline in enumerate(arcconf):
            if __arcconf_re['skip'].match(confline):
                continue
            block_match = __arcconf_re['block'].match(confline)
            if block_match:
                block_dict = block_match.groupdict()
                block_id = _canonicalize_blockid(block_dict['block'])
                parsed_confdict_ref[block_id] = {'__options': [], '__values': []}
                parsed_blockslist_ref.append(block_id)
                if block_dict['block_name'] is not None:
                    parsed_confdict_ref[block_id]['__block_name'] = block_dict['block_name'][1:].strip()
                continue
            # match for option = value
            option_match = __arcconf_re['option'].match(confline)
            if option_match:
                if block_id is None:
                    __logger.error('Option definition comes before block definition and will be ignored - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    continue
                option = option_match.groupdict()['option']
                value = option_match.groupdict()['value'].strip()
                # ordered lists of options
                parsed_confdict_ref[block_id]['__options'].append(option)
                parsed_confdict_ref[block_id]['__values'].append(value)
                continue
            __logger.warning("Failed to parse line #%s: %s", ln + 1, confline.strip('\n'))


def _config_list_values(block, option):
    """Returns list of options values in the block (list of one element if not multivalued or None if not defined)"""
    if block not in __parsed_config:
        return None
    values = None
    if option in __parsed_config[block]['__options']:
        values = [__parsed_config[block]['__values'][i]
                  for i, opt in enumerate(__parsed_config[block]['__options'])
                  if opt == option]
    return values


def _config_dict(blocks=None):
    """Returns configuration dictionary for requested blocks"""
    parsed_dict = {}
    for b in __parsed_blocks:
        if blocks is not None:
            if b not in blocks:
                continue
        parsed_dict[b] = {}
        for k, v in zip(__parsed_config[b]['__options'], __parsed_config[b]['__values']):
            if k in parsed_dict[b]:
                if not isinstance(parsed_dict[b][k], list):
                    parsed_dict[b][k] = [parsed_dict[b][k]]
                parsed_dict[b][k].append(v)
            else:
                parsed_dict[b][k] = v
    return parsed_dict


def _blocks_list(blocks=None):
    """Returns list from string or list argument"""
    if blocks is None:
        blocks = []
    # ability to pass single block name as a string
    if isinstance(blocks, (bytes, str)):
        blocks = [blocks]
    return blocks


def export_json(blocks=None, subsections=False):
    """
    Returns JSON representation of parsed config structure
    :param blocks: List of blocks to export
    :param subsections: Export all subblocks
    :return: JSON-dumped string
    """
    if blocks is not None:
        blocks = [_canonicalize_blockid(b) for b in blocks]
        if subsections:
            blocks = get_subblocks(blocks)
    return json.dumps(_config_dict(blocks))


def export_bash(blocks=None, subsections=False, options_filter=None):
    """
    Returns CONFIG_key="value" strings ready to eval in shell
    :param blocks: List of blocks to export
    :param subsections: Export all subblocks
    :param options_filter: Limit exported options only to the specified list
    :return: Eval-ready string
    """
    # it does not make sense to export entire configuration this way
    # if blocks chain is not specified - default is to parse just [common]
    if blocks is None:
        blocks = ['common']
    if subsections:
        blocks = get_subblocks(blocks, is_reversed=True)
    else:
        blocks.reverse()
    # loop over block and update bash config in precedence order
    bash_config = {}
    for b in blocks:
        b = _canonicalize_blockid(b)
        if b not in __parsed_blocks:
            continue
        block_config = {}
        for k, v in zip(__parsed_config[b]['__options'], __parsed_config[b]['__values']):
            if options_filter:
                if k not in options_filter:
                    __logger.debug('Option "%s" will not be exported (not in allowed list)', k)
                    continue
            bash_key = 'CONFIG_' + k
            # if key exists already in block (multivalued) - create list of values
            if bash_key in block_config:
                if not isinstance(block_config[bash_key], list):
                    block_config[bash_key] = [block_config[bash_key]]
                block_config[bash_key].append(v)
            else:
                block_config[bash_key] = v
        bash_config.update(block_config)
    # construct eval string
    eval_str = ''
    for k, v in bash_config.items():
        if isinstance(v, list):
            eval_str += k + '="__array__"\n'
            for i, vi in enumerate(v):
                eval_str += '{0}_{1}="{2}"\n'.format(k, i, vi.replace('"', '\\"'))
        else:
            eval_str += '{0}="{1}"\n'.format(k, v.replace('"', '\\"'))
    return eval_str


def get_value(option, blocks=None, force_list=False, bool_yesno=False):
    """
    Return the config option value
    :param option: option name
    :param blocks: list of blocks (in order to check for option)
    :param force_list: always return list
    :param bool_yesno: return boolean True/False instead of string yes/no
    :return: option value
    """
    for b in _blocks_list(blocks):
        b = _canonicalize_blockid(b)
        if b in __parsed_config:
            values = _config_list_values(b, option)
            if values is not None:
                # convert yes/no to True/False if requested
                if bool_yesno:
                    for i, value in enumerate(values):
                        if value == 'yes':
                            values[i] = True
                        elif value == 'no':
                            values[i] = False
                # return value if list is not requested
                if not force_list and len(values) == 1:
                    return values[0]
                return values
    if force_list:
        return []
    return None


def get_subblocks(blocks=None, is_reversed=False, is_sorted=False):
    """
    Fetch all subblocks for defined parent blocks
    :param blocks: List of blocks
    :param is_reversed: Subblocks in the list is before parent block
    :param is_sorted: Subblocks within the block are sorted alphabetically
    :return: List of blocks with subblocks
    """
    if blocks is None:
        blocks = []
    subblocks = []
    for b in _blocks_list(blocks):
        b = _canonicalize_blockid(b)
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


def get_config_dict(blocks=None):
    """Returns the entire dictionary that holds parsed configuration"""
    if blocks is not None:
        blocks = [_canonicalize_blockid(b) for b in blocks]
    return _config_dict(blocks)

def get_default_config_dict():
    """Returns the dictionary of default configuration"""
    return __default_config

def get_config_blocks():
    """Returns list of configuration blocks (order is preserved)"""
    return __parsed_blocks


def check_blocks(blocks=None, and_logic=True):
    """
    Check specified blocks exists in configuration
    :param blocks: List of blocks to check
    :param and_logic: All blocks should be there to return True
    :return: Check status
    """
    result = and_logic
    for b in _blocks_list(blocks):
        b = _canonicalize_blockid(b)
        if and_logic:
            result = (b in __parsed_blocks) and result
        else:
            result = (b in __parsed_blocks) or result
    return result
