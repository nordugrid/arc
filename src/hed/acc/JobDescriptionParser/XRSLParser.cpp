// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/JobDescription.h>

#include "RSLParser.h"
#include "XRSLParser.h"

namespace Arc {

  XRSLParser::XRSLParser()
    : JobDescriptionParser("XRSL") {}

  XRSLParser::~XRSLParser() {}

  Plugin* XRSLParser::Instance(PluginArgument *arg) {
    return new XRSLParser();
  }

  static Software::ComparisonOperator convertOperator(const RSLRelOp& op) {
    if (op == RSLNotEqual) return &Software::operator!=;
    if (op == RSLLess) return &Software::operator<;
    if (op == RSLGreater) return &Software::operator>;
    if (op == RSLLessOrEqual) return &Software::operator <=;
    if (op == RSLGreaterOrEqual) return &Software::operator>=;
    return &Software::operator==;
  }

  static RSLRelOp convertOperator(const Software::ComparisonOperator& op) {
    if (op == &Software::operator==) return RSLEqual;
    if (op == &Software::operator<)  return RSLLess;
    if (op == &Software::operator>)  return RSLGreater;
    if (op == &Software::operator<=) return RSLLessOrEqual;
    if (op == &Software::operator>=) return RSLGreaterOrEqual;
    return RSLNotEqual;
  }

  static std::list<const RSL*> SplitRSL(const RSL *r) {
    const RSLBoolean *b;
    std::list<const RSL*> l;
    if ((b = dynamic_cast<const RSLBoolean*>(r)) && b->Op() == RSLMulti)
      for (std::list<RSL*>::const_iterator it = b->begin();
           it != b->end(); it++) {
        std::list<const RSL*> L = SplitRSL(*it);
        l.insert(l.end(), L.begin(), L.end());
      }
    else
      l.push_back(r);
    return l;
  }

  bool XRSLParser::cached = true;

  bool XRSLParser::Parse(const std::string& source, JobDescription& jobdesc) const {
    jobdesc = JobDescription();

    RSLParser parser(source);
    const RSL *r = parser.Parse();
    if (!r) {
      logger.msg(VERBOSE, "RSL parsing error");
      return false;
    }

    std::list<const RSL*> l = SplitRSL(r);

    std::list<JobDescription> J;
    for (std::list<const RSL*>::iterator it = l.begin(); it != l.end(); it++) {
      JobDescription j;
      if (!Parse(*it, j)) {
        logger.msg(ERROR, "XRSL parsing error");
        return false;
      }

      j.XRSL_elements["clientxrsl"] = source;

      J.push_back(j);
    }

    if (J.size() > 1) {
      logger.msg(WARNING, "Multiple RSL in one job description not yet supported");
      return false;
    }

    if(J.size() != 1) {
      // Probably never happens so check is just in case of future changes
      logger.msg(WARNING, "No RSL content in job desription found");
      return false;
    }

    if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
      std::string action = "request";
      if (J.front().XRSL_elements.find("action") != J.front().XRSL_elements.end()) {
        action = J.front().XRSL_elements["action"];
      }
      // action = request means real job description.
      // Any other action may (and currently should) have almost 
      // empty job description.
      if (action == "request") {
        if(J.front().Application.Executable.Name.empty()) return false;
      }
    } else {
      // action is not expected in client side job request
      if (J.front().XRSL_elements.find("action") != J.front().XRSL_elements.end()) return false;
      if (J.front().Application.Executable.Name.empty()) return false;
    }
    jobdesc = J.front();

