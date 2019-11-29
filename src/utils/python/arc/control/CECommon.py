import logging
from arc.paths import *

logger = logging.getLogger('ARCCTL.CECommon')


# Runtime configuration (used for root only)
arcctl_runtime_config = ARC_RUN_DIR + '/arcctl.runtime.conf'
if os.geteuid() != 0:
    arcctl_runtime_config = None


def arcctl_ce_mode():
    """Just return True indicating that we are working in ARC CE mode with arc.conf in place"""
    return True


def remove_runtime_config():
    """Remove ARC CE runtime configuration if defined"""
    if arcctl_runtime_config is not None:
        if os.path.exists(arcctl_runtime_config):
            os.unlink(arcctl_runtime_config)
