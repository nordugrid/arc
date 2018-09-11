import logging
import json
import re
import sys
import os
import subprocess

# init module logger
logger = logging.getLogger('ARC.ConfigParserPy')
logger.setLevel(logging.WARNING)
log_handler_stderr = logging.StreamHandler()
log_handler_stderr.setFormatter(
    logging.Formatter('[%(asctime)s] [%(name)s] [%(levelname)s] [%(process)d] [%(message)s]'))
logger.addHandler(log_handler_stderr)

# set relevant paths
try:
    from arc import paths
    ARC_CONF = paths.ARC_CONF
    ARC_DATA_DIR = paths.ARC_DATA_DIR
    ARC_RUN_DIR = paths.ARC_RUN_DIR
except ImportError:
    paths = None
    logger.error('There are no installation specific paths defined. '
                 'It seams you are running config parser without autoconf - hardcoded defaults will be used.')
    ARC_CONF = '/etc/arc.conf'
    ARC_DATA_DIR = '/usr/share/arc'
    ARC_RUN_DIR = '/var/run/arc'

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


# arc.conf parsing function
def parse_arc_conf(conf_f=__def_path_arcconf, defaults_f=__def_path_defaults):
    """General entry point for parsing arc.conf (with defaults substitutions if defined)"""
    # parse /etc/arc.conf first
    _parse_config(conf_f, __parsed_config, __parsed_blocks)
    # save parsed dictionary keys that holds original arc.conf values
    for block, options_dict in __parsed_config.items():
        __parsed_config_admin_defined.update({block: options_dict.keys()})
    if defaults_f is not None:
        if os.path.exists(defaults_f):
            # parse defaults
            _parse_config(defaults_f, __default_config, __default_blocks)
            # merge defaults to parsed arc.conf
            _merge_defults()
        else:
            logger.error('There is no defaults file at %s. Default values will not be substituted.', defaults_f)
    # evaluate special values
    _evaluate_values()


def yield_arc_conf():
    """Yield stored config dict line by line"""
    for b in __parsed_blocks:
        yield '[{0}]\n'.format(b)
        for opt in __parsed_config[b]['__options_order']:
            val = __parsed_config[b][opt]
            if isinstance(val, list):
                for v in val:
                    yield '{0}={1}\n'.format(opt, v)
            else:
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
        logger.error('Failed to save runtime configuration. Runtime path %s is invalid.', runconf_f)
        sys.exit(1)
    try:
        if not os.path.exists(runconf_dir):
            logger.debug('Making %s directory', runconf_dir)
            os.makedirs(runconf_dir, mode=0o755)
        dump_arc_conf(runconf_f)
    except IOError as e:
        logger.error('Failed to save runtime configuration to %s. Error: %s', runconf_f, e.strerror)
        sys.exit(1)


def load_run_config(runconf_f=__def_path_runconf):
    """Load runing configuration (without parsing defaults)"""
    if not os.path.exists(runconf_f):
        logger.error('There is no running configuration in %s. Dump it first.', runconf_f)
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
            value = exec_sp.stdout.readline().strip()
        except OSError:
            logger.error('Failed to find %s command to substitute in %s. Config is not usable, terminating.',
                         ev.groupdict()['command'], ev.group(0))
            sys.exit(1)
        logger.debug('Substituting command output %s value: %s', ev.group(0), value)
        optstr = optstr.replace(ev.group(0), value)
        subst = True
    return subst, optstr


def _conf_substitute_var(optstr, current_block):
    """Search for $VAR{} values in parsed config structure and returns substituted variable value"""
    subst = False
    valr = __var_re.finditer(optstr)
    for v in valr:
        refvar = v.groupdict()['option']
        block = v.groupdict()['block']
        if block is None:
            block = current_block
        # references is already in arc.conf dict
        value = None
        if block in __parsed_blocks:
            if refvar in __parsed_config[block]:
                value = __parsed_config[block][refvar]
        # but there are cases with missing block
        elif block in __default_blocks:
            logger.warning('Reference variable for %s is in the block not defined in arc.conf', v.group(0))
            if refvar in __default_config[block]:
                if __default_config[block][refvar] != __no_default:
                    value = __default_config[block][refvar]
        if value is None:
            logger.debug('Failed to find reference variable value for %s. Skipping.', v.group(0))
            optstr = None
        else:
            logger.debug('Substituting variable %s value: %s', v.group(0), value)
            # for multivalued parameters substitution replaces the value with the whole list
            if isinstance(value, list):
                optstr = value
            else:
                optstr = optstr.replace(v.group(0), value)
        subst = True
    return subst, optstr


def _conf_substitute_eval(optstr):
    """Search for $EVAL{} values in parsed config structure and returns substituted evaluation value"""
    subst = False
    evalr = __eval_re.finditer(optstr)
    for ev in evalr:
        try:
            value = str(eval(ev.groupdict()['evalstr']))
            logger.debug('Substituting evaluation %s value: %s', ev.group(0), value)
            optstr = optstr.replace(ev.group(0), value)
            subst = True
        except SyntaxError:
            logger.error('Wrong syntax to evaluate %s', ev.group(0))
        except NameError as err:
            logger.error('Wrong identifiers to evaluate %s: %s', ev.group(0), str(err))
    return subst, optstr


