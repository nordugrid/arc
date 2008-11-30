##############################################

# Configuration parser module.
# Requires a POSIX shell and perl

##############################################
#
# Synopsis:
#
#   . config_parser.sh
#
#   config_parse_file /etc/arc.conf || exit 1
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
    script='my ($nb,$bn)=(0,0,""); my %opts=(); while(<>) { chomp;
              if (/^\s*\[([\w\-\.\/]+)\]\s*$/) {
                print_section() if $nb; $nb++; $bn=$1;
              } elsif (/^(\w+)\s*=\s*([\"'\'']?)(.*)(\2)\s*$/) {
                my ($opt,$val)=($1,$3); $val=~s/'\''/'\''\\'\'''\''/g;
                $bn =~ s|^(.+?)(/[^/]*)?$|$1/$val| if $opt eq "name";
                unshift @{$opts{$opt}}, $val;
              } elsif (/^\s*#/) { # skip comment line
              } elsif (/^\s*$/) { # skip empty line
              } elsif (/^\s*all\s*$/) { # make an exception for "all" command
              } else { print "echo config_parser: Skipping malformed line in section \\\[$bn\\\] at line number $. 1>&2\n";
            } }
            print_section(); print "_CONFIG_NUM_BLOCKS='\''$nb'\'';\n";
            sub print_section { my $no=0; while (my ($opt,$val)=each %opts) { $no++;
                print "_CONFIG_BLOCK${nb}_OPT${no}_NAME='\''$opt'\'';\n";
                print "_CONFIG_BLOCK${nb}_OPT${no}_VALUE='\''$val->[@$val-1]'\'';\n";
                if (@$val > 1) { for $i (1 .. @$val) { $no++; # multi-valued option
                    print "_CONFIG_BLOCK${nb}_OPT${no}_NAME='\''${opt}_$i'\'';\n";
                    print "_CONFIG_BLOCK${nb}_OPT${no}_VALUE='\''$val->[$i-1]'\'';\n";
              } } }; %opts=();
              print "_CONFIG_BLOCK${nb}_NAME='\''$bn'\'';\n";
              print "_CONFIG_BLOCK${nb}_NUM='\''$no'\'';\n";
            }
           '
    config=`cat $arc_conf | perl -w -e "$script"` || return $?
    unset script
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
      eval value="\$_CONFIG_BLOCK${i}_OPT${j}_VALUE"
      if [ -z "$name" ]; then return 1; fi
      eval "CONFIG_$name='$value'"
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


