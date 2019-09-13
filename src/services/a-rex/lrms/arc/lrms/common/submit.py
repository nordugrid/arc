"""
Classes and functions to setup job submission.
"""

from __future__ import absolute_import

import os, re
import arc
from .config import Config
from .proc import execute_local, execute_remote
from .files import write
from .log import *


def validate_attributes(jd):
     """
     Checks if GRID (global) job ID and executable are set in job description
     and that runtime environment is resolved. For none-shared filesystems, 
     the scratchdir must also be specified.

     :param jd: job description object
     :type jd: :py:class:`arc.JobDescription`
     """

     if 'joboption;gridid' not in jd.OtherAttributes:
          raise ArcError('Grid ID not specified', 'common.submit')

     if not 'joboption;directory' in jd.OtherAttributes:
          jd.OtherAttributes['joboption;directory'] = \
              os.path.join(Config.sessiondir, jd.OtherAttributes['joboption;gridid'])
     if Config.remote_sessiondir:
          job_sessiondir = os.path.join(Config.remote_sessiondir, jd.OtherAttributes['joboption;gridid'])
          jd.Application.Input = jd.Application.Input.replace(jd.OtherAttributes['joboption;directory'], job_sessiondir)
          jd.Application.Output = jd.Application.Output.replace(jd.OtherAttributes['joboption;directory'], job_sessiondir)
          jd.Application.Error = jd.Application.Error.replace(jd.OtherAttributes['joboption;directory'], job_sessiondir)
          jd.OtherAttributes['joboption;directory'] = job_sessiondir


     if not jd.Application.Executable.Path:
          raise ArcError('Executable is not specified', 'common.submit')
     if not jd.Resources.RunTimeEnvironment.isResolved():
          raise ArcError('Run-time Environment not satisfied', 'common.submit')
     if not Config.shared_filesystem and not Config.scratchdir:
          raise ArcError('Need to know at which directory to run job: '
                         'RUNTIME_LOCAL_SCRATCH_DIR must be set if '
                         'RUNTIME_NODE_SEES_FRONTEND is empty', 'common.submit')


def set_grid_global_jobid(jd):
     has_globalid = False
     for env_pair in jd.Application.Environment:
          if env_pair[0] == 'GRID_GLOBAL_JOBID':
               has_globalid = True
               break
     if not has_globalid:
          globalid = 'gsiftp://%s:%s%s/%s' % (Config.hostname, Config.gm_port, Config.gm_mount_point,
                                              jd.OtherAttributes['joboption;gridid'])
          jd.Application.Environment.append(('GRID_GLOBAL_JOBID', globalid))


def write_script_file(jobscript):
     """
     Write job script to a temporary file.

     :param str jobscript: job script buffer
     :return: path to file
     :rtype: :py:obj:`str`
     """

     import stat, tempfile

     mode = stat.S_IXUSR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXGRP | stat.S_IRGRP | stat.S_IXOTH | stat.S_IROTH
     path = tempfile.mktemp('.sh', 'job')

     if not write(path, jobscript, mode, False, Config.remote_host):
          raise ArcError('Failed to write jobscript', 'common.submit')

     return path


def set_req_mem(jd):
   """
   Resolve memory requirement in job description.

   :param jd: job description object
   :type jd: :py:class:`arc.JobDescription`
   """

   if jd.Resources.IndividualPhysicalMemory.max <= 0:
       defaultmemory = 0
       if (jd.Resources.QueueName in Config.queue
           and hasattr(Config.queue[jd.Resources.QueueName], 'defaultmemory')):
            defaultmemory = Config.queue[jd.Resources.QueueName].defaultmemory
       if defaultmemory == 0 and Config.defaultmemory >= 0:
            defaultmemory = Config.defaultmemory

       debug('-'*69, 'common.submit')
       debug('WARNING: The job description contains no explicit memory requirement.', 'common.submit')
       if defaultmemory > 0:
           jd.Resources.IndividualPhysicalMemory.max = defaultmemory
           debug('         A default memory limit taken from \'defaultmemory\' in', 'common.submit')
           debug('         arc.conf will apply.', 'common.submit')
           debug('         Limit is: %s mb.' % (defaultmemory), 'common.submit')
       else:
           debug('         No \'defaultmemory\' enforcement in in arc.conf.', 'common.submit')
           debug('         JOB WILL BE PASSED TO BATCH SYSTEM WITHOUT MEMORY LIMIT !!!', 'common.submit')
       debug('-'*69, 'common.submit')


