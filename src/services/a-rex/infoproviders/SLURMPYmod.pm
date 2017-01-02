package SLURMPYmod;

use strict;
use POSIX qw(ceil floor);

use Inline Python => <<"END_OF_PYTHON_CODE";
from arc.lrms.slurm import get_lrms_options_schema, get_lrms_info
END_OF_PYTHON_CODE

our @ISA = ('Exporter');

# Module implements these subroutines for the LRMS interface
our @EXPORT_OK = ('get_lrms_info', 'get_lrms_options_schema');

Inline->init();

1;
