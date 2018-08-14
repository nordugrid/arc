"""
Mapping between sh (and GRAMi files) and Python.
"""

from __future__ import absolute_import

import re, os

import arc
from .log import ArcError

class RTE0EnvCreator(object):
    """
    :todo: parse joboption_pre_#_#, joboption_pre_#_code, joboption_post_#_#, \
    joboption_post_#_code, joboption_penv_type, joboption_penv_version, \
    joboption_penv_processesperhost, joboption_penv_threadsperprocess
    """

    _shToPy = \
        {
        "RUNTIME_FRONTEND_SEES_NODE" : "config.shared_scratch",
        "RUNTIME_NODE_SEES_FRONTEND" : "config.shared_filesystem",
        "RUNTIME_LOCAL_SCRATCH_DIR"  : "config.scratchdir",
        "joboption_directory"        : "config.sessiondir",
        "joboption_controldir"       : "config.controldir",
        "joboption_arg_#"            : "Application.Executable",
        "joboption_env_#"            : "Application.Environment",
        "joboption_stdin"            : "Application.Input",
        "joboption_stdout"           : "Application.Output",
        "joboption_stderr"           : "Application.Error",
        "joboption_starttime"        : "Application.ProcessingStartTime",
        "joboption_priority"         : "Application.Priority",
        "joboption_cputime"          : "Resources.TotalCPUTime.range.max",
        "joboption_walltime"         : "Resources.IndividualWallTime.range.max",
        "joboption_memory"           : "Resources.IndividualPhysicalMemory.max",
        "joboption_virtualmemory"    : "Resources.IndividualVirtualMemory.max",
        "joboption_count"            : "Resources.SlotRequirement.NumberOfSlots",
        "joboption_countpernode"     : "Resources.SlotRequirement.SlotsPerHost",
        "joboption_exclusivenode"    : "Resources.SlotRequirement.ExclusiveExecution",
        "joboption_runtime_#"        : "Resources.RunTimeEnvironment",
        "joboption_jobname"          : "Identification.JobName",
        "joboption_queue"            : "Resources.QueueName",
        "joboption_inputfile_#"      : "DataStaging.InputFiles",
        "joboption_outputfile_#"     : "DataStaging.OutputFiles",
        "joboption_nodeproperty_#"   : "OtherAttributes.joboption;nodeproperty",
        "joboption_rsl_project     " : "OtherAttributes.joboption;rsl_project",
        "joboption_rsl_architecture" : "OtherAttributes.joboption;rsl_architecture",
        "joboption_gridid"           : "OtherAttributes.joboption;gridid"
        }

    def __init__(self, jobdesc, config, mappingUpdate = {}):
        self.jobdesc, self.config = jobdesc, config
        RTE0EnvCreator._shToPy.update(mappingUpdate)

    def getShEnv(self):

        stage0_environ = os.environ
        for shEnv, pyKey in RTE0EnvCreator._shToPy.items():
            obj, key = self._getPyAttr(pyKey)
            if obj:
                obj = getattr(obj, key, None)
                if (type(obj) == int or type(obj) == str):
                    stage0_environ[shEnv] = str(obj)
                elif type(obj) == bool:
                    stage0_environ[shEnv] = "yes" if obj else "no"
                elif type(obj) == arc.common.Time:
                    if not obj.GetTime() == -1:
                        stage0_environ[shEnv] = obj.str(arc.common.MDSTime)
                elif type(obj) == arc.compute.ExecutableType:
                    stage0_environ[shEnv[:-1] + "0"] = obj.Path
                    for i in range(len(obj.Argument)):
                        stage0_environ[shEnv[:-1] + str(i+1)] = obj.Argument[i]
                    stage0_environ[shEnv[:-1] + "code"] = \
                        str(obj.SuccessExitCode.second) \
                        if obj.SuccessExitCode.first else ""
                elif type(obj) == arc.compute.StringPairList:
                    for i in range(len(obj)):
                        stage0_environ[shEnv[:-1] + str(i)] = \
                            "'" + ("%s=%s" % obj[i]) + "'"
                elif type(obj) == arc.compute.SoftwareRequirement:
                    rtes = obj.getSoftwareList()
                    for i in range(len(rtes)):
                        stage0_environ[shEnv[:-1] + str(i)] = str(rtes[i])
                        for j in range(len(rtes[i].getOptions())):
                            stage0_environ[shEnv[:-1] + str(i) + \
                                               "_" + str(j+1)] = \
                                               rtes[i].getOptions()[j]
                elif pyKey == "DataStaging.InputFiles":
                    i = 0
                    for f in self.jobdesc.DataStaging.InputFiles:
                        if not f.Sources.empty():
                            stage0_environ[shEnv[:-1] + str(i)] = \
                                f.Sources.front().str()
                            i += 1
                elif pyKey == "DataStaging.OutputFiles":
                    i = 0
                    for f in self.jobdesc.DataStaging.OutputFiles:
                        if not f.Targets.empty():
                            stage0_environ[shEnv[:-1] + str(i)] = \
                                f.Targets.front().str()
                            i += 1

        return stage0_environ

    def setPyEnv(self, stage0_environ):

        old_inputfiles = \
           arc.compute.InputFileTypeList(self.jobdesc.DataStaging.InputFiles)
        self.jobdesc.DataStaging.InputFiles.clear()
        old_outputfiles = \
           arc.compute.OutputFileTypeList(self.jobdesc.DataStaging.OutputFiles)
        self.jobdesc.DataStaging.OutputFiles.clear()
        for shEnv, pyKey in RTE0EnvCreator._shToPy.items():
            pObj, key = self._getPyAttr(pyKey)
            if pObj is None:
                continue
            obj = getattr(pObj, key, None)
            i = 0
            if type(obj) == arc.compute.ExecutableType:
                obj.Path = stage0_environ.get(shEnv[:-1] + "0", "")
                obj.Path = RTE0EnvCreator._removeQuotes(obj.Path)
                while shEnv[:-1] + str(i+1) in stage0_environ \
                        or i < len(obj.Argument):
                    arg = stage0_environ[shEnv[:-1] + str(i+1)]
                    arg = RTE0EnvCreator._removeQuotes(arg)
                    if len(obj.Argument) == i:
                        obj.Argument.append(arg)
                    else:
                        obj.Argument[i] = arg
                    i += 1
                if shEnv[:-1] + str(i+1) not in stage0_environ \
                        and i < len(obj.Argument):
                    obj.Argument.resize(i)
                if shEnv[:-1] + "code" in stage0_environ:
                    obj.SuccessExitCode.first = \
                        stage0_environ[shEnv[:-1] + "code"].isdigit()
                    if obj.SuccessExitCode.first:
                        obj.SuccessExitCode.second = \
                            int(stage0_environ[shEnv[:-1] + "code"])
            elif type(obj) == arc.compute.StringPairList:
                # shEnv = joboption_env_#
                while shEnv[:-1] + str(i) in stage0_environ or i < len(obj):
                    envPair = stage0_environ[shEnv[:-1] + str(i)]
                    envPair = \
                        RTE0EnvCreator._removeQuotes(envPair).split("=", 1)
                    envPair[1] = RTE0EnvCreator._removeQuotes(envPair[1])
                    if len(obj) == i:
                        obj.append(envPair)
                    else:
                        obj[i] = envPair
                    i += 1
            elif type(obj) == arc.compute.SoftwareRequirement:
                swr = arc.compute.SoftwareRequirement()
                while shEnv[:-1] + str(i) in stage0_environ:
                    sw = arc.compute.Software(RTE0EnvCreator._removeQuotes(stage0_environ[shEnv[:-1] + str(i)]))
                    j = 1
                    while shEnv[:-1] + str(i) + "_" + str(j) in stage0_environ:
                        sw.addOption(RTE0EnvCreator._removeQuotes(stage0_environ[shEnv[:-1] + str(i) + "_" + str(j)]))
                        j += 1
                    swr.add(sw, arc.compute.Software.EQUAL)
                    i += 1
                self.jobdesc.Resources.RunTimeEnvironment = swr
            elif pyKey == "DataStaging.InputFiles":
                while shEnv[:-1] + str(i) in stage0_environ:
                    path = RTE0EnvCreator._removeQuotes(stage0_environ[shEnv[:-1] + str(i)])
                    f = arc.compute.InputFileType()
                    # TODO: Are there any options at the end of value? If so maybe those should be removed.
                    f.Name = path.rsplit("/", 1)[-1]
                    f.Sources.append(arc.compute.SourceType(path))
                    obj.append(f)
                    if not old_inputfiles.empty():
                        old_inputfiles.pop_front()
                    i += 1
            elif pyKey == "DataStaging.OutputFiles":
                while shEnv[:-1] + str(i) in stage0_environ:
                    path = RTE0EnvCreator._removeQuotes(stage0_environ[shEnv[:-1] + str(i)])
                    f = arc.compute.OutputFileType()
                    # TODO: Are there any options at the end of value? If so maybe those should be removed.
                    f.Name = path.rsplit("/", 1)[-1]
                    f.Targets.append(arc.compute.TargetType(path))
                    obj.append(f)
                    if not old_outputfiles.empty():
                        old_outputfiles.pop_front()
                    i += 1
            elif type(pObj) == arc.common.StringStringMap and shEnv[-1] == "#":
                if not obj:
                    pObj[key] = ""
                while shEnv[:-1] + str(i) in stage0_environ:
                    pObj[key] += " " + RTE0EnvCreator._removeQuotes(stage0_environ[shEnv[:-1] + str(i)])
                    i += 1
            elif shEnv in stage0_environ:
                value = RTE0EnvCreator._removeQuotes(stage0_environ[shEnv])
                if type(pObj) == arc.common.StringStringMap:
                    pObj[key] = value
                elif type(obj) == int:
                    setattr(pObj, key, int(value) if value.isdigit() else -1)
                elif type(obj) == bool:
                    setattr(pObj, key, value != "no" and value != "")
                elif type(obj) == str:
                    setattr(pObj, key, value)
                elif type(obj) == arc.common.Time:
                    setattr(pObj, key, arc.common.Time(value if value else -1))

        for f in old_inputfiles:
            self.jobdesc.DataStaging.InputFiles.append(f)
        for f in old_outputfiles:
            self.jobdesc.DataStaging.OutputFiles.append(f)

    def _getPyAttr(self, pyKey):
        if pyKey == "none":
            return (None, None)
        if pyKey[:7] == "config.":
            if hasattr(self.config, pyKey[7:]):
                return (self.config, pyKey[7:])
            return (None, None)
        else:
            obj, keys = self.jobdesc, pyKey.split(".")
            for attr in keys[:-1]:
                if hasattr(obj, attr):
                    obj = getattr(obj, attr)
                else:
                    return (None, None)
            return (obj, keys[-1])

    @staticmethod
    def _removeQuotes(text):
      return re.sub(r"^'(.*)'$", "\\1", text)


class JobDescriptionParserGRAMi(object):
    """
    :todo: Add docstring
    """

    def __init__(self):
        pass

    @staticmethod
    def Parse(source, jobdescs, language = "", dialect = ""):

        mapping = {
                   "RUNTIME_FRONTEND_SEES_NODE" : "none",
                   "RUNTIME_NODE_SEES_FRONTEND" : "none", 
                   "RUNTIME_LOCAL_SCRATCH_DIR"  : "none",
                   "joboption_directory"        : "OtherAttributes.joboption;directory",
                   "joboption_controldir"       : "OtherAttributes.joboption;controldir",
                   }
        j = arc.compute.JobDescription()
        try:
            env = dict( line.strip().replace("\\\\", "\\").split("=", 1) for line in source.splitlines() if not re.search(r"^\s*(#|$)", line) )
        except:
            return False
        RTE0EnvCreator(j, type('JobDescription', (object,), {})(), mapping).setPyEnv(env)
        jobdescs.append(j)
        return True