    return true;
  }

  bool XRSLParser::SingleValue(const RSLCondition *c,
                               std::string& value) {
    if (!value.empty()) {
      logger.msg(ERROR, "XRSL attribute %s multiply defined", c->Attr());
      return false;
    }
    if (c->size() != 1) {
      logger.msg(ERROR, "XRSL attribute %s is not a single value", c->Attr());
      return false;
    }
    const RSLLiteral *n = dynamic_cast<const RSLLiteral*>(*c->begin());
    if (!n) {
      logger.msg(ERROR, "XRSL attribute %s is not a string", c->Attr());
      return false;
    }
    value = n->Value();
    return true;
  }

  bool XRSLParser::ListValue(const RSLCondition *c,
                             std::list<std::string>& value) {
    if (!value.empty()) {
      logger.msg(ERROR, "XRSL attribute %s multiply defined", c->Attr());
      return false;
    }
    for (std::list<RSLValue*>::const_iterator it = c->begin();
         it != c->end(); it++) {
      const RSLLiteral *n = dynamic_cast<const RSLLiteral*>(*it);
      if (!n) {
        logger.msg(ERROR, "XRSL attribute %s is not a string", c->Attr());
        return false;
      }
      value.push_back(n->Value());
    }
    return true;
  }

  bool XRSLParser::SeqListValue(const RSLCondition *c,
                                std::list<std::list<std::string> >& value,
                                int seqlength) {
    if (!value.empty()) {
      logger.msg(ERROR, "XRSL attribute %s multiply defined", c->Attr());
      return false;
    }
    for (std::list<RSLValue*>::const_iterator it = c->begin();
         it != c->end(); it++) {
      const RSLSequence *s = dynamic_cast<const RSLSequence*>(*it);
      if (!s) {
        logger.msg(ERROR, "XRSL attribute %s is not sequence", c->Attr());
        return false;
      }
      if (seqlength != -1 && int(s->size()) != seqlength) {
        logger.msg(ERROR, "XRSL attribute %s has wrong sequence length",
                   c->Attr());
        return false;
      }
      std::list<std::string> l;
      for (std::list<RSLValue*>::const_iterator it = s->begin();
           it != s->end(); it++) {
        const RSLLiteral *n = dynamic_cast<const RSLLiteral*>(*it);
        if (!n) {
          logger.msg(ERROR, "XRSL attribute %s is not a string", c->Attr());
          return false;
        }
        l.push_back(n->Value());
      }
      value.push_back(l);
    }
    return true;
  }

  static char StateToShortcut(const std::string& state) {
    if(state == "PREPARING") {
      return 'b';
    }
    if(state == "INLRMS") {
      return 'q';
    }
    if(state == "FINISHING") {
      return 'f';
    }
    if(state == "FINISHED") {
      return 'e';
    }
    if(state == "DELETED") {
      return 'd';
    }
    if(state == "CANCELING") {
      return 'c';
    }
    return ' ';
  }

  static std::string ShortcutToState(char state) {
    if(state == 'b') {
      return "PREPARING";
    }
    if(state == 'q') {
      return "INLRMS";
    }
    if(state == 'f') {
      return "FINISHING";
    }
    if(state == 'e') {
      return "FINISHED";
    }
    if(state == 'd') {
      return "DELETED";
    }
    if(state == 'c') {
      return "CANCELING";
    }
    return "";
  }

  static bool AddNotificationState(
         NotificationType &notification,
         const std::string& states) {
    for (int n = 0; n<states.length(); n++) {
      std::string state = ShortcutToState(states[n]);
      if (state.empty()) {
        return false;
      }
      for (std::list<std::string>::const_iterator s = notification.States.begin();
           s != notification.States.end(); s++) {
        if(*s == state) { // Check if state is already added.
          state.resize(0);
          break;
        }
      }
      if (!state.empty()) {
        notification.States.push_back(state);
      }
    }
    return true;
  }

  static bool AddNotification(
         std::list<NotificationType> &notifications,
         const std::string& states, const std::string& email) {
    for (std::list<NotificationType>::iterator it = notifications.begin();
         it != notifications.end(); it++) {
      if (it->Email == email) { // If email already exist in the list add states to that entry.
        return AddNotificationState(*it,states);
      }
    }

    NotificationType notification;
    notification.Email = email;
    if (!AddNotificationState(notification,states)) {
      return false;
    }
    notifications.push_back(notification);

    return true;
  }

  bool XRSLParser::Parse(const RSL *r, JobDescription& j) const {
    const RSLBoolean *b;
    const RSLCondition *c;
    if ((b = dynamic_cast<const RSLBoolean*>(r))) {
      if (b->Op() == RSLAnd) {
        for (std::list<RSL*>::const_iterator it = b->begin();
             it != b->end(); it++)
          if (!Parse(*it, j)) {
            return false;
          }
      }
      else if (b->Op() == RSLOr) {
        if (b->size() != 1) {
          logger.msg(ERROR, "RSL conditionals currently not yet supported");
          return false;
        }
        if (!Parse(*b->begin(), j)) {
          return false;
        }
      }
      else {
        logger.msg(ERROR, "Unexpected RSL type");
        return false;
      }
    }
    else if ((c = dynamic_cast<const RSLCondition*>(r))) {
      if (c->Attr() == "executable")
        return SingleValue(c, j.Application.Executable.Name);

      if (c->Attr() == "arguments")
        return ListValue(c, j.Application.Executable.Argument);

      if (c->Attr() == "stdin")
        return SingleValue(c, j.Application.Input);

      if (c->Attr() == "stdout")
        return SingleValue(c, j.Application.Output);

      if (c->Attr() == "stderr")
        return SingleValue(c, j.Application.Error);

      if (c->Attr() == "inputfiles") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll, 2))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          std::list<std::string>::iterator it2 = it->begin();
          FileType file;
          file.Name = *it2++;
          DataSourceType source;
          long fileSize;
          // The second string in the list (it2) might either be a URL or file size.
          if (!it2->empty() && !stringto(*it2, fileSize)) {
            source.URI = *it2;
            if (!source.URI)
              return false;
          }
          else {
            source.URI = file.Name;
          }
          source.Threads = -1;
          file.Source.push_back(source);
          file.KeepData = false;
          file.IsExecutable = false;
          file.DownloadToCache = true;
          j.DataStaging.File.push_back(file);
        }
        return true;
      }

      if (c->Attr() == "executables") {
        std::list<std::string> execs;
        if (!ListValue(c, execs))
          return false;
        for (std::list<std::string>::iterator it = execs.begin();
             it != execs.end(); it++)
          for (std::list<FileType>::iterator it2 = j.DataStaging.File.begin();
               it2 != j.DataStaging.File.end(); it2++)
            if (it2->Name == (*it))
              it2->IsExecutable = true;
        return true;
      }

      if (c->Attr() == "cache") {
        std::string cache;
        if (!SingleValue(c, cache))
          return false;
        if (lower(cache) != "yes")
          cached = false;
        return true;
      }

      if (c->Attr() == "outputfiles") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll, 2))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          std::list<std::string>::iterator it2 = it->begin();
          FileType file;
          file.Name = *it2++;
          long fileSize;
          URL turl(*it2);
          // The second string in the list (it2) might be a URL or file size.
          if (!it2->empty() && !stringto(*it2, fileSize) && turl.Protocol() != "file") {
            if (!turl)
              return false;
            DataTargetType target;
            target.URI = turl;
            target.Threads = -1;
            file.Target.push_back(target);
          } else
            file.KeepData = true;
          file.IsExecutable = false;
          file.DownloadToCache = false;
          j.DataStaging.File.push_back(file);
        }
        return true;
      }

      if (c->Attr() == "queue") {
        std::string queueName;
        if (!SingleValue(c, queueName))
          return false;
        if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
          if (c->Op() != RSLEqual) {
            logger.msg(ERROR, "Parsing the queue xrsl attribute failed. An invalid comparison operator was used, only \"=\" is allowed.");
            return false;
          }
        } else {
          if (c->Op() != RSLNotEqual && c->Op() != RSLEqual) {
            logger.msg(ERROR, "Parsing the queue xrsl attribute failed. An invalid comparison operator was used, only \"!=\" or \"=\" are allowed.");
            return false;
          }
        }

        if (j.Resources.CandidateTarget.empty()) {
          ResourceTargetType candidateTarget;
          candidateTarget.EndPointURL = URL();
          candidateTarget.QueueName = queueName;
          candidateTarget.UseQueue = (c->Op() == RSLEqual);
          j.Resources.CandidateTarget.push_back(candidateTarget);
        }
        else {
          // It is not correct to allow next queue to overwrite previous one
          // Fix it globally. BLOCKER !!!!
          logger.msg(ERROR, "Parsing the queue xrsl attribute failed. Multiple queue attributes found, only one is allowed.");
          return false;
          //j.Resources.CandidateTarget.front().QueueName = queueName;
          //j.Resources.CandidateTarget.front().UseQueue = (c->Op() == RSLEqual);
        }
        return true;
      }

      if (c->Attr() == "starttime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        j.Application.ProcessingStartTime = time;
        return true;
      }

      if (c->Attr() == "lifetime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
          j.Resources.SessionLifeTime = Period(time, PeriodSeconds);
        } else {
          j.Resources.SessionLifeTime = Period(time, PeriodDays);
        }
        return true;
      }

      if (c->Attr() == "cputime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
          j.Resources.TotalCPUTime = Period(time, PeriodSeconds).GetPeriod();
        } else {
          j.Resources.TotalCPUTime = Period(time, PeriodMinutes).GetPeriod();
        }
        return true;
      }

      if (c->Attr() == "walltime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
          j.Resources.TotalWallTime = Period(time, PeriodSeconds).GetPeriod();
        } else {
          j.Resources.TotalWallTime = Period(time, PeriodMinutes).GetPeriod();
        }
        return true;
      }

      if (c->Attr() == "gridtime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
          j.Resources.TotalCPUTime.range = Period(time, PeriodSeconds).GetPeriod();
        } else {
          j.Resources.TotalCPUTime.range = Period(time, PeriodMinutes).GetPeriod();
        }
        j.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>("clock rate", 2800);
        return true;
      }

      if (c->Attr() == "benchmarks") {
        std::list<std::list<std::string> > bm;
        if (!SeqListValue(c, bm, 3))
          return false;
        double bValue;
        // Only the first parsable benchmark is currently supported.
        for (std::list< std::list<std::string> >::const_iterator it = bm.begin();
             it != bm.end(); it++) {
          std::list<std::string>::const_iterator itB = it->begin();
          if (!stringto(*++itB, bValue))
            continue;
          if(GetHint("SOURCEDIALECT") == "GRIDMANAGER") {
            j.Resources.TotalCPUTime.range = Period(*++itB, PeriodSeconds).GetPeriod();
          } else {
            j.Resources.TotalCPUTime.range = Period(*++itB, PeriodMinutes).GetPeriod();
          }
          j.Resources.TotalCPUTime.benchmark = std::pair<std::string, double>(it->front(), bValue);
          return true;
        }
        return false;
      }

      if (c->Attr() == "memory") {
        std::string mem;
        if (!SingleValue(c, mem))
          return false;
        j.Resources.IndividualPhysicalMemory = stringto<int64_t>(mem);
        return true;
      }

      if (c->Attr() == "disk") {
        std::string disk;
        if (!SingleValue(c, disk))
          return false;
          j.Resources.DiskSpaceRequirement.DiskSpace = stringto<int64_t>(disk);
        return true;
      }

      if (c->Attr() == "runtimeenvironment") {
        std::string runtime;
        if (!SingleValue(c, runtime))
          return false;
        j.Resources.RunTimeEnvironment.add(Software(runtime), convertOperator(c->Op()));
        return true;
       }

      if (c->Attr() == "middleware") {
        std::string cetype;
        if (!SingleValue(c, cetype))
          return false;
        j.Resources.CEType.add(Software(cetype), convertOperator(c->Op()));
        return true;
      }

      if (c->Attr() == "opsys") {
        std::string opsys;
        if (!SingleValue(c, opsys))
          return false;
        j.Resources.OperatingSystem.add(Software(opsys), convertOperator(c->Op()));
        return true;
      }

      if (c->Attr() == "join") {
        std::string join;
        if (!SingleValue(c, join))
          return false;
        j.Application.Join = (lower(join) == "yes");
        return true;
      }

      if (c->Attr() == "gmlog")
        return SingleValue(c, j.Application.LogDir);

      if (c->Attr() == "jobname")
        return SingleValue(c, j.Identification.JobName);

      if (c->Attr() == "ftpthreads") {
        std::string sthreads;
        if (!SingleValue(c, sthreads))
          return false;
        int threads = stringtoi(sthreads);
        for (std::list<FileType>::iterator it = j.DataStaging.File.begin();
             it != j.DataStaging.File.end(); it++) {
          if (it->Source.front().Threads > threads || it->Source.front().Threads == -1)
            it->Source.front().Threads = threads;
          if (it->Target.front().Threads > threads || it->Target.front().Threads == -1)
            it->Target.front().Threads = threads;
        }
        return true;
      }

      if (c->Attr() == "acl") {
        std::string acl;
        if (!SingleValue(c, acl))
          return false;
        XMLNode node(acl);
        node.New(j.Application.AccessControl);
        return true;
      }

      if (c->Attr() == "cluster") {
        std::string cluster;
        if (!SingleValue(c, cluster))
          return false;
        if (!URL(cluster))
          return false;
        if (j.Resources.CandidateTarget.empty()) {
          ResourceTargetType candidateTarget;
          candidateTarget.EndPointURL = cluster;
          candidateTarget.QueueName = "";
          candidateTarget.UseQueue = true;
          j.Resources.CandidateTarget.push_back(candidateTarget);
        }
        else
          j.Resources.CandidateTarget.front().EndPointURL = cluster;
        return true;
      }

      if (c->Attr() == "notify") {
        std::list<std::string> l;
        if (!ListValue(c, l))
          return false;
        for (std::list<std::string>::const_iterator notf = l.begin();
             notf != l.end(); ++notf) {
          std::list<std::string> ll;
          tokenize(*notf, ll, " \t");
          std::list<std::string>::const_iterator it = ll.begin();
          std::string states = "be"; // Default value.
          if (it->find('@') == std::string::npos) { // The first string is state flags.
            if (ll.size() == 1) { // Only state flags in value.
              logger.msg(ERROR, "Syntax error in notify attribute value ('%s'), it must contain an email address.", *notf);
              return false;
            }
            states = *it;
            it++;
          }
          for (; it != ll.end(); it++) {
            if (it->find('@') == std::string::npos) {
              logger.msg(ERROR, "Syntax error in notify attribute value ('%s'), it must only contain email addresses after state flag(s).", *notf);
              return false;
            }
            if(!AddNotification(j.Application.Notification,states,*it)) {
              logger.msg(ERROR, "Syntax error in notify attribute value ('%s'), it contains unknown state flags.", *notf);
              return false;
            }
          }
        }
        return true;
      }

      if (c->Attr() == "replicacollection") {
        std::string collection;
        if (!SingleValue(c, collection))
          return false;
        URL url(collection);
        if (!url)
          return false;
        for (std::list<FileType>::iterator it = j.DataStaging.File.begin();
             it != j.DataStaging.File.end(); it++)
          it->DataIndexingService.push_back(url);
        return true;
      }

      if (c->Attr() == "rerun") {
        std::string rerun;
        if (!SingleValue(c, rerun))
          return false;
        j.Application.Rerun = stringtoi(rerun);
        return true;
      }

      if (c->Attr() == "architecture")
        return SingleValue(c, j.Resources.Platform);

      if (c->Attr() == "nodeaccess") {
        std::list<std::string> l;
        if (!ListValue(c, l))
          return false;
        for (std::list<std::string>::iterator it = l.begin();
             it != l.end(); it++)
          if (*it == "inbound")
            j.Resources.NodeAccess = (j.Resources.NodeAccess == NAT_OUTBOUND || j.Resources.NodeAccess == NAT_INOUTBOUND ? NAT_INOUTBOUND : NAT_INBOUND);
          else if (*it == "outbound")
            j.Resources.NodeAccess = (j.Resources.NodeAccess == NAT_INBOUND || j.Resources.NodeAccess == NAT_INOUTBOUND ? NAT_INOUTBOUND : NAT_OUTBOUND);
          else {
            logger.msg(VERBOSE, "Invalid nodeaccess value: %s", *it);
            return false;
          }
        return true;
      }

      if (c->Attr() == "dryrun") {
        std::string dryrun;
        if (!SingleValue(c, dryrun))
          return false;
        if (lower(dryrun) == "yes" || lower(dryrun) == "dryrun")
         j.XRSL_elements["dryrun"] = "yes";
        return true;
      }

      // Underscore, in 'rsl_substitution', is removed by normalization.
      if (c->Attr() == "rslsubstitution")
        // Handled internally by the RSL parser
        return true;

      if (c->Attr() == "environment") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll, 2))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          j.Application.Environment.push_back(std::make_pair(it->front(), it->back()));
        }
        return true;
      }

      if (c->Attr() == "count") {
        std::string count;
        if (!SingleValue(c, count))
          return false;
        j.Resources.SlotRequirement.ProcessPerHost = stringtoi(count);
        return true;
      }

      if (c->Attr() == "jobreport") {
        std::string jobreport;
        if (!SingleValue(c, jobreport))
          return false;
        if (!URL(jobreport))
          return false;
        j.Application.RemoteLogging.push_back(URL(jobreport));
        return true;
      }

      if (c->Attr() == "credentialserver") {
        std::string credentialserver;
        if (!SingleValue(c, credentialserver))
          return false;
        if (!URL(credentialserver))
          return false;
        j.Application.CredentialService.push_back(credentialserver);
        return true;
      }

      // GM-side attributes.
      if (c->Attr() == "action") {
        std::string action;
        if (!SingleValue(c, action))
          return false;
        if (action != "request" && action != "cancel" && action != "clean" && action != "renew" && action != "restart") {
          logger.msg(VERBOSE, "Invalid action value %s", action);
          return false;
        }
        j.XRSL_elements["action"] = action;
        return true;
      }

      if (c->Attr() == "hostname") {
        std::string hostname;
        if (!SingleValue(c, hostname))
          return false;
        j.XRSL_elements["hostname"] = hostname;
        return true;
      }

      if (c->Attr() == "jobid") {
        std::string jobid;
        if (!SingleValue(c, jobid))
          return false;
        j.XRSL_elements["jobid"] = jobid;
        return true;
      }

      if (c->Attr() == "clientxrsl") {
        std::string clientxrsl;
        if (!SingleValue(c, clientxrsl))
          return false;
        j.XRSL_elements["clientxrsl"] = clientxrsl;
        return true;
      }

      if (c->Attr() == "clientsoftware") {
        std::string clientsoftware;
        if (!SingleValue(c, clientsoftware))
          return false;
        j.XRSL_elements["clientsoftware"] = clientsoftware;
        return true;
      }

      if (c->Attr() == "savestate") {
        std::string savestate;
        if (!SingleValue(c, savestate))
          return false;
        j.XRSL_elements["savestate"] = savestate;
        return true;
      }

      // Unsupported Globus RSL attributes.
      if (c->Attr() == "resourcemanagercontact" ||
          c->Attr() == "directory" ||
          c->Attr() == "maxcputime" ||
          c->Attr() == "maxwalltime" ||
          c->Attr() == "maxtime" ||
          c->Attr() == "maxmemory" ||
          c->Attr() == "minmemory" ||
          c->Attr() == "grammyjob" ||
          c->Attr() == "project" ||
          c->Attr() == "hostcount" ||
          c->Attr() == "label" ||
          c->Attr() == "subjobcommstype" ||
          c->Attr() == "subjobstarttype" ||
          c->Attr() == "filecleanup" ||
          c->Attr() == "filestagein" ||
          c->Attr() == "filestageinshared" ||
          c->Attr() == "filestageout" ||
          c->Attr() == "gasscache" ||
          c->Attr() == "jobtype" ||
          c->Attr() == "librarypath" ||
          c->Attr() == "remoteiourl" ||
          c->Attr() == "scratchdir") {
        logger.msg(WARNING, "The specified Globus attribute (%s) is not supported. %s ignored.", c->Attr(), c->Attr());
        return true;
      }

      logger.msg(VERBOSE, "Unknown XRSL attribute: %s - Ignoring it.", c->Attr());
      return true;
    }
    else {
      logger.msg(ERROR, "Unexpected RSL type");
      return false;
    }

    // This part will run only when the parsing is at the end of the xrsl file

    // Value defined in "cache" element is applicable to all input files
    for (std::list<FileType>::iterator it = j.DataStaging.File.begin();
         it != j.DataStaging.File.end(); it++)
      if (!it->Source.empty())
        it->DownloadToCache = cached;

    // Since OR expressions in XRSL is spilt into serveral JobDescriptions the requirement for RTE must be all (AND).
    j.Resources.RunTimeEnvironment.setRequirement(true);

    return true;
  }

  bool XRSLParser::UnParse(const JobDescription& j, std::string& product) const {
    // First check if the job description is valid.
    if (j.Application.Executable.Name.empty()) {
      return false;
    }

    RSLBoolean r(RSLAnd);

    if (!j.Application.Executable.Name.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.Executable.Name));
      r.Add(new RSLCondition("executable", RSLEqual, l));
    }

    if (!j.Application.Executable.Argument.empty()) {
      RSLList *l = new RSLList;
      for (std::list<std::string>::const_iterator it = j.Application.Executable.Argument.begin();
           it != j.Application.Executable.Argument.end(); it++)
        l->Add(new RSLLiteral(*it));
      r.Add(new RSLCondition("arguments", RSLEqual, l));
    }

    if (!j.Application.Input.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.Input));
      r.Add(new RSLCondition("stdin", RSLEqual, l));
    }

    if (j.Application.Join) {
      if (!j.Application.Output.empty() && !j.Application.Error.empty() && j.Application.Output != j.Application.Error) {
        logger.msg(ERROR, "Incompatible RSL attributes");
        return "";
      }
      if (!j.Application.Output.empty() || !j.Application.Error.empty()) {
        const std::string& eo = !j.Application.Output.empty() ? j.Application.Output : j.Application.Error;
        RSLList *l1 = new RSLList;
        l1->Add(new RSLLiteral(eo));
        r.Add(new RSLCondition("stdout", RSLEqual, l1));
        RSLList *l2 = new RSLList;
        l2->Add(new RSLLiteral(eo));
        r.Add(new RSLCondition("stderr", RSLEqual, l2));
      }
    }
    else {
      if (!j.Application.Output.empty()) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(j.Application.Output));
        r.Add(new RSLCondition("stdout", RSLEqual, l));
      }

      if (!j.Application.Error.empty()) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(j.Application.Error));
        r.Add(new RSLCondition("stderr", RSLEqual, l));
      }
    }

    if (j.Resources.TotalCPUTime.range > -1) {
      RSLList *l = new RSLList;
      if(GetHint("TARGETDIALECT") == "GRIDMANAGER") {
        // Seconds
        l->Add(new RSLLiteral(tostring(j.Resources.TotalCPUTime.range)));
      } else {
        // Free format
        l->Add(new RSLLiteral((std::string)Period(j.Resources.TotalCPUTime.range)));
      }
      r.Add(new RSLCondition("cputime", RSLEqual, l));
    }

    if (j.Resources.TotalWallTime.range > -1) {
      RSLList *l = new RSLList;
      if(GetHint("TARGETDIALECT") == "GRIDMANAGER") {
        // Seconds
        l->Add(new RSLLiteral(tostring(j.Resources.TotalWallTime.range)));
      } else {
        // Free format
        l->Add(new RSLLiteral((std::string)Period(j.Resources.TotalWallTime.range)));
      }
      r.Add(new RSLCondition("walltime", RSLEqual, l));
    }

    if (j.Resources.IndividualPhysicalMemory > -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.Resources.IndividualPhysicalMemory)));
      r.Add(new RSLCondition("memory", RSLEqual, l));
    }

    if (!j.Application.Environment.empty()) {
      RSLList *l = new RSLList;
      for (std::list< std::pair<std::string, std::string> >::const_iterator it = j.Application.Environment.begin();
           it != j.Application.Environment.end(); it++) {
        RSLList *s = new RSLList;
        s->Add(new RSLLiteral(it->first));
        s->Add(new RSLLiteral(it->second));
        l->Add(new RSLSequence(s));
      }
      r.Add(new RSLCondition("environment", RSLEqual, l));
    }

    if (!j.DataStaging.File.empty() || !j.Application.Executable.Name.empty() || !j.Application.Input.empty()) {
      struct stat fileStat;
      RSLList *l = NULL;
      for (std::list<FileType>::const_iterator it = j.DataStaging.File.begin();
           it != j.DataStaging.File.end(); it++) {
        if (it->Source.empty())
          continue;
        RSLList *s = new RSLList;
        s->Add(new RSLLiteral(it->Name));
        if (it->Source.front().URI.Protocol() == "file") {
          if (stat(it->Source.front().URI.Path().c_str(), &fileStat) == 0)
            s->Add(new RSLLiteral(tostring(fileStat.st_size)));
          else {
            logger.msg(ERROR, "Cannot stat local input file %s", it->Source.front().URI.Path());
            delete s;
            if (l)
              delete l;
            return "";
          }
        }
        else
          s->Add(new RSLLiteral(it->Source.front().URI.fullstr()));
        if (!l)
          l = new RSLList;
        l->Add(new RSLSequence(s));
      }

      if (l)
        r.Add(new RSLCondition("inputfiles", RSLEqual, l));

      // Executables
      l = NULL;
      for (std::list<FileType>::const_iterator it = j.DataStaging.File.begin();
           it != j.DataStaging.File.end(); it++)
        if (it->IsExecutable) {
          if (!l)
            l = new RSLList;
          l->Add(new RSLLiteral(it->Name));
        }
      if (l)
        r.Add(new RSLCondition("executables", RSLEqual, l));
    }

    if (!j.DataStaging.File.empty() || !j.Application.Output.empty() || !j.Application.Error.empty()) {
      RSLList *l = NULL;
      for (std::list<FileType>::const_iterator it = j.DataStaging.File.begin();
           it != j.DataStaging.File.end(); it++) {
        if (!it->Target.empty()) {
          RSLList *s = new RSLList;
          s->Add(new RSLLiteral(it->Name));
          if (!it->Target.front().URI || it->Target.front().URI.Protocol() == "file")
            s->Add(new RSLLiteral(""));
          else {
            URL url(it->Target.front().URI);
            if (it->DownloadToCache)
              url.AddOption("cache", "yes");
            s->Add(new RSLLiteral(url.fullstr()));
          }
          if (!l)
            l = new RSLList;
          l->Add(new RSLSequence(s));
        }
        else if (it->KeepData) {
          RSLList *s = new RSLList;
          s->Add(new RSLLiteral(it->Name));
          s->Add(new RSLLiteral(""));
          if (!l)
            l = new RSLList;
          l->Add(new RSLSequence(s));
        }
      }

      if (l)
        r.Add(new RSLCondition("outputfiles", RSLEqual, l));
    }

    if (!j.Resources.CandidateTarget.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Resources.CandidateTarget.front().QueueName));
      r.Add(new RSLCondition("queue", (j.Resources.CandidateTarget.front().UseQueue ? RSLEqual : RSLNotEqual), l));
    }

    if (j.Application.Rerun != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.Application.Rerun)));
      r.Add(new RSLCondition("rerun", RSLEqual, l));
    }

    if (j.Resources.SessionLifeTime != -1) {
      RSLList *l = new RSLList;
      if(GetHint("TARGETDIALECT") == "GRIDMANAGER") {
        // Seconds
        l->Add(new RSLLiteral(tostring(j.Resources.SessionLifeTime.GetPeriod())));
      } else {
        // Free format
        l->Add(new RSLLiteral((std::string)j.Resources.SessionLifeTime));
      }
      r.Add(new RSLCondition("lifetime", RSLEqual, l));
    }

    if (j.Resources.DiskSpaceRequirement.DiskSpace > -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.Resources.DiskSpaceRequirement.DiskSpace)));
      r.Add(new RSLCondition("disk", RSLEqual, l));
    }

    if (!j.Resources.RunTimeEnvironment.empty()) {
      std::list<Software>::const_iterator itSW = j.Resources.RunTimeEnvironment.getSoftwareList().begin();
      std::list<Software::ComparisonOperator>::const_iterator itCO = j.Resources.RunTimeEnvironment.getComparisonOperatorList().begin();
      for (; itSW != j.Resources.RunTimeEnvironment.getSoftwareList().end(); itSW++, itCO++) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(*itSW));
        r.Add(new RSLCondition("runtimeenvironment", convertOperator(*itCO), l));
      }
    }

    if (!j.Resources.CEType.empty()) {
      std::list<Software>::const_iterator itSW = j.Resources.CEType.getSoftwareList().begin();
      std::list<Software::ComparisonOperator>::const_iterator itCO = j.Resources.CEType.getComparisonOperatorList().begin();
      for (; itSW != j.Resources.CEType.getSoftwareList().end(); itSW++, itCO++) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(*itSW));
        r.Add(new RSLCondition("middleware", convertOperator(*itCO), l));
      }
    }

    if (!j.Resources.OperatingSystem.empty()) {
      std::list<Software>::const_iterator itSW = j.Resources.OperatingSystem.getSoftwareList().begin();
      std::list<Software::ComparisonOperator>::const_iterator itCO = j.Resources.OperatingSystem.getComparisonOperatorList().begin();
      for (; itSW != j.Resources.OperatingSystem.getSoftwareList().end(); itSW++, itCO++) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral((std::string)*itSW));
        r.Add(new RSLCondition("opsys", convertOperator(*itCO), l));
      }
    }

    if (!j.Resources.Platform.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Resources.Platform));
      r.Add(new RSLCondition("architecture", RSLEqual, l));
    }

    if (j.Resources.SlotRequirement.ProcessPerHost > -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.Resources.SlotRequirement.ProcessPerHost)));
      r.Add(new RSLCondition("count", RSLEqual, l));
    }

    if (j.Application.ProcessingStartTime != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.ProcessingStartTime.str(MDSTime)));
      r.Add(new RSLCondition("starttime", RSLEqual, l));
    }

    if (!j.Application.LogDir.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.LogDir));
      r.Add(new RSLCondition("gmlog", RSLEqual, l));
    }

    if (!j.Identification.JobName.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Identification.JobName));
      r.Add(new RSLCondition("jobname", RSLEqual, l));
    }

    if (j.Application.AccessControl) {
      RSLList *l = new RSLList;
      std::string acl;
      j.Application.AccessControl.GetXML(acl, true);
      l->Add(new RSLLiteral(acl));
      r.Add(new RSLCondition("acl", RSLEqual, l));
    }

    if (!j.Application.Notification.empty()) {
      RSLList *l = new RSLList;
      for (std::list<NotificationType>::const_iterator it = j.Application.Notification.begin();
           it != j.Application.Notification.end(); it++) {
        // Suboptimal, group emails later
        std::string states;
        for (std::list<std::string>::const_iterator s = it->States.begin();
             s != it->States.end(); s++) {
          char state = StateToShortcut(*s);
          if (state != ' ') states+=state;
        }
        l->Add(new RSLLiteral(states + " " + it->Email));
      }
      r.Add(new RSLCondition("notify", RSLEqual, l));
    }

    if (!j.Application.RemoteLogging.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.RemoteLogging.front().fullstr()));
      r.Add(new RSLCondition("jobreport", RSLEqual, l));
    }

    if (!j.Application.CredentialService.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Application.CredentialService.front().fullstr()));
      r.Add(new RSLCondition("credentialserver", RSLEqual, l));
    }

    for (std::map<std::string, std::string>::const_iterator it = j.XRSL_elements.begin();
         it != j.XRSL_elements.end(); it++) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(it->second));
      r.Add(new RSLCondition(it->first, RSLEqual, l));
    }

    std::stringstream ss;
    ss << r;
    product = ss.str();
    return true;
  }

} // namespace Arc
