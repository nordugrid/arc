#
# Common block for cancel scripts
# must be called with the grami file as argument
# remember to set $joboption_lrms

common_init () {
   # parse grami file
   no_grami_extra_processing=1
   parse_grami_file $GRAMI_FILE
   #  parse configuration
   read_arc_conf
   # read pbs-specific environment
   .  ${pkgdatadir}/configure-${joboption_lrms}-env.sh || exit $?
}

