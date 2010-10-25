#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <fstream>

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>

#include "job_desc.h"
#include "../files/info_files.h"
#include "../misc/canonical_dir.h"


static Arc::Logger& logger = Arc::Logger::getRootLogger();

bool get_arc_job_description(const std::string& fname, Arc::JobDescription& desc) {
  std::string job_desc_str;
  if (!job_description_read_file(fname, job_desc_str)) {
    logger.msg(Arc::ERROR, "Job description file could not be read.");
    return false;
  }

  desc.AddHint("SOURCEDIALECT","GRIDMANAGER");
  return desc.Parse(job_desc_str);
}

bool write_grami(const Arc::JobDescription& arc_job_desc, const JobDescription& job_desc, const JobUser& user, const char* opt_add) {
  if(job_desc.get_local() == NULL) return false;
  const std::string session_dir = job_desc.SessionDir();
  JobLocalDescription& job_local_desc = *(job_desc.get_local());
  const std::string fgrami = user.ControlDir() + "/job." + job_desc.get_id() + ".grami";
  std::ofstream f(fgrami.c_str(),std::ios::out | std::ios::trunc);
  if(!f.is_open()) return false;
  if(!fix_file_permissions(fgrami,job_desc,user)) return false;
  if(!fix_file_owner(fgrami,job_desc,user)) return false;

  f<<"joboption_directory='"<<session_dir<<"'"<<std::endl;

  {
    std::string executable = Arc::trim(arc_job_desc.Application.Executable.Name);
    if (executable[0] != '/' && executable[0] != '$' && !(executable[0] == '.' && executable[1] == '/')) executable = "./"+executable;
    f<<"joboption_arg_0"<<"="<<value_for_shell(executable.c_str(),true)<<std::endl;
    int i = 1;
    for (std::list<std::string>::const_iterator it = arc_job_desc.Application.Executable.Argument.begin();
         it != arc_job_desc.Application.Executable.Argument.end(); it++, i++) {
      f<<"joboption_arg_"<<i<<"="<<value_for_shell(it->c_str(),true)<<std::endl;
    }
  }
  
  f<<"joboption_stdin="<<value_for_shell(arc_job_desc.Application.Input.empty()?NG_RSL_DEFAULT_STDIN:arc_job_desc.Application.Input,true)<<std::endl;

  if (!arc_job_desc.Application.Output.empty()) {
    std::string output = arc_job_desc.Application.Output;
    if (canonical_dir(output) != 0) {
      logger.msg(Arc::ERROR,"Bad name for stdout: %s", output);
      return false;
    }
  }
  f<<"joboption_stdout="<<value_for_shell(arc_job_desc.Application.Output.empty()?NG_RSL_DEFAULT_STDOUT:session_dir+"/"+arc_job_desc.Application.Output,true)<<std::endl;
  if (!arc_job_desc.Application.Error.empty()) {
    std::string error = arc_job_desc.Application.Error;
    if (canonical_dir(error) != 0) {
      logger.msg(Arc::ERROR,"Bad name for stderr: %s", error);
      return false;
    }
  }
  f<<"joboption_stderr="<<value_for_shell(arc_job_desc.Application.Error.empty()?NG_RSL_DEFAULT_STDERR:session_dir+"/"+arc_job_desc.Application.Error,true)<<std::endl;

  {
    int i = 0;
    for (std::list< std::pair<std::string, std::string> >::const_iterator it = arc_job_desc.Application.Environment.begin();
         it != arc_job_desc.Application.Environment.end(); it++, i++) {
        f<<"joboption_env_"<<i<<"="<<value_for_shell(it->first+"="+it->second,true)<<std::endl;
    }
    value_for_shell globalid=value_for_shell(job_local_desc.globalid,true);
    f<<"joboption_env_"<<i<<"=GRID_GLOBAL_JOBID="<<globalid<<std::endl;
  }

  
  f<<"joboption_cputime="<<(arc_job_desc.Resources.TotalCPUTime.range.max != -1 ? Arc::tostring(arc_job_desc.Resources.TotalCPUTime.range.max):"")<<std::endl;
  f<<"joboption_walltime="<<(arc_job_desc.Resources.TotalWallTime.range.max != -1 ? Arc::tostring(arc_job_desc.Resources.TotalWallTime.range.max):"")<<std::endl;
  f<<"joboption_memory="<<(arc_job_desc.Resources.IndividualPhysicalMemory.max != -1 ? Arc::tostring(arc_job_desc.Resources.IndividualPhysicalMemory.max):"")<<std::endl;
  f<<"joboption_count="<<(arc_job_desc.Resources.SlotRequirement.ProcessPerHost.max != -1 ? Arc::tostring(arc_job_desc.Resources.SlotRequirement.ProcessPerHost.max):"1")<<std::endl;

  {
    int i = 0; 
    for (std::list<Arc::Software>::const_iterator itSW = arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().begin();
         itSW != arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++) {
      if (itSW->empty()) continue;
      std::string rte = Arc::upper(*itSW);
      if (canonical_dir(rte) != 0) {
        logger.msg(Arc::ERROR, "Bad name for runtime environment: %s", (std::string)*itSW);
        return false;
      }
      f<<"joboption_runtime_"<<i++<<"="<<value_for_shell((std::string)*itSW,true)<<std::endl;
    }
  }

  f<<"joboption_jobname="<<value_for_shell(job_local_desc.jobname,true)<<std::endl;
  f<<"joboption_queue="<<value_for_shell(job_local_desc.queue,true)<<std::endl;
  f<<"joboption_starttime="<<(job_local_desc.exectime != -1?job_local_desc.exectime.str(Arc::MDSTime):"")<<std::endl;
  f<<"joboption_gridid="<<value_for_shell(job_desc.get_id(),true)<<std::endl;

  // Here we need another 'local' description because some info is not
  // stored in job.#.local and still we do not want to mix both.
  // TODO: clean this. 
  {
    JobLocalDescription stageinfo;
    stageinfo = arc_job_desc;
    int i = 0;
    for(FileData::iterator s=stageinfo.inputdata.begin();
                           s!=stageinfo.inputdata.end(); ++s) {
      f<<"joboption_inputfile_"<<(i++)<<"="<<value_for_shell(s->pfn,true)<<std::endl;
    }
    i = 0;
    for(FileData::iterator s=stageinfo.outputdata.begin();
                           s!=stageinfo.outputdata.end(); ++s) {
      f<<"joboption_outputfile_"<<(i++)<<"="<<value_for_shell(s->pfn,true)<<std::endl;
    }
  } 
  if(opt_add) f<<opt_add<<std::endl;

  return true;
}
     
