#!/usr/bin/python2

import argparse
from arc.config import *

# command line arguments parsing
parser = argparse.ArgumentParser(description='Nordugrid ARC configuration parser')
parser.add_argument('-c', '--config', action='store',
                    help='config file location (default is /etc/arc.conf)', default='/etc/arc.conf')
parser.add_argument('-b', '--block', action='append',
                    help='block name (can be specified several times)')
parser.add_argument('-o', '--option', action='store', help='option name')
parser.add_argument('-s', '--subblocks', action='store_true',
                    help='also match subblocks against supplied block name(s)')
parser.add_argument('-e', '--export', action='store', choices=['bash', 'json'],
                    help='export configuration to the defined format')
cmd_args = parser.parse_args()

# parse the entire config
parse_arc_conf(cmd_args.config)
if cmd_args.export:
    # export data
    if cmd_args.export == 'json':
        print export_json(cmd_args.block, cmd_args.subblocks)
    elif cmd_args.export == 'bash':
        print export_bash(cmd_args.block, cmd_args.subblocks)
else:
    # get value
    if cmd_args.option and cmd_args.block:
        values = get_value(cmd_args.option, cmd_args.block)
        if isinstance(values, list):
            for v in values:
                print v
        else:
            print values
    else:
        parser.print_help()
