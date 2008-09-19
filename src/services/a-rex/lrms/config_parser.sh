##############################################

# Configuration parser module.
# Requires a POSIX shell and perl

##############################################
#
# Synopsis:
#
#   . $ARC_LOCATION/libexec/config_parser
#
#   config_parse_file /etc/arc.conf || exit 1
#   config_import_section common
#   config_import_section grid-manager
#   config_import_section infosys
#   
#   env | grep CONFIG_
#
#   config_match_section queue/short || echo No such queue
#
#   for name in config_subsections queue; do
#       echo Found section: queue/$name
#   fi
#
##############################################

#
# Parse the config file given as an argument
#
config_parse_file() {
    arc_conf=$1
    if [ -z "$arc_conf" ]; then
        echo 'config_parser: No config file given!' 1>&2
        return 1
    elif [ ! -r "$arc_conf" ]; then
        echo "config_parser: Cannot read config file: $arc_conf" 1>&2
        return 1
    fi
    script='my ($nb,$no,$bn)=(0,0,""); while(<>) { chomp;
              if (/^\s*\[([\w\-\.\/]+)\]\s*$/) {
                print "_CONFIG_BLOCK${nb}_NAME='\''$bn'\'';\n" if $nb;
                print "_CONFIG_BLOCK${nb}_NUM='\''$no'\'';\n" if $nb;
                $nb++; $no=0; $bn=$1;
              } elsif (/^(\w+)\s*=\s*([\"'\''])(.*)(\2)\s*$/) { $no++;
                my ($opt,$val)=($1,$3); $val=~s/'\''/'\''\\'\'''\''/g;
                $bn =~ s|^(.+?)(/[^/]*)?$|$1/$val| if $opt eq "name";
                print "_CONFIG_BLOCK${nb}_OPT${no}_NAME='\''$opt'\'';\n";
                print "_CONFIG_BLOCK${nb}_OPT${no}_VALUE='\''$val'\'';\n";
              } elsif (/^\s*#/) { # skip comment line
              } elsif (/^\s*$/) { # skip empty line
              } else { print "echo config_parser: Skipping bad input at line number $. 1>&2\n";
            } }
            print "_CONFIG_BLOCK${nb}_NAME='\''$bn'\'';\n";
            print "_CONFIG_BLOCK${nb}_NUM='\''$no'\'';\n";
            print "_CONFIG_NUM_BLOCKS='\''$nb'\'';\n";
           '
    config=`cat $arc_conf | perl -w -e "$script"` || return $?
    eval "$config" || return $?
    unset config
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
      eval value="\$_CONFIG_BLOCK${i}_OPT${j}_VALUE"
      if [ -z "$name" ]; then return 1; fi
      eval export "CONFIG_$name='$value'"
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
  while [ $i -lt $_CONFIG_NUM_BLOCKS ]; do
    i=$(($i+1))
    eval name="\$_CONFIG_BLOCK${i}_NAME"
    # skip the parent section itself
    if [ "x$block" = "x$name" ]; then continue; fi
    head=${name%%/*}
    tail=${name#*/}
    if [ "x$block" = "x$head" ]; then echo $tail; fi
  done
}

config_hide_all() {
    unset `set|grep ^CONFIG_|cut -f1 -d=`
}