def get_rte_path(sw):
    rte_path = None
    # os.path.exists return False for broken symlinks (handled automatically)
    if os.path.exists('%s/rte/enabled/%s' % (Config.controldir, sw)):
        rte_path = '%s/rte/enabled/%s' % (Config.controldir, sw)
    elif os.path.exists('%s/rte/default/%s' % (Config.controldir, sw)):
        rte_path = '%s/rte/default/%s' % (Config.controldir, sw)
    else:
        warn('Requested RunTimeEnvironment %s is missing, broken or not enabled.' % (sw), 'common.submit')
    return rte_path


def get_rte_params_path(sw):
    rte_params_path = None
    if os.path.exists('%s/rte/params/%s' % (Config.controldir, sw)):
        rte_params_path = '%s/rte/params/%s' % (Config.controldir, sw)
    return rte_params_path


# include optional params file to the RTE file content
def get_rte_content(sw):
    rte_path = get_rte_path(sw)
    rte_params_path = get_rte_params_path(sw)
    rte_content = ''
    if rte_params_path:
        try:
            with open(rte_params_path, 'r') as f:
                rte_content += f.read()
        except IOError:
            pass
    try:
        with open(rte_path, 'r') as f:
            rte_content += f.read()
    except IOError:
        pass
    return rte_content


# Limitation: In RTE stage 0, scripts MUST use the 'export' command if any
# new variables are defined which should be picked up by the submit script.
# Also no output from the RTE script in stage 0 will be printed.
def RTE_stage0(jobdesc, lrms, **mapping):
   """
   Source RTE scripts and update job description environment.

   :param jd: job description object
   :type jd: :py:class:`arc.JobDescription`
   :param dict mapping: mapping between additional environment variables and job description
   :todo: is all this sh-to-py back and forth nescessary?
   """

   if not jobdesc.Resources.RunTimeEnvironment.empty():
        from .parse import RTE0EnvCreator
        envCreator = RTE0EnvCreator(jobdesc, Config, mapping)
        stage0_environ = envCreator.getShEnv()
        stage0_environ['joboption_lrms'] = lrms

        # Source RTE script and update the RTE stage0 dict
        def source_sw(sw, opts):
            rte_path = get_rte_path(sw)
            if not rte_path:
                return

            args = 'sourcewithargs () { script=$1; shift; . $script;}; sourcewithargs %s 0' % (rte_path)
            for opt in opts:
                args += ' "%s"' % (opt.replace('"', '\\"'))
            args += ' > /dev/null 2>&1 && env -0'
            handle = execute_local(args, env = stage0_environ, zerobyte = True)
            stdout = handle.stdout
            if handle.returncode != 0:
                 raise ArcError('Runtime script %s failed' % sw, 'common.submit')
            # construct new env dictionary
            new_env = dict((k, v.rstrip('\n')) for k, v in (l.split('=', 1) for l in stdout if l and l != '\n'))
            # update environment
            stage0_environ.update(new_env)

        # Source RTE scripts from the software list
        sw_list = []
        for sw in jobdesc.Resources.RunTimeEnvironment.getSoftwareList():
            sw_list.append(str(sw))
            source_sw(sw, sw.getOptions())

        # Source new RTE scripts set by the previous step (if any)
        rte_environ = dict((k,v) for k,v in stage0_environ.items() if re.match(r'joboption_runtime_\d+', k))
        rte_environ_opts = dict((k,v) for k,v in stage0_environ.items() if re.match(r'joboption_runtime_\d+_\d+', k))
        while len(rte_environ) > len(sw_list):
           for rte, sw in rte_environ.items():
              try:
                 i = re.match(r'joboption_runtime_(\d+)', rte).groups()[0]
                 if sw not in sw_list:
                    opts = []
                    for rte_, opt in rte_environ_opts.items():
                       try:
                          j = re.match(rte + r'_(\d+)', rte_).groups()[0]
                          opts.append(opt)
                       except:
                          pass
                    source_sw(sw, opts)
                    sw_list.append(sw)
              except:
                 pass
           rte_environ = dict((k,v) for k,v in stage0_environ.items() if re.match(r'joboption_runtime_\d+', k))
           rte_environ_opts = dict((k,v) for k,v in stage0_environ.items() if re.match(r'joboption_runtime_\d+_\d+', k))
        # Update jobdesc
        envCreator.setPyEnv(stage0_environ)
        if "RUNTIME_ENABLE_MULTICORE_SCRATCH" in stage0_environ:
             os.environ["RUNTIME_ENABLE_MULTICORE_SCRATCH"] = ""

   # joboption_count might have been changed by an RTE.
   # Save it for accouting purposes.
   if jobdesc.Resources.SlotRequirement.NumberOfSlots > 0:
       try:
           diagfilename = '%s/job.%s.diag' % (Config.controldir, jobdesc.OtherAttributes['joboption;gridid'])
           with open(diagfilename, 'w+') as diagfile:
               diagfile.write('Processors=%s\n' % jobdesc.Resources.SlotRequirement.NumberOfSlots)
       except IOError:
           debug('Unable to write to diag file (%s)' % diagfilename)