def _merge_defults():
    """Add not specified config options from defaults file to parsed config dict (defined blocks only)"""
    for block in __parsed_blocks:
        optdict = __parsed_config[block]
        if ':' in block:
            block = block.split(':')[0].strip()
        if block in __default_config:
            for opt in __default_config[block]['__options_order']:
                val = __default_config[block][opt]
                if opt not in optdict:
                    # add option from default file
                    if val != __no_default:
                        optdict['__options_order'].append(opt)
                        optdict.update({opt: val})
        else:
            logger.warning('Configuration block [%s] is not in the defaults file.', block)


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
        for opt in __parsed_config[block]['__options_order']:
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            val = __parsed_config[block][opt]
            if '$EXEC' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval = _conf_substitute_exec(val)
                if subst:
                    __parsed_config[block][opt] = subval
    # Evaluate substitutions: VAR
    # e.g. $VAR{[common]globus_tcp_port_range}
    for block in __parsed_blocks:
        for opt in list(__parsed_config[block]['__options_order']):
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            val = __parsed_config[block][opt]
            if '$VAR' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval = _conf_substitute_var(val, block)
                if subst:
                    if subval is None:
                        __parsed_config[block]['__options_order'].remove(opt)
                        del __parsed_config[block][opt]
                    else:
                        __parsed_config[block][opt] = subval
    # Evaluate substitutions: EVAL
    # e.g. $EVAL{$VAR{bdii_provider_timeout} + $VAR{infoproviders_timelimit} + ${wakeupperiod}}
    for block in __parsed_blocks:
        for opt in __parsed_config[block]['__options_order']:
            # skip if option is defined in the /etc/arc.conf
            if opt in __parsed_config_admin_defined[block]:
                continue
            val = __parsed_config[block][opt]
            if '$EVAL' in val:
                # try to substitute (match regex and proceed on match)
                subst, subval = _conf_substitute_eval(val)
                if subst:
                    __parsed_config[block][opt] = subval


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
                block_id = block_dict['block']
                parsed_confdict_ref[block_id] = {'__options_order': []}
                parsed_blockslist_ref.append(block_id)
                if block_dict['block_name'] is not None:
                    parsed_confdict_ref[block_id]['__block_name'] = block_dict['block_name'][1:].strip()
                continue
            # match for option = value
            option_match = __arcconf_re['option'].match(confline)
            if option_match:
                if block_id is None:
                    logger.error('Option definition comes before block definition and will be ignored - line #%s: %s',
                                 ln + 1, confline.strip('\n'))
                    continue
                option = option_match.groupdict()['option']
                value = option_match.groupdict()['value']
                # TODO: no quotes strip (check "" as default values first)
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]
                # values are lists when multivalued
                if option in parsed_confdict_ref[block_id]:
                    if isinstance(parsed_confdict_ref[block_id][option], list):
                        value = parsed_confdict_ref[block_id][option] + [value]
                    else:
                        value = [parsed_confdict_ref[block_id][option], value]
                # add option to the ordered list
                if option not in parsed_confdict_ref[block_id]['__options_order']:
                    parsed_confdict_ref[block_id]['__options_order'].append(option)
                # add option to confdict
                parsed_confdict_ref[block_id][option] = value
                continue
            logger.warning("Failed to parse line #%s: %s", ln + 1, confline.strip('\n'))


def _config_subset(blocks=None):
    """Returns configuration dictionary for requested blocks"""
    if blocks is None:
        blocks = []
    blocks_dict = {}
    for cb in __parsed_config:
        if cb in blocks:
            blocks_dict[cb] = __parsed_config[cb]
    return blocks_dict


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
        if subsections:
            blocks = get_subblocks(blocks)
        return json.dumps(_config_subset(blocks))
    return json.dumps(__parsed_config)


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
    blocks_dict = _config_subset(blocks)
    bash_config = {}
    for b in blocks:
        if b not in blocks_dict:
            continue
        for k in blocks_dict[b]['__options_order']:
            if options_filter:
                if k not in options_filter:
                    logger.debug('Option "%s" will not be exported (not in allowed list)', k)
                    continue
            v = blocks_dict[b][k]
            if isinstance(v, list):
                bash_config['CONFIG_' + k] = '__array__'
                for i, vi in enumerate(v):
                    bash_config['CONFIG_{0}_{1}'.format(k, i)] = vi
            else:
                bash_config['CONFIG_' + k] = v
    eval_str = ''
    for k, v in bash_config.items():
        eval_str += k + '="' + v.replace('"', '\\"') + '"\n'
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


def get_config_dict():
    """Returns the entire dictionary that holds parsed configuration"""
    return __parsed_config


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
        if and_logic:
            result = (b in __parsed_config) and result
        else:
            result = (b in __parsed_config) or result
    return result


def yield_modified_conf(refblock, refoption, newvalue, conf_f=__def_path_arcconf):
    sferblock = '[{0}]'.format(refblock)
    with open(conf_f, 'rt') as refconf:
        in_correct_block = False
        inserted = False
        for confline in refconf:
            sconfline = confline.strip()
            if not in_correct_block:
                if sconfline == sferblock:
                    in_correct_block = True
            else:
                is_nextblock = sconfline.startswith('[')
                if sconfline.startswith(refoption) or is_nextblock:
                    if not inserted:
                        for v in newvalue:
                            yield '{0}={1}\n'.format(refoption, v)
                        if is_nextblock:
                            yield '\n'
                        inserted = True
                    if not is_nextblock:
                        continue
            yield confline