JobReqResult get_acl(const Arc::JobDescription& arc_job_desc, std::string& acl) {
  if( !arc_job_desc.Application.AccessControl ) return JobReqSuccess;
  Arc::XMLNode typeNode = arc_job_desc.Application.AccessControl["Type"];
  Arc::XMLNode contentNode = arc_job_desc.Application.AccessControl["Content"];
  if( !contentNode ) {
    logger.msg(Arc::ERROR, "ARC: acl element wrongly formated - missing Content element");
    return JobReqMissingFailure;
  };
  if( (!typeNode) || ( ( (std::string) typeNode ) == "GACL" ) || ( ( (std::string) typeNode ) == "ARC" ) ) {
    std::string str_content;
    if(contentNode.Size() > 0) {
      Arc::XMLNode acl_doc;
      contentNode.Child().New(acl_doc);
      acl_doc.GetDoc(str_content);
    } else {
      str_content = (std::string)contentNode;
    }
    if( str_content != "" ) acl=str_content;
  } else {
    logger.msg(Arc::ERROR, "ARC: unsupported ACL type specified: %s", (std::string)typeNode);
    return JobReqUnsupportedFailure;
  };
  return JobReqSuccess;
}

bool set_execs(const Arc::JobDescription& desc, const std::string& session_dir) {
  if (!desc) return false;

  if (desc.Application.Executable.Name[0] != '/' && desc.Application.Executable.Name[0] != '$') {
    std::string executable = desc.Application.Executable.Name;
    if(canonical_dir(executable) != 0) {
      logger.msg(Arc::ERROR, "Bad name for executable: ", executable);
      return false;
    }
    fix_file_permissions(session_dir+"/"+executable,true);
  }

  for(std::list<Arc::FileType>::const_iterator it = desc.DataStaging.File.begin();
      it!=desc.DataStaging.File.end();it++) {
    if(it->IsExecutable) {
      std::string executable = it->Name;
      if (executable[0] != '/' && executable[0] != '.' && executable[1] != '/') executable = "./"+executable;
      if(canonical_dir(executable) != 0) {
        logger.msg(Arc::ERROR, "Bad name for executable: %s", executable); 
        return false;
      }
      fix_file_permissions(session_dir+"/"+executable,true);
    }
  }

  return true;
}

std::ostream& operator<<(std::ostream &o,const value_for_shell &s) {
  if(s.str == NULL) return o;
  if(s.quote) o<<"'";
  const char* p = s.str;
  for(;;) {
    const char* pp = strchr(p,'\'');
    if(pp == NULL) { o<<p; if(s.quote) o<<"'"; break; };
    o.write(p,pp-p); o<<"'\\''"; p=pp+1;
  };
  return o;
}

std::ostream& operator<<(std::ostream &o,const numvalue_for_shell &s) {
  o<<s.n;
  return o;
}
  
