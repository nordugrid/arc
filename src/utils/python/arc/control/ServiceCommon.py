import logging
from arc.paths import *

logger = logging.getLogger('ARCCTL.ServiceCommon')


# Runtime configuration (used for root only)
arcctl_runtime_config = ARC_RUN_DIR + '/arcctl.runtime.conf'
if os.geteuid() != 0:
    arcctl_runtime_config = None


def arcctl_server_mode():
    """Just return True indicating that we are working in server mode with arc.conf in place"""
    # TODO: non-root ARC - use arc.conf user instead of hardcoded root
    if os.geteuid() != 0:
        logger.warning('Running arcctl on ARC CE from non-root user. Disabling the service mode.')
        return False
    return True


def remove_runtime_config():
    """Remove ARC runtime configuration if defined"""
    if arcctl_runtime_config is not None:
        if os.path.exists(arcctl_runtime_config):
            os.unlink(arcctl_runtime_config)
