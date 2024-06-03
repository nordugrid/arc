#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <glibmm.h>

#include <arc/FileUtils.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/XMLNode.h>

#include "../files/ControlFileHandling.h"
#include "../conf/GMConfig.h"
#include "../../delegation/DelegationStore.h"
#include "../../delegation/DelegationStores.h"

#include "JobDescriptionHandler.h"

// TODO: move to using process_job_req as much as possible

namespace ARex {

Arc::Logger JobDescriptionHandler::logger(Arc::Logger::getRootLogger(), "JobDescriptionHandler");
const std::string JobDescriptionHandler::NG_RSL_DEFAULT_STDIN("/dev/null");
const std::string JobDescriptionHandler::NG_RSL_DEFAULT_STDOUT("/dev/null");
const std::string JobDescriptionHandler::NG_RSL_DEFAULT_STDERR("/dev/null");

bool JobDescriptionHandler::process_job_req(const GMJob &job,JobLocalDescription &job_desc) const {
  /* read local first to get some additional info pushed here by script */
  job_local_read_file(job.get_id(),config,job_desc);
  /* some default values */
  if(job_desc.lrms.empty()) job_desc.lrms=config.DefaultLRMS();
  if(job_desc.queue.empty()) job_desc.queue=config.DefaultQueue();
  if(job_desc.lifetime.empty()) job_desc.lifetime=Arc::tostring(config.KeepFinished());

  if(parse_job_req(job.get_id(),job_desc) != JobReqSuccess) return false;
  if(job_desc.reruns>config.Reruns()) job_desc.reruns=config.Reruns();
  if(!job_local_write_file(job,config,job_desc)) return false;

  // Convert delegation ids to credential paths.
  // Add default credentials for file which have no own assigned.
  ARex::DelegationStores* delegs = config.GetDelegations();
  std::string default_cred = job_proxy_filename(job.get_id(), config); // TODO: drop job.proxy as source of delegation
  std::string default_cred_type;
  if(!job_desc.delegationid.empty()) {
    if(delegs) {
      std::list<std::string> meta;
      DelegationStore& deleg = delegs->operator[](config.DelegationDir());
      std::string fname = deleg.FindCred(job_desc.delegationid, job_desc.DN, meta);
      if(!fname.empty()) {
        default_cred = fname;
        default_cred_type = (!meta.empty())?meta.front():"";
      };
    };
  };

  // Resolve delegation ids into proxy credential paths
  for(std::list<FileData>::iterator f = job_desc.inputdata.begin();
                                   f != job_desc.inputdata.end(); ++f) {
    if(f->has_lfn()) {
      if(f->cred.empty()) {
        f->cred = default_cred;
        f->cred_type = default_cred_type;
      } else {
        std::string path;
        std::list<std::string> meta;
        if(delegs) path = (*delegs)[config.DelegationDir()].FindCred(f->cred,job_desc.DN,meta);
        f->cred = path;
        f->cred_type = (!meta.empty())?meta.front():"";
      };
    };
  };
  for(std::list<FileData>::iterator f = job_desc.outputdata.begin();
                                   f != job_desc.outputdata.end(); ++f) {
    if(f->has_lfn()) {
      if(f->cred.empty()) {
        f->cred = default_cred;
      } else {
        std::string path;
        std::list<std::string> meta;
        ARex::DelegationStores* delegs = config.GetDelegations();
        if(delegs) path = (*delegs)[config.DelegationDir()].FindCred(f->cred,job_desc.DN,meta);
        f->cred = path;
        f->cred_type = (!meta.empty())?meta.front():"";
      };
    };
  };
  if(!job_input_write_file(job,config,job_desc.inputdata)) return false;
  if(!job_output_write_file(job,config,job_desc.outputdata,job_output_success)) return false;
  return true;
}

JobReqResult JobDescriptionHandler::parse_job_req_from_mem(JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,const std::string &desc_str,bool check_acl) const {
  {
    std::list<Arc::JobDescription> descs;
    Arc::JobDescriptionResult r = Arc::JobDescription::Parse(desc_str, descs, "", "GRIDMANAGER");
    if (!r) {
      std::string failure = r.str();
      if(failure.empty()) failure = "Unable to parse job description.";
      return JobReqResult(JobReqInternalFailure, "", failure);
    }
    if(descs.size() != 1) {
      return JobReqResult(JobReqInternalFailure, "", "Multiple job descriptions not supported");
    }
    arc_job_desc = descs.front();
  }

  return parse_job_req_internal(job_desc, arc_job_desc, check_acl);
}

JobReqResult JobDescriptionHandler::parse_job_req_from_file(JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,const std::string &fname,bool check_acl) const {
  Arc::JobDescriptionResult arc_job_res = get_arc_job_description(fname, arc_job_desc);
  if (!arc_job_res) {
    std::string failure = arc_job_res.str();
    if(failure.empty()) failure = "Unable to read or parse job description.";
    return JobReqResult(JobReqInternalFailure, "", failure);
  }
  return parse_job_req_internal(job_desc, arc_job_desc, check_acl);
}


JobReqResult JobDescriptionHandler::parse_job_req_internal(JobLocalDescription &job_desc,Arc::JobDescription const& arc_job_desc,bool check_acl) const {
  if (!arc_job_desc.Resources.RunTimeEnvironment.isResolved()) {
    return JobReqResult(JobReqInternalFailure, "", "Runtime environments have not been resolved.");
  }

  job_desc = arc_job_desc;
  // Additional queue processing
  // TODO: Temporary solution.
  // Check for special WLCG queues made out of "queue name_VO name".
  for(std::list<std::string>::const_iterator q = config.Queues().begin();
               q != config.Queues().end();++q) {
    if(*q == job_desc.queue) break;
    const std::list<std::string> & vos = config.AuthorizedVOs(q->c_str()); // per queue
    const std::list<std::string> & cvos = config.AuthorizedVOs(""); // per cluster
    bool vo_found = false;
    if(!vos.empty()) {
      for(std::list<std::string>::const_iterator vo = vos.begin();vo != vos.end(); ++vo) {
        std::string synthetic_queue = *q;
        synthetic_queue += "_";
        synthetic_queue += *vo;
        if(synthetic_queue == job_desc.queue) {
          vo_found = true;
          break;
        };
      };
    } else {
      for(std::list<std::string>::const_iterator vo = cvos.begin();vo != cvos.end(); ++vo) {
        std::string synthetic_queue = *q;
        synthetic_queue += "_";
        synthetic_queue += *vo;
        if(synthetic_queue == job_desc.queue) {
          vo_found = true;
          break;
        };
      };
    };
    if(vo_found) {
      logger.msg(Arc::WARNING, "Replacing queue '%s' with '%s'", job_desc.queue, *q);
      job_desc.queue = *q;
      break;
    };
  };

  if (check_acl) return get_acl(arc_job_desc);
  return JobReqSuccess;
}

JobReqResult JobDescriptionHandler::parse_job_req(const JobId &job_id,JobLocalDescription &job_desc,bool check_acl) const {
  Arc::JobDescription arc_job_desc;
  return parse_job_req(job_id,job_desc,arc_job_desc,check_acl);
}

JobReqResult JobDescriptionHandler::parse_job_req(const JobId &job_id,JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,bool check_acl) const {
  std::string fname = job_control_path(config.ControlDir(),job_id,sfx_desc);
  return parse_job_req_from_file(job_desc,arc_job_desc,fname,check_acl);
}

std::string JobDescriptionHandler::get_local_id(const JobId &job_id) const {
  std::string id;
  std::string joboption("joboption_jobid=");
  std::string fgrami(job_control_path(config.ControlDir(),job_id,sfx_grami));
  std::list<std::string> grami_data;
  if (Arc::FileRead(fgrami, grami_data)) {
    for (std::list<std::string>::iterator line = grami_data.begin(); line != grami_data.end(); ++line) {
      if (line->find(joboption) == 0) {
        id = line->substr(joboption.length());
        id = Arc::trim(id, "'");
        break;
      }
    }
  }
  return id;
}

bool JobDescriptionHandler::write_grami_executable(std::ofstream& f, const std::string& name, const Arc::ExecutableType& exec) const {
  std::string executable = Arc::trim(exec.Path);
  if (executable[0] != '/' && executable[0] != '$' && !(executable[0] == '.' && executable[1] == '/')) executable = "./"+executable;
  f<<"joboption_"<<name<<"_0"<<"="<<value_for_shell(executable.c_str(),true)<<std::endl;
  int i = 1;
  for (std::list<std::string>::const_iterator it = exec.Argument.begin();
       it != exec.Argument.end(); it++, i++) {
    f<<"joboption_"<<name<<"_"<<i<<"="<<value_for_shell(it->c_str(),true)<<std::endl;
  }
  if(exec.SuccessExitCode.first) {
    f<<"joboption_"<<name<<"_code"<<"="<<Arc::tostring(exec.SuccessExitCode.second)<<std::endl;
  }
  return true;
}

bool JobDescriptionHandler::write_grami(GMJob &job,const char *opt_add) const {
  const std::string fname = job_control_path(config.ControlDir(),job.get_id(),sfx_desc);

  Arc::JobDescription arc_job_desc;
  if (!get_arc_job_description(fname, arc_job_desc)) return false;

  return write_grami(arc_job_desc, job, opt_add);
}

bool JobDescriptionHandler::write_grami(const Arc::JobDescription& arc_job_desc, GMJob& job, const char* opt_add) const {
  if(job.GetLocalDescription(config) == NULL) return false;
  JobLocalDescription& job_local_desc = *(job.GetLocalDescription(config));
  const std::string session_dir = job.SessionDir();
  const std::string control_dir = config.ControlDir();
  const std::string fgrami = job_control_path(control_dir,job.get_id(),sfx_grami);
  std::ofstream f(fgrami.c_str(),std::ios::out | std::ios::trunc);
  if(!f.is_open()) return false;
  if(!fix_file_permissions(fgrami,job,config)) return false;
  if(!fix_file_owner(fgrami,job)) return false;

  f<<"joboption_directory='"<<session_dir<<"'"<<std::endl;
  f<<"joboption_controldir='"<<control_dir<<"'"<<std::endl;

  if(!write_grami_executable(f,"arg",arc_job_desc.Application.Executable)) return false;
  int n = 0;
  for(std::list<Arc::ExecutableType>::const_iterator e =
                     arc_job_desc.Application.PreExecutable.begin();
                     e != arc_job_desc.Application.PreExecutable.end(); ++e) {
    if(!write_grami_executable(f,"pre_"+Arc::tostring(n),*e)) return false;
    ++n;
  }
  for(std::list<Arc::ExecutableType>::const_iterator e =
                     arc_job_desc.Application.PostExecutable.begin();
                     e != arc_job_desc.Application.PostExecutable.end(); ++e) {
    if(!write_grami_executable(f,"post_"+Arc::tostring(n),*e)) return false;
  }

  f<<"joboption_stdin="<<value_for_shell(arc_job_desc.Application.Input.empty()?NG_RSL_DEFAULT_STDIN:arc_job_desc.Application.Input,true)<<std::endl;

  if (!arc_job_desc.Application.Output.empty()) {
    std::string output = arc_job_desc.Application.Output;
    if (!Arc::CanonicalDir(output)) {
      logger.msg(Arc::ERROR,"Bad name for stdout: %s", output);
      return false;
    }
  }
  f<<"joboption_stdout="<<value_for_shell(arc_job_desc.Application.Output.empty()?NG_RSL_DEFAULT_STDOUT:session_dir+"/"+arc_job_desc.Application.Output,true)<<std::endl;
  if (!arc_job_desc.Application.Error.empty()) {
    std::string error = arc_job_desc.Application.Error;
    if (!Arc::CanonicalDir(error)) {
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
    f<<"joboption_env_"<<i<<"=GRID_GLOBAL_JOBID="<<value_for_shell(job_local_desc.globalid,true)<<std::endl;
    ++i;
    f<<"joboption_env_"<<i<<"=GRID_GLOBAL_JOBURL="<<value_for_shell(job_local_desc.globalurl,true)<<std::endl;
    ++i;
    f<<"joboption_env_"<<i<<"=GRID_GLOBAL_JOBINTERFACE="<<value_for_shell(job_local_desc.interface,true)<<std::endl;
    ++i;
    f<<"joboption_env_"<<i<<"=GRID_GLOBAL_JOBHOST="<<value_for_shell(job_local_desc.headhost,true)<<std::endl;
  }


  f<<"joboption_cputime="<<(arc_job_desc.Resources.TotalCPUTime.range.max != -1 ? Arc::tostring(arc_job_desc.Resources.TotalCPUTime.range.max):"")<<std::endl;
  f<<"joboption_walltime="<<(arc_job_desc.Resources.TotalWallTime.range.max != -1 ? Arc::tostring(arc_job_desc.Resources.TotalWallTime.range.max):"")<<std::endl;
  f<<"joboption_memory="<<(arc_job_desc.Resources.IndividualPhysicalMemory.max != -1 ? Arc::tostring(arc_job_desc.Resources.IndividualPhysicalMemory.max):"")<<std::endl;
  f<<"joboption_virtualmemory="<<(arc_job_desc.Resources.IndividualVirtualMemory.max != -1 ? Arc::tostring(arc_job_desc.Resources.IndividualVirtualMemory.max):"")<<std::endl;
  f<<"joboption_disk="<<(arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max != -1 ? Arc::tostring(arc_job_desc.Resources.DiskSpaceRequirement.DiskSpace.max):"")<<std::endl;

  //calculate the number of nodes/hosts needed
  {
    int count= arc_job_desc.Resources.SlotRequirement.NumberOfSlots;
    if (count != -1) {
      int count_per_node = arc_job_desc.Resources.SlotRequirement.SlotsPerHost;
      f<<"joboption_count="<<Arc::tostring(count)<<std::endl;
      f<<"joboption_countpernode="<<Arc::tostring(count_per_node)<<std::endl;
      if (count_per_node > 0) {
        int num_nodes = count / count_per_node;
        if ((count % count_per_node) > 0) num_nodes++;
        f<<"joboption_numnodes="<<Arc::tostring(num_nodes)<<std::endl;
      }
      f<<"joboption_penv_type="<<Arc::tostring(arc_job_desc.Resources.ParallelEnvironment.Type)<<std::endl;
      f<<"joboption_penv_version="<<Arc::tostring(arc_job_desc.Resources.ParallelEnvironment.Version)<<std::endl;
      f<<"joboption_penv_processesperhost="<<(arc_job_desc.Resources.ParallelEnvironment.ProcessesPerSlot != -1 ? Arc::tostring(arc_job_desc.Resources.ParallelEnvironment.ProcessesPerSlot):"")<<std::endl;
      f<<"joboption_penv_threadsperprocess="<<(arc_job_desc.Resources.ParallelEnvironment.ThreadsPerProcess != -1 ? Arc::tostring(arc_job_desc.Resources.ParallelEnvironment.ThreadsPerProcess):"")<<std::endl;

    }else{
      f<<"joboption_count=1"<<std::endl;
    } 
  }

  if (arc_job_desc.Resources.SlotRequirement.ExclusiveExecution == Arc::SlotRequirementType::EE_TRUE){
    f<<"joboption_exclusivenode=true"<<std::endl;
  }



  {
    int i = 0;
    for (std::list<Arc::Software>::const_iterator itSW = arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().begin();
         itSW != arc_job_desc.Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++) {
      if (itSW->empty()) continue;
      std::string rte = Arc::upper(*itSW);
      if (!Arc::CanonicalDir(rte)) {
        logger.msg(Arc::ERROR, "Bad name for runtime environment: %s", (std::string)*itSW);
        return false;
      }
      f<<"joboption_runtime_"<<i<<"="<<value_for_shell((std::string)*itSW,true)<<std::endl;
      const std::list<std::string>& opts = itSW->getOptions();
      int n = 1;
      for(std::list<std::string>::const_iterator opt = opts.begin();
                            opt != opts.end();++opt) {
        f<<"joboption_runtime_"<<i<<"_"<<n<<"="<<value_for_shell(*opt,true)<<std::endl;
        ++n;
      }
      ++i;
    }
  }
  f<<"joboption_jobname="<<value_for_shell(job_local_desc.jobname,true)<<std::endl;
  f<<"joboption_queue="<<value_for_shell(job_local_desc.queue,true)<<std::endl;
  f<<"joboption_starttime="<<(job_local_desc.exectime != -1?job_local_desc.exectime.str(Arc::MDSTime):"")<<std::endl;
  f<<"joboption_gridid="<<value_for_shell(job.get_id(),true)<<std::endl;
  f<<"joboption_priority="<<Arc::tostring(job_local_desc.priority)<<std::endl;

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

Arc::JobDescriptionResult JobDescriptionHandler::get_arc_job_description(const std::string& fname, Arc::JobDescription& desc) const {
  std::string job_desc_str;
  if (!job_description_read_file(fname, job_desc_str)) {
    logger.msg(Arc::ERROR, "Job description file could not be read.");
    return false;
  }

  std::list<Arc::JobDescription> descs;
  Arc::JobDescriptionResult r = Arc::JobDescription::Parse(job_desc_str, descs, "", "GRIDMANAGER");
  if (r) {
    if(descs.size() == 1) {
      desc = descs.front();
    } else {
      r = Arc::JobDescriptionResult(false,"Multiple job descriptions not supported");
    }
  }
  return r;
}

JobReqResult JobDescriptionHandler::get_acl(const Arc::JobDescription& arc_job_desc) const {
  if( !arc_job_desc.Application.AccessControl ) return JobReqSuccess;
  Arc::XMLNode typeNode = arc_job_desc.Application.AccessControl["Type"];
  Arc::XMLNode contentNode = arc_job_desc.Application.AccessControl["Content"];
  if( !contentNode ) {
    std::string failure = "acl element wrongly formatted - missing Content element";
    logger.msg(Arc::ERROR, failure);
    return JobReqResult(JobReqMissingFailure, "", failure);
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
    return JobReqResult(JobReqSuccess, str_content);
  }
  std::string failure = "ARC: unsupported ACL type specified: " + (std::string)typeNode;
  logger.msg(Arc::ERROR, "%s", failure);
  return JobReqResult(JobReqUnsupportedFailure, "", failure);
}

/* parse job description and set specified file permissions to executable */
bool JobDescriptionHandler::set_execs(const GMJob &job) const {
  std::string fname = job_control_path(config.ControlDir(),job.get_id(),sfx_desc);
  Arc::JobDescription desc;
  if (!get_arc_job_description(fname, desc)) return false;

  std::string session_dir = job.SessionDir();
  if (desc.Application.Executable.Path[0] != '/' && desc.Application.Executable.Path[0] != '$') {
    std::string executable = desc.Application.Executable.Path;
    if(!Arc::CanonicalDir(executable)) {
      logger.msg(Arc::ERROR, "Bad name for executable: %s", executable);
      return false;
    }
    fix_file_permissions_in_session(session_dir+"/"+executable,job,config,true);
  }

  // TOOD: Support for PreExecutable and PostExecutable

  for(std::list<Arc::InputFileType>::const_iterator it = desc.DataStaging.InputFiles.begin();
      it!=desc.DataStaging.InputFiles.end();it++) {
    if(it->IsExecutable) {
      std::string executable = it->Name;
      if (executable[0] != '/' && executable[0] != '.' && executable[1] != '/') executable = "./"+executable;
      if(!Arc::CanonicalDir(executable)) {
        logger.msg(Arc::ERROR, "Bad name for executable: %s", executable);
        return false;
      }
      fix_file_permissions_in_session(session_dir+"/"+executable,job,config,true);
    }
  }

  return true;
}

std::ostream& operator<<(std::ostream& o, const JobDescriptionHandler::value_for_shell& s) {
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

} // namespace ARex
