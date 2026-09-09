# Set the Grid Engine environment and command paths used by all SGE backends.
#

# Conditionaly enable performance logging
init_perflog

# Keep tracing on stderr: stdout is the submit/cancel protocol. Do not dump
# scripts, the GRAMi file or the environment, which can contain credentials.
sge_log () {
    printf '[%s] %s[%s]: arc_job=%s sge_job=%s %s\n' \
        "$(date -u '+%Y-%m-%dT%H:%M:%SZ')" "${0##*/}" "$$" \
        "${gridid:-${joboption_gridid:--}}" "${id:-${joboption_jobid:--}}" "$*" >&2
}

sge_debug () {
    [ "${CONFIG_sge_debug:-no}" = yes ] || return 0
    sge_log "DEBUG $*"
}

case ${CONFIG_sge_debug:-no} in
    yes|no) ;;
    *) echo 'sge_debug must be yes or no' >&2; return 1 ;;
esac

##############################################################
# Initialize SGE environment variables
##############################################################

SGE_ROOT=${CONFIG_sge_root:-$SGE_ROOT}

if [ -z "$SGE_ROOT" ]; then
    echo 'SGE_ROOT not set' 1>&2
    return 1
fi

SGE_CELL=${SGE_CELL:-default}
SGE_CELL=${CONFIG_sge_cell:-$SGE_CELL}
export SGE_ROOT SGE_CELL

if [ -n "$CONFIG_sge_qmaster_port" ]; then
    SGE_QMASTER_PORT=$CONFIG_sge_qmaster_port
    export SGE_QMASTER_PORT
fi

if [ -n "$CONFIG_sge_execd_port" ]; then
    SGE_EXECD_PORT=$CONFIG_sge_execd_port
    export SGE_EXECD_PORT
fi

# Grid Engine command output contains dates, decimal numbers and, for some
# commands, prose.  A stable locale is required by the accounting parser.
LC_ALL=C
LANG=C
export LC_ALL LANG

##############################################################
# Find path to SGE executables
##############################################################

# 1. use sge_bin_path config option, if set
if [ -n "$CONFIG_sge_bin_path" ]; then
    SGE_BIN_PATH=$CONFIG_sge_bin_path
fi

# 2. otherwise see if qsub can be found in the path
if [ -z "$SGE_BIN_PATH" ]; then
    qsub=$(command -v qsub 2>/dev/null)
    SGE_BIN_PATH=${qsub%/*}
    unset qsub
fi

for sge_command in qsub qstat qacct qdel qconf qhost; do
    if [ ! -x "$SGE_BIN_PATH/$sge_command" ]; then
        echo "SGE executable not found: $SGE_BIN_PATH/$sge_command" 1>&2
        echo 'Check that sge_bin_path points to the Grid Engine binary directory' 1>&2
        unset sge_command
        return 1
    fi
done
unset sge_command

export SGE_BIN_PATH

sge_qsub="$SGE_BIN_PATH/qsub"
sge_qstat="$SGE_BIN_PATH/qstat"
sge_qacct="$SGE_BIN_PATH/qacct"
sge_qdel="$SGE_BIN_PATH/qdel"
sge_qconf="$SGE_BIN_PATH/qconf"
sge_qhost="$SGE_BIN_PATH/qhost"

sge_debug "event=environment root=$SGE_ROOT cell=$SGE_CELL binaries=$SGE_BIN_PATH qmaster_port=${SGE_QMASTER_PORT:-service-default} execd_port=${SGE_EXECD_PORT:-service-default}"

# qstat has no equivalent of qsub -clear. Its global and private default files
# may therefore narrow or reshape machine-readable output. Only selectors ARC
# overrides explicitly are safe; reject everything else rather than mistake an
# incomplete snapshot for completed jobs.
validate_sge_qstat_defaults () {
    /usr/bin/perl - "$SGE_ROOT/$SGE_CELL/common/sge_qstat" <<'PERL'
use strict;
use warnings;
use Text::ParseWords qw(shellwords);

my @files = @ARGV;
my @passwd = getpwuid($<);
push @files, "$passwd[7]/.sge_qstat" if @passwd && defined $passwd[7];

for my $file (@files) {
    next unless defined $file && -r $file;
    open(my $fh, '<', $file) or die "Cannot read qstat defaults $file: $!\n";
    my $number = 0;
    while (my $line = <$fh>) {
        ++$number;
        $line =~ s/#.*//;
        next if $line =~ /^\s*$/;
        my @words = eval { shellwords($line) };
        die "Malformed qstat defaults in $file line $number\n"
            if $@ || !@words;
        tr/'"//d for @words;
        while (@words) {
            my $option = shift @words;
            die "Unsafe qstat default '$option' in $file line $number\n"
                unless $option eq '-u' || $option eq '-s' || $option eq '-q';
            die "Missing value for qstat default '$option' in $file line $number\n"
                unless @words;
            my $value = shift @words;
            die "Invalid value for qstat default '$option' in $file line $number\n"
                if !length($value) || $value =~ /^-/;
        }
    }
    close($fh) or die "Cannot close qstat defaults $file: $!\n";
}
PERL
}

