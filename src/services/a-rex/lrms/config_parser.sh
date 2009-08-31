##############################################

# Configuration parser module.
# Requires a POSIX shell and perl

##############################################
#
# Synopsis:
#
#   basename=/usr/local/libexec/arc
#   . config_parser.sh
#
#   config_parse_file /etc/arex.xml || exit 1
#   config_import_section common
#   config_import_section grid-manager
#   config_import_section infosys
#   
#   set | grep CONFIG_
#
#   config_match_section queue/short || echo No such queue
#
#   for name in config_subsections queue; do
#       echo Found section: queue/$name
#   fi
#
##############################################

config_parse_default() {
    if [ -z "$ARC_XML_CONFIG" ]; then echo 'ARC_XML_CONFIG is not set' 1>&2; return 1; fi
    config_parse_file "$ARC_XML_CONFIG"
}

#
# Parse the config file given as an argument
#
config_parse_file() {
    arex_conf=$1
    if [ -z "$arex_conf" ]; then
        echo 'config_parser: No config file given!' 1>&2
        return 1
    elif [ ! -r "$arex_conf" ]; then
        echo "config_parser: Cannot read config file: $arex_conf" 1>&2
        return 1
    fi
    if [ -z "$basename" ]; then echo "basename must be set" 1>&2; return 1; fi
    config_printer="$basename/printConfigForShells.pl"
    if [ ! -f "$config_printer" ]; then echo 'Cannot find $config_printer' 1>&2; return 1; fi

    config=`$config_printer "$arex_conf"` || return $?
    eval "$config" || return $?
    unset config
    return 0
}

#
# Imports a section of the config file into shell variables.
# Option names from will be prefixed with CONFIG_
#
config_import_section() {
  block=$1
  i=0
  if [ -z "$_CONFIG_NUM_BLOCKS" ]; then return 1; fi
  while [ $i -lt $_CONFIG_NUM_BLOCKS ]; do
    i=$(($i+1))
    eval name="\$_CONFIG_BLOCK${i}_NAME"
    if [ "x$block" != "x$name" ]; then continue; fi
    eval num="\$_CONFIG_BLOCK${i}_NUM"
    if [ -z "$num" ]; then return 1; fi
    j=0
    while [ $j -lt $num ]; do
      j=$(($j+1))
      eval name="\$_CONFIG_BLOCK${i}_OPT${j}_NAME"
      if [ -z "$name" ]; then return 1; fi
      eval "CONFIG_$name=\$_CONFIG_BLOCK${i}_OPT${j}_VALUE"
    done
    return 0
  done
  return 1
}

config_match_section() {
  block=$1
  i=0
  if [ -z "$_CONFIG_NUM_BLOCKS" ]; then return 1; fi
  while [ $i -lt $_CONFIG_NUM_BLOCKS ]; do
    i=$(($i+1))
    eval name="\$_CONFIG_BLOCK${i}_NAME"
    if [ "x$block" = "x$name" ]; then return 0; fi
  done
  return 1
}

config_subsections() {
  block=$1
  i=0
  if [ -z "$_CONFIG_NUM_BLOCKS" ]; then return 1; fi
  {
    while [ $i -lt $_CONFIG_NUM_BLOCKS ]; do
      i=$(($i+1))
      eval name="\$_CONFIG_BLOCK${i}_NAME"
      tail=${name#$block/}
      if [ "x$name" != "x$tail" ]; then echo ${tail%%/*}; fi
    done
  } | sort -u
}

config_hide_all() {
    unset `set|cut -f1 -d=|grep '^CONFIG_[A-Za-z0-9_]*$'`
}

config_reset() {
    config_hide_all
    unset `set|cut -f1 -d=|grep '^_CONFIG_[A-Za-z0-9_]*$'`
}

config_destroy() {
    config_reset
    unset config_parse_file
    unset config_import_section
    unset config_match_section
    unset config_subsections
    unset config_hide_all
    unset config_reset
    unset config_destroy
}


