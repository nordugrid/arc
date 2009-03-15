// -*- indent-tabs-mode: nil -*-

#include <list>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/job/runtimeenvironment.h>

#include "RSLParser.h"
#include "XRSLParser.h"

namespace Arc {

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

  bool XRSLParser::parse(JobInnerRepresentation& innerRepresentation,
                         const std::string source) {

    RSLParser parser(source);
    const RSL *r = parser.Parse();
    if (!r) {
      logger.msg(ERROR, "RSL parsing error");
      return false;
    }

    std::list<const RSL*> l = SplitRSL(r);

    std::list<JobInnerRepresentation> J;
    for (std::list<const RSL*>::iterator it = l.begin(); it != l.end(); it++) {
      JobInnerRepresentation j;
      if (!parse(*it, j)) {
        logger.msg(ERROR, "XRSL parsing error");
        return false;
      }
      J.push_back(j);
    }

    if (J.size() > 1) {
      logger.msg(WARNING, "Multiple RSL in one file not yet supported");
      return false;
    }

    innerRepresentation = *J.begin();
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
                                std::list<std::list<std::string> >& value) {
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

  bool XRSLParser::parse(const RSL *r, JobInnerRepresentation& j) {

    const RSLBoolean *b;
    const RSLCondition *c;

    if ((b = dynamic_cast<const RSLBoolean*>(r))) {
      if (b->Op() == RSLAnd)
        for (std::list<RSL*>::const_iterator it = b->begin();
             it != b->end(); it++)
          if (!parse(*it, j)) {
            logger.msg(ERROR, "XRSL parsing problem");
            return false;
          }
          else if (b->Op() == RSLOr) {
            if (b->size() != 1) {
              logger.msg(ERROR, "RSL conditionals currentsly not yet supported");
              return false;
            }
            if (!parse(*b->begin(), j)) {
              logger.msg(ERROR, "XRSL parsing problem");
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
        return SingleValue(c, j.Executable);

      if (c->Attr() == "arguments")
        return ListValue(c, j.Argument);

      if (c->Attr() == "stdin")
        return SingleValue(c, j.Input);

      if (c->Attr() == "stdout")
        return SingleValue(c, j.Output);

      if (c->Attr() == "stderr")
        return SingleValue(c, j.Error);

      if (c->Attr() == "inputfiles") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          std::list<std::string>::iterator it2 = it->begin();
          FileType file;
          file.Name = *it2++;
          SourceType source;
          if (!it2->empty())
            source.URI = *it2;
          else
            source.URI = file.Name;
          source.Threads = -1;
          file.Source.push_back(source);
          file.KeepData = false;
          file.IsExecutable = false;
          file.DownloadToCache = j.cached;
          j.File.push_back(file);
        }
        return true;
      }

      if (c->Attr() == "executables") {
        std::list<std::string> execs;
        if (!ListValue(c, execs))
          return false;
        for (std::list<std::string>::iterator it = execs.begin();
             it != execs.end(); it++)
          for (std::list<FileType>::iterator it2 = j.File.begin();
               it2 != j.File.end(); it2++)
            if (it2->Name == (*it))
              it2->IsExecutable = true;
        return true;
      }

      if (c->Attr() == "cache") {
        std::string cache;
        if (!SingleValue(c, cache))
          return false;
        if (lower(cache) == "yes") {
          j.cached = true;
          for (std::list<FileType>::iterator it = j.File.begin();
               it != j.File.end(); it++)
            if (!it->Source.empty())
              it->DownloadToCache = true;
        }
        return true;
      }

      if (c->Attr() == "outputfiles") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          std::list<std::string>::iterator it2 = it->begin();
          FileType file;
          file.Name = *it2++;
          TargetType target;
          if (!it2->empty())
            target.URI = *it2;
          target.Threads = -1;
          file.Target.push_back(target);
          file.KeepData = false;
          file.IsExecutable = false;
          file.DownloadToCache = j.cached;
          j.File.push_back(file);
        }
        return true;
      }

      if (c->Attr() == "queue")
        return SingleValue(c, j.QueueName);

      if (c->Attr() == "starttime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        j.ProcessingStartTime = time;
        return true;
      }

      if (c->Attr() == "lifetime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        j.SessionLifeTime = Period(time, PeriodMinutes);
        return true;
      }

      if (c->Attr() == "cputime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        j.TotalCPUTime = Period(time, PeriodMinutes);
        return true;
      }

      if (c->Attr() == "walltime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        j.TotalWallTime = Period(time, PeriodMinutes);
        return true;
      }

      if (c->Attr() == "gridtime") {
        std::string time;
        if (!SingleValue(c, time))
          return false;
        ReferenceTimeType rtime;
        rtime.benchmark = "gridtime";
        rtime.value = 2800;
        rtime.time = Period(time, PeriodMinutes);
        j.ReferenceTime.push_back(rtime);
        return true;
      }

      if (c->Attr() == "benchmarks") {
        std::list<std::list<std::string> > bm;
        if (!SeqListValue(c, bm))
          return false;
        for (std::list<std::list<std::string> >::iterator it = bm.begin();
             it != bm.end(); it++) {
          if (it->size() != 3) {
            logger.msg(ERROR, "XRSL benchmark sequence has wrong size");
            return false;
          }
          std::list<std::string>::iterator it2 = it->begin();
          ReferenceTimeType rtime;
          rtime.benchmark = *it2++;
          rtime.value = stringtod(*it2++);
          rtime.time = Period(*it2, PeriodMinutes);
          j.ReferenceTime.push_back(rtime);
        }
        return true;
      }

      if (c->Attr() == "memory") {
        std::string mem;
        if (!SingleValue(c, mem))
          return false;
        j.IndividualPhysicalMemory = stringtoi(mem);
        return true;
      }

      if (c->Attr() == "disk") {
        std::string disk;
        if (!SingleValue(c, disk))
          return false;
        j.DiskSpace = stringtoi(disk);
        return true;
      }

      if (c->Attr() == "runtimeenvironment")
        // Needs some though
        return true;

      if (c->Attr() == "middleware")
        return SingleValue(c, j.CEType);

      if (c->Attr() == "opsys")
        return SingleValue(c, j.OSName);

      if (c->Attr() == "join") {
        std::string join;
        if (!SingleValue(c, join))
          return false;
        if (lower(join) == "true")
          j.Join = true;
        return true;
      }

      if (c->Attr() == "gmlog")
        return SingleValue(c, j.LogDir);

      if (c->Attr() == "jobname")
        return SingleValue(c, j.JobName);

      if (c->Attr() == "ftpthreads") {
        std::string sthreads;
        if (!SingleValue(c, sthreads))
          return false;
        int threads = stringtoi(sthreads);
        for (std::list<FileType>::iterator it = j.File.begin();
             it != j.File.end(); it++) {
          for (std::list<SourceType>::iterator sit = it->Source.begin();
               sit != it->Source.end(); sit++)
            if (sit->Threads > threads || sit->Threads == -1)
              sit->Threads = threads;
          for (std::list<TargetType>::iterator tit = it->Target.begin();
               tit != it->Target.end(); tit++)
            if (tit->Threads > threads || tit->Threads == -1)
              tit->Threads = threads;
        }
        for (std::list<DirectoryType>::iterator it = j.Directory.begin();
             it != j.Directory.end(); it++) {
          for (std::list<SourceType>::iterator sit = it->Source.begin();
               sit != it->Source.end(); sit++)
            if (sit->Threads > threads || sit->Threads == -1)
              sit->Threads = threads;
          for (std::list<TargetType>::iterator tit = it->Target.begin();
               tit != it->Target.end(); tit++)
            if (tit->Threads > threads || tit->Threads == -1)
              tit->Threads = threads;
        }
        return true;
      }

      if (c->Attr() == "acl") {
        std::string acl;
        if (!SingleValue(c, acl))
          return false;
        XMLNode node(acl);
        node.New(j.AccessControl);
        return true;
      }

      if (c->Attr() == "cluster") {
        std::string cluster;
        if (!SingleValue(c, cluster))
          return false;
        j.EndPointURL = cluster;
        return true;
      }

      if (c->Attr() == "notify") {
        std::list<std::string> l;
        if (!ListValue(c, l))
          return false;
        NotificationType nofity;
        if (l.size() < 2) {
          logger.msg(DEBUG, "Invalid notify attribute");
          return false;
        }
        for (std::list<std::string>::iterator it = l.begin();
             it != l.end(); it++) {
          if (it == l.begin())
            for (std::string::iterator i = it->begin(); i != it->end(); i++)
              switch (*i) {
              case 'b':
                nofity.State.push_back("PREPARING");
                break;

              case 'q':
                nofity.State.push_back("INLRMS");
                break;

              case 'f':
                nofity.State.push_back("FINISHING");
                break;

              case 'e':
                nofity.State.push_back("FINISHED");
                break;

              case 'c':
                nofity.State.push_back("CANCELLED");
                break;

              case 'd':
                nofity.State.push_back("DELETED");
                break;

              default:
                logger.msg(DEBUG, "Invalid notify attribute: %c", *i);
                return false;
              }
          else
            nofity.Address.push_back(*it);
        }
        j.Notification.push_back(nofity);
        return true;
      }

      if (c->Attr() == "replicacollection") {
        std::string collection;
        if (!SingleValue(c, collection))
          return false;
        URL url(collection);
        for (std::list<FileType>::iterator it = j.File.begin();
             it != j.File.end(); it++)
          it->DataIndexingService = url;
        for (std::list<DirectoryType>::iterator it = j.Directory.begin();
             it != j.Directory.end(); it++)
          it->DataIndexingService = url;
        return true;
      }

      if (c->Attr() == "rerun") {
        std::string rerun;
        if (!SingleValue(c, rerun))
          return false;
        j.LRMSReRun = stringtoi(rerun);
        return true;
      }

      if (c->Attr() == "architecture")
        return SingleValue(c, j.Platform);

      if (c->Attr() == "nodeaccess") {
        std::list<std::string> l;
        if (!ListValue(c, l))
          return false;
        for (std::list<std::string>::iterator it = l.begin();
             it != l.end(); it++)
          if (*it == "inbound")
            j.InBound = true;
          else if (*it == "outbound")
            j.OutBound = true;
          else {
            logger.msg(DEBUG, "Invalid nodeaccess value: %s", *it);
            return false;
          }
        return true;
      }

      if (c->Attr() == "dryrun") {
        std::string dryrun;
        if (!SingleValue(c, dryrun))
          return false;
        if (lower(dryrun) == "yes" || lower(dryrun) == "dryrun")
          ; // j.DryRun = true;
        return true;
      }

      if (c->Attr() == "rslsubstitution")
        // Handled internally by the RSL parser
        return true;

      if (c->Attr() == "environment") {
        std::list<std::list<std::string> > ll;
        if (!SeqListValue(c, ll))
          return false;
        for (std::list<std::list<std::string> >::iterator it = ll.begin();
             it != ll.end(); it++) {
          if (it->size() != 2) {
            logger.msg(ERROR, "Wrong length of env sequence");
            return false;
          }
          EnvironmentType env;
          std::list<std::string>::iterator it2 = it->begin();
          env.name_attribute = *it2++;
          env.value = *it2++;
          j.Environment.push_back(env);
        }
        return true;
      }

      if (c->Attr() == "count") {
        std::string count;
        if (!SingleValue(c, count))
          return false;
        j.ProcessPerHost = stringtoi(count);
        return true;
      }

      if (c->Attr() == "jobreport") {
        std::string jobreport;
        if (!SingleValue(c, jobreport))
          return false;
        j.RemoteLogging = jobreport;
        return true;
      }

      if (c->Attr() == "credentialserver") {
        std::string credentialserver;
        if (!SingleValue(c, credentialserver))
          return false;
        j.CredentialService = credentialserver;
        return true;
      }

      logger.msg(DEBUG, "Unknown XRSL attribute: %s", c->Attr());
      return false;
    }
  }

  bool XRSLParser::getProduct(const JobInnerRepresentation& j,
                              std::string& product) const {
    RSLBoolean r(RSLAnd);

    if (!j.Executable.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Executable));
      r.Add(new RSLCondition("executable", RSLEqual, l));
    }

    if (!j.Argument.empty()) {
      RSLList *l = new RSLList;
      for (std::list<std::string>::const_iterator it = j.Argument.begin();
           it != j.Argument.end(); it++)
        l->Add(new RSLLiteral(*it));
      r.Add(new RSLCondition("arguments", RSLEqual, l));
    }

    if (!j.Input.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Input));
      r.Add(new RSLCondition("stdin", RSLEqual, l));
    }

    if (j.Join) {
      if (!j.Output.empty() && !j.Error.empty() && j.Output != j.Error) {
        logger.msg(ERROR, "Incompatible RSL attributes");
        return false;
      }
      if (!j.Output.empty() || j.Error.empty()) {
        const std::string& eo = !j.Output.empty() ? j.Output : j.Error;
        RSLList *l1 = new RSLList;
        l1->Add(new RSLLiteral(eo));
        r.Add(new RSLCondition("stdout", RSLEqual, l1));
        RSLList *l2 = new RSLList;
        l2->Add(new RSLLiteral(eo));
        r.Add(new RSLCondition("stderr", RSLEqual, l2));
      }
    }
    else {
      if (!j.Output.empty()) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(j.Output));
        r.Add(new RSLCondition("stdout", RSLEqual, l));
      }

      if (!j.Error.empty()) {
        RSLList *l = new RSLList;
        l->Add(new RSLLiteral(j.Error));
        r.Add(new RSLCondition("stderr", RSLEqual, l));
      }
    }

    if (j.TotalCPUTime != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.TotalCPUTime.GetPeriod())));
      r.Add(new RSLCondition("cputime", RSLEqual, l));
    }

    if (j.TotalWallTime != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.TotalWallTime.GetPeriod())));
      r.Add(new RSLCondition("walltime", RSLEqual, l));
    }

    if (j.IndividualPhysicalMemory != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.IndividualPhysicalMemory)));
      r.Add(new RSLCondition("memory", RSLEqual, l));
    }

    if (!j.Environment.empty()) {
      RSLList *l = new RSLList;
      for (std::list<EnvironmentType>::const_iterator it = j.Environment.begin();
           it != j.Environment.end(); it++) {
        RSLList *s = new RSLList;
        s->Add(new RSLLiteral(it->name_attribute));
        s->Add(new RSLLiteral(it->value));
        l->Add(new RSLSequence(s));
      }
      r.Add(new RSLCondition("environment", RSLEqual, l));
    }

    if (!j.File.empty()) {
      RSLList *l = NULL;
      for (std::list<FileType>::const_iterator it = j.File.begin();
           it != j.File.end(); it++)
        for (std::list<SourceType>::const_iterator sit = it->Source.begin();
             sit != it->Source.end(); sit++) {
          RSLList *s = new RSLList;
          s->Add(new RSLLiteral(it->Name));
          if (sit->URI.Protocol() == "file") {
            struct stat fileStat;
            if (stat(sit->URI.Path().c_str(), &fileStat) == 0)
              s->Add(new RSLLiteral(tostring(fileStat.st_size)));
            else {
              logger.msg(ERROR, "Can not stat local input file");
              delete s;
              if (l)
                delete l;
              return false;
            }
          }
          else
            s->Add(new RSLLiteral(sit->URI.fullstr()));
          if (!l)
            l = new RSLList;
          l->Add(new RSLSequence(s));
        }
      if (l)
        r.Add(new RSLCondition("inputfiles", RSLEqual, l));
    }

    if (!j.File.empty()) {
      RSLList *l = NULL;
      for (std::list<FileType>::const_iterator it = j.File.begin();
           it != j.File.end(); it++)
        if (it->IsExecutable) {
          if (!l)
            l = new RSLList;
          l->Add(new RSLLiteral(it->Name));
        }
      if (l)
        r.Add(new RSLCondition("executables", RSLEqual, l));
    }

    if (!j.File.empty()) {
      bool output(false), error(false);
      RSLList *l = NULL;
      for (std::list<FileType>::const_iterator it = j.File.begin();
           it != j.File.end(); it++)
        for (std::list<TargetType>::const_iterator tit = it->Target.begin();
             tit != it->Target.end(); tit++) {
          RSLList *s = new RSLList;
          s->Add(new RSLLiteral(it->Name));
          if (it->Name == j.Output)
            output = true;
          if (it->Name == j.Error)
            error = true;
          if (!tit->URI || tit->URI.Protocol() == "file")
            s->Add(new RSLLiteral(""));
          else
            s->Add(new RSLLiteral(tit->URI.fullstr()));
          if (!l)
            l = new RSLList;
          l->Add(new RSLSequence(s));
        }
      if (!j.Output.empty() && !output) {
        RSLList *s = new RSLList;
        s->Add(new RSLLiteral(j.Output));
        s->Add(new RSLLiteral(""));
        if (!l)
          l = new RSLList;
        l->Add(new RSLSequence(s));
      }
      if (!j.Error.empty() && !error) {
        RSLList *s = new RSLList;
        s->Add(new RSLLiteral(j.Error));
        s->Add(new RSLLiteral(""));
        if (!l)
          l = new RSLList;
        l->Add(new RSLSequence(s));
      }
      if (l)
        r.Add(new RSLCondition("outputfiles", RSLEqual, l));
    }

    if (!j.QueueName.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.QueueName));
      r.Add(new RSLCondition("queue", RSLEqual, l));
    }

    if (j.LRMSReRun != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.LRMSReRun)));
      r.Add(new RSLCondition("rerun", RSLEqual, l));
    }

    if (j.SessionLifeTime != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.SessionLifeTime.GetPeriod())));
      r.Add(new RSLCondition("lifetime", RSLEqual, l));
    }

    if (j.DiskSpace != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.DiskSpace)));
      r.Add(new RSLCondition("disk", RSLEqual, l));
    }

    if (!j.RunTimeEnvironment.empty()) {
      RSLList *l = new RSLList;
      for (std::list<RunTimeEnvironmentType>::const_iterator it =
             j.RunTimeEnvironment.begin();
           it != j.RunTimeEnvironment.end(); it++)
        l->Add(new RSLLiteral(it->Name + (it->Version.empty() ? "" :
                                          '-' + *it->Version.begin())));
      r.Add(new RSLCondition("runtimeenvironment", RSLEqual, l));
    }

    if (!j.OSName.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.OSName));
      r.Add(new RSLCondition("opsys", RSLEqual, l));
    }

    if (!j.Platform.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.Platform));
      r.Add(new RSLCondition("architacture", RSLEqual, l));
    }

    if (j.ProcessPerHost != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(tostring(j.ProcessPerHost)));
      r.Add(new RSLCondition("count", RSLEqual, l));
    }

    if (j.ProcessingStartTime != -1) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.ProcessingStartTime.str(MDSTime)));
      r.Add(new RSLCondition("starttime", RSLEqual, l));
    }

    if (!j.LogDir.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.LogDir));
      r.Add(new RSLCondition("gmlog", RSLEqual, l));
    }

    if (!j.JobName.empty()) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.JobName));
      r.Add(new RSLCondition("jobname", RSLEqual, l));
    }

    if (j.AccessControl) {
      RSLList *l = new RSLList;
      std::string acl;
      j.AccessControl.GetXML(acl, true);
      l->Add(new RSLLiteral(acl));
      r.Add(new RSLCondition("acl", RSLEqual, l));
    }

    if (!j.Notification.empty()) {
      RSLList *l = new RSLList;
      for (std::list<NotificationType>::const_iterator it =
             j.Notification.begin(); it != j.Notification.end(); it++) {
        std::string states;
        for (std::list<std::string>::const_iterator it2 = it->State.begin();
             it2 != it->State.end(); it2++) {
          if (*it2 == "PREPARING")
            states += "b";
          else if (*it2 == "INLRMS")
            states += "q";
          else if (*it2 == "FINISHING")
            states += "f";
          else if (*it2 == "FINISHED")
            states += "e";
          else if (*it2 == "CANCELLED")
            states += "c";
          else if (*it2 == "DELETED")
            states += "d";
          else {
            logger.msg(DEBUG, "Invalid State \"%s\" in Notification!", *it2);
            return false;
          }
        }
        l->Add(new RSLLiteral(states));
        for (std::list<std::string>::const_iterator it2 = it->Address.begin();
             it2 != it->Address.end(); it2++)
          l->Add(new RSLLiteral(*it2));
      }
      r.Add(new RSLCondition("notify", RSLEqual, l));
    }

    if (bool(j.RemoteLogging)) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.RemoteLogging.fullstr()));
      r.Add(new RSLCondition("jobreport", RSLEqual, l));
    }

    if (bool(j.CredentialService)) {
      RSLList *l = new RSLList;
      l->Add(new RSLLiteral(j.CredentialService.fullstr()));
      r.Add(new RSLCondition("credentialserver", RSLEqual, l));
    }

    std::stringstream ss;
    ss << r;
    product = ss.str();

    return true;
  }

} // namespace Arc
