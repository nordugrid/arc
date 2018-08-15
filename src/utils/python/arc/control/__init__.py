from __future__ import absolute_import

from .RunTimeEnvironment import RTEControl
from .Jobs import JobsControl
from .ThirdPartyDeployment import ThirdPartyControl
from .Services import ServicesControl
from .Accounting import AccountingControl

CTL_COMPONENTS = [RTEControl, JobsControl, ServicesControl, ThirdPartyControl, AccountingControl]