# qsub consumes some options before it reaches the ordered -clear operation.
# Reject defaults which could make ARC block synchronously, create an array or
# recursively import unchecked options.  A
# private qsub working directory prevents a session-local .sge_request from
# participating; these are the remaining global and passwd-home defaults.
validate_sge_qsub_defaults () {
    /usr/bin/perl - "$SGE_ROOT/$SGE_CELL/common/sge_request" <<'PERL'
use strict;
use warnings;
use Text::ParseWords qw(shellwords);

my @files = @ARGV;
my @passwd = getpwuid($<);
push @files, "$passwd[7]/.sge_request" if @passwd && defined $passwd[7];

for my $file (@files) {
    next unless defined $file && -r $file;
    open(my $fh, '<', $file) or die "Cannot read qsub defaults $file: $!\n";
    my $number = 0;
    while (my $line = <$fh>) {
        ++$number;
        $line =~ s/#.*//;
        next if $line =~ /^\s*$/;
        my @words = eval { shellwords($line) };
        die "Malformed qsub defaults in $file line $number\n"
            if $@ || !@words;
        # Grid Engine strips every quote character after its initial split,
        # including quotes embedded inside another quoted token.
        tr/'"//d for @words;
        for (my $i = 0; $i < @words; ++$i) {
            my $option = $words[$i];
            die "Unsafe qsub option-file default '$option' in $file line $number\n"
                if $option =~ /^-@/
                    || $option =~ /^-(?:t|tc|binding)(?:=|$)/;
            if ($option eq '-sync') {
                my $value = $words[$i + 1];
                die "Missing value for qsub default '$option' in $file line $number\n"
                    unless defined $value && $value !~ /^-/;
                die "Unsafe qsub default '$option $value' in $file line $number\n"
                    if $value =~ /^(?:y|yes)$/i;
            } elsif ($option =~ /^-sync=(.*)$/) {
                die "Unsafe qsub default '-sync=$1' in $file line $number\n"
                    if $1 =~ /^(?:y|yes)$/i;
            }
        }
    }
    close($fh) or die "Cannot close qsub defaults $file: $!\n";
}
PERL
}

# ARC submits ordinary (non-array) jobs, whose Grid Engine identifier is an
# unsigned decimal integer.  Validate before passing an identifier as a command
# argument so malformed control files cannot be interpreted as options.
verify_jobid () {
    case $1 in
        ''|*[!0-9]*)
            echo "Invalid SGE job id: ${1:-<empty>}" 1>&2
            return 1
            ;;
    esac
    case $1 in
        *[1-9]*) ;;
        *)
            echo "Invalid SGE job id: $1" 1>&2
            return 1
            ;;
    esac
    return 0
}