class JobscriptAssembler(object):
     """
     Parses and expands jobscript stubs defined in ``job_script.stubs``.
     Each stub is initiated by a line starting with ``>>``, followed by
     the stub name. The stub must be enclosed within two ``>`` on
     separate lines::

       >> my_export_stub
       >
       export FOO=bar
       export BAR=baz
       >

     To expand variables, use the Python format syntax ``%(LOCAL)s``::

       >> set_sessiondir
       >
       export RUNTIME_SESSIONDIR=%(SESSIONDIR)s
       >

     This will evaluate to the session directory specified in the 
     job description::

       export RUNTIME_SESSIONDIR=/scratch/grid/123456

     It is also possible to define loops that depend on some iterable 
     in the job description object. The attribute Application.Environment 
     is an iterable, where each item is a tuple of two elements, an 
     environment variable and its respective value. This iterable is mapped
     to ``ENVS``, and the macros that return the first and second tuple element
     are mapped to ``ENV`` and ``ENV_VAL``, respectively.

     Loops must be enclosed within two ``@`` on separate lines. The top ``@``
     must be followed by the map names of the iterable and the macros. The
     simplest macro is mapped to ``ITEM`` and simply returns the current 
     item in the iterable. This is only applicable when each item is a 
     string. Note the ``%`` in front of each macro name in the following example::

        >> some_stuff
        >
        do_something
        ...
        @ ENVS %ENV %ENV_VAL
        export %(ENV)s=%(ENV_VAL)s
        @
        >

     This will expand to a set of export lines for every environment 
     variable represented in Application.Environment.

     The format variable, iterable and macro mapping is defined in the 
     ``map`` attribute.
     """

     def get_standard_jobscript(self):
          """
          Assemble a generic, out-of-the-box jobscript. This does
          not contain the batch system-specific instructions you
          typically find at the top of the jobscript.

          :return: jobscript
          :rtype: :py:obj:`str`
          """

          script  = self.get_stub('umask_and_sourcewithargs')
          script += self.get_stub('user_env')
          script += self.get_stub('runtime_env')
          script += self.get_stub('move_files_to_node')
          script += "\nRESULT=0\n\n"
          script += "if [ \"$RESULT\" = '0' ] ; then\n"
          script += self.get_stub('rte_stage1')
          script += "if [ \"$RESULT\" = '0' ] ; then\n"
          script += self.get_stub('cd_and_run')
          script += "fi\nfi\n"
          script += self.get_stub('rte_stage2')
          script += self.get_stub('clean_scratchdir')
          script += self.get_stub('move_files_to_frontend')
          return script

     def get_stub(self, stub):
          """
          Get expanded jobscript stub.

          :param str stub: name of jobscript stub
          :return: expanded jobscript stub
          :rtype: :py:obj:`str`
          """
          return self._stubs.get(stub, '')

     def __init__(self, jobdesc):
          self.jobdesc = jobdesc
          self._stubs = {}
          self._ignore = []
          # String format map
          self.map = {
               'i'                    : '', # Reserved for getting loop index.
               # Simple strings
               'GRIDID'               : jobdesc.OtherAttributes['joboption;gridid'],
               'SESSIONDIR'           : jobdesc.OtherAttributes['joboption;directory'],
               'STDIN'                : jobdesc.Application.Input,
               'STDOUT'               : jobdesc.Application.Output,
               'STDERR'               : jobdesc.Application.Error,
               'STDIN_REDIR'          : '<$RUNTIME_JOB_STDIN' if jobdesc.Application.Input else '',
               'STDOUT_REDIR'         : '1>$RUNTIME_JOB_STDOUT' if jobdesc.Application.Output else '',
               'STDERR_REDIR'         : '2>$RUNTIME_JOB_STDERR' if jobdesc.Application.Error and \
                                        jobdesc.Application.Output != jobdesc.Application.Error else '2>&1',
               'EXEC'                 : jobdesc.Application.Executable.Path,
               'ARGS'                 : '"' + '" "'.join(list(jobdesc.Application.Executable.Argument)) + '"' \
                                        if jobdesc.Application.Executable.Argument else "",
               'PROCESSORS'           : jobdesc.Resources.SlotRequirement.NumberOfSlots,
               'NODENAME'             : Config.nodename,
               'GNU_TIME'             : Config.gnu_time,
               'GM_MOUNTPOINT'        : Config.gm_mount_point,
               'GM_PORT'              : Config.gm_port,
               'GM_HOST'              : Config.hostname,
               'LOCAL_SCRATCHDIR'     : Config.scratchdir,
               'RUNTIMEDIR'           : Config.remote_runtimedir if Config.remote_runtimedir else Config.runtimedir,
               'SHARED_SCRATCHDIR'    : Config.shared_scratch,
               'IS_SHARED_FILESYSTEM' : 'yes' if Config.shared_filesystem else '',
               'ARC_LOCATION'         : arc.common.ArcLocation.Get(),
               'ARC_TOOLSDIR'         : arc.common.ArcLocation.GetToolsDir(),
               'GLOBUS_LOCATION'      : os.environ.get('GLOBUS_LOCATION', ''),
               # Iterables and associated macros
               'ENVS'                 : jobdesc.Application.Environment, # Iterable
               'ENV'                  : lambda item: item[0], # Macro
               'ENV_VAL'              : lambda item: item[1], # Macro

               'RTES'                 : jobdesc.Resources.RunTimeEnvironment.getSoftwareList(), # Iterable
               'RTE'                  : lambda item: str(item), # Macro
               'OPTS'                 : lambda item: ' '.join(['"' + opt.replace('"', '\\"') + '"' for opt in item.getOptions()]), # Macro
               'RTE_CONTENT'          : lambda item: get_rte_content(item), # Macro
               # To be setup in a later method
               'OUTPUT_LISTS'         : '', # Setup later
               'OUTPUT_FILES'         : '', # Setup later
               'ITEM'                 : lambda item: item # Generic macro
               }

          self._setup_cleaning()
          if not Config.shared_filesystem:
               self._setup_runtime_env()
          self._parse()

     def __getitem__(self, item):
          return self.map.get(item, '')

     def __setitem__(self, item, value):
          self.map[item] = value

     def _parse(self):

          PARSE = 0
          SKIP  = 1
          READ  = 2
          LOOP  = 4

          do = PARSE
          stub_name = ''
          stub = ''
          loop = ''
          _iterable = None
          _macros = None
          _locals = dict(item for item in self.map.items())

          with open(os.path.join(arc.common.ArcLocation_GetDataDir(), 'job_script.stubs'), 'r') as stubs:
               for num, line in enumerate(stubs):
                    try:
                         if do & READ:
                              # END of stub
                              #
                              # Save to stubs map
                              if line[0] == '>':
                                   if stub_name not in self._ignore:
                                        self._stubs[stub_name] = stub
                                   do = PARSE

                              # Encountered conditional block (only include if key is True (! False))
                              elif line[0:2] == '@@' or do & SKIP:
                                  if line[0:2] == '@@' and do & SKIP:
                                      do ^= SKIP
                                  elif line[0:2] == '@@':
                                      key = line[2:].split()
                                      if key:
                                          do_skip = bool(key[0][0] != '!')
                                          key = self.map[key[0].lstrip('!')]
                                          if bool(len(key) == 0) == do_skip:
                                              do |= SKIP

                              # Encountered loop tag
                              elif line[0] == '@':

                                   # END of loop stub
                                   #
                                   # Loop over the iterable, and for each item:
                                   #   run the macro(s) to resolve the string format variable(s)
                                   #   expand the loop stub
                                   #   add expanded loop stub to stub
                                   if do & LOOP:
                                        i = 0
                                        for item in _iterable:
                                             for key in _macros:
                                                  _locals[key] = self.map[key](item)
                                             _locals['i'] = i
                                             stub += loop % _locals
                                             i += 1
                                        do ^= LOOP

                                   # BEGIN loop stub
                                   #
                                   # Get the iterable and macros to resolve the string format variables
                                   else:
                                        loop = ''
                                        keys = line[1:].split()
                                        key = keys[0]
                                        _iterable = self.map[key]
                                        _macros = []
                                        for key in keys[1:]:
                                             assert(key[0] == '%')
                                             _macros.append(key[1:])
                                        do |= LOOP

                              # Inside loop. Add line to loop stub. Expand later
                              elif do & LOOP:
                                   loop += line
                              # Expand line and add to stub
                              else:
                                   stub += line % self.map

                         elif do & SKIP:
                              # BEGIN stub
                              #
                              # Everything after the '>' tag is read into the stub string, inlcuding empty lines
                              stub = ''
                              if line[0] == '>':
                                   do = READ

                         elif line[:2] == '>>':
                              # Signals that a new stub is coming up
                              stub_name = line[2:].strip()
                              stub = ''
                              do = SKIP

                    except ValueError:
                         raise ArcError('Incomplete format key. Maybe a missing \'s\' after \'%%( ... )\'',
                                        'common.submit.JobscriptAssembler')
                    except AssertionError:
                         raise ArcError('Syntax error at line %i in job_script.stubs. Missing \'%%\'.'
                                        % num, 'common.submit.JobscriptAssembler')
                    except KeyError as e:
                         raise ArcError('Unknown key %s at line %i in job_script.stubs.'
                                        % (str(e), num), 'common.submit.JobscriptAssembler')


     def _setup_cleaning(self):
          """
          Setup trash cleaning. No reason to keep trash until gm-kick runs.
          """

          output = Config.controldir + '/job.' + self['GRIDID'] + '.output'
          try:
               with open(output, 'r') as outputfile:
                    self['OUTPUT_LISTS'] = []
                    self['OUTPUT_FILES'] = []
                    for name in outputfile:
                         name = (
                              # Replace escaped back-slashes (\).
                              # Note: Backslash escapes in 2. argument
                              # are processed. Pattern explained: First
                              # look for escaped backslashes (8x\), then
                              # look for other escaped chars (4x\).
                              re.sub(r'\\\\\\\\|\\\\', '\\\\',
                                     # Remove possible remote destination (URL).
                                     # Take spaces and escaping into account.
                                     # URL is the first string which is preceded by
                                     # a space which is not escaped.  
                                     re.sub(r'([^\\](\\\\)*) .*', '\\1', name.strip()).
                                     # Replace escaped spaces and single quotes
                                     replace('\\ ', ' ').replace("'", "'\\''").
                                     # Strip leading slashes (/).
                                     lstrip('/'))
                              )
                         if name[0] == '@':
                              self['OUTPUT_LISTS'].append(name[1:])
                         else:
                              self['OUTPUT_FILES'].append(name)

          except IOError: # Skip cleaning, else output files will be deleted
               self._ignore.append('clean_scratchdir')
               verbose('Failed to read job output file (%s)' % output, 'common.submit')


     def _setup_runtime_env(self):
          """
          For non-shared filesystems, replace sessiondir with local scratchdir
          in the stdin, stdout and stderr paths.
          """

          sdir = self['SESSIONDIR']
          for f in ('STDIN', 'STDOUT', 'STDERR', 'SESSIONDIR'):
               if self[f].startswith(sdir):
                    self[f] = os.path.join(self['LOCAL_SCRATCHDIR'], self['GRIDID'], self[f][len(sdir):].lstrip('/')).rstrip('/')
