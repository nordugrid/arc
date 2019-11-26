// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/ArcLocation.h>
#include <arc/OptionParser.h>
#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/CheckSum.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataExternalComm.h>
#include <arc/data/DataPointDelegate.h>

#include "DataExternalHelper.h"

namespace Arc {

  Logger DataExternalHelper::logger(Logger::getRootLogger(), "DataPoint.External");


  DataStatus DataExternalHelper::Check() {
    if (!plugin)
      return DataStatus::NotInitializedError;
    return plugin->Check(true); // TODO: check_meta
  }

  DataStatus DataExternalHelper::Remove() {
    if (!plugin)
      return DataStatus::NotInitializedError;
    return plugin->Remove();
  }

  DataStatus DataExternalHelper::CreateDirectory(bool with_parents) {
    if (!plugin)
      return DataStatus::NotInitializedError;
    return plugin->CreateDirectory(with_parents);
  }

  DataStatus DataExternalHelper::Read() {
    if (!plugin)
      return DataStatus::NotInitializedError;


    plugin->Range(range_start, range_end);

    unsigned int max_inactivity_time = 1000;

    unsigned int wait_time = 0;
    DataStatus result = plugin->PrepareReading(max_inactivity_time, wait_time);
    if(!result.Passed()) {
      plugin->FinishReading(true);
      return result;
    }

    CheckSumAny crc;
    DataBuffer buffer(&crc, bufsize, threads*2);
    //if (!buffer) logger.msg(WARNING, "Buffer creation failed !");
    //buffer.speed.set_min_speed(min_speed, min_speed_time);
    //buffer.speed.set_min_average_speed(min_average_speed);
    buffer.speed.set_max_inactivity_time(max_inactivity_time);
    //buffer.speed.verbose(be_verbose);
    //if (be_verbose) {
    //  if (prefix)
    //    buffer.speed.verbose(std::string(prefix));
    //  else
    //    buffer.speed.verbose(verbose_prefix);
    //  buffer.speed.set_progress_indicator(show_progress);
    //}

    result = plugin->StartReading(buffer);
    if (!result.Passed()) {
      plugin->FinishReading(true);
      return result;
    }

    // Now plugin reads asynchronously. And here we read from buffer.
    while(true) {
      int handle;
      unsigned int length;
      unsigned long long int offset;
      if(!buffer.for_write(handle, length, offset, true)) {
        break;
      }
      if(length > 0) {
        // Report received content
        DataExternalComm::DataChunkClient dataEntry(buffer[handle], offset, length);
        dataEntry.write(outstream<<DataExternalComm::DataChunkTag);
      }
      buffer.is_written(handle);
    }
    buffer.eof_write(true);
    plugin->StopReading();
    plugin->FinishReading(buffer.error());

    if(buffer.error()) {
      return DataStatus::ReadError;
    }

    return DataStatus::Success;
  }

  DataStatus DataExternalHelper::Write() {
    static char dummy;

    if (!plugin)
      return DataStatus::NotInitializedError;


    plugin->Range(range_start, range_end);

    unsigned int max_inactivity_time = 1000;

    unsigned int wait_time = 0;
    DataStatus result = plugin->PrepareWriting(max_inactivity_time, wait_time);
    if(!result.Passed()) {
      plugin->FinishWriting(true);
      return result;
    }

    CheckSumAny crc;
    DataBuffer buffer(&crc, bufsize, threads*2);
    //if (!buffer) logger.msg(WARNING, "Buffer creation failed !");
    //buffer.speed.set_min_speed(min_speed, min_speed_time);
    //buffer.speed.set_min_average_speed(min_average_speed);
    buffer.speed.set_max_inactivity_time(max_inactivity_time);
    //buffer.speed.verbose(be_verbose);
    //if (be_verbose) {
    //  if (prefix)
    //    buffer.speed.verbose(std::string(prefix));
    //  else
    //    buffer.speed.verbose(verbose_prefix);
    //  buffer.speed.set_progress_indicator(show_progress);
    //}

    result = plugin->StartWriting(buffer);
    if (!result.Passed()) {
      plugin->FinishReading(true);
      return result;
    }

    // Now plugin writes asynchronously. And here we supply from buffer.
    while(true) {
      char c = DataExternalComm::InTag(instream);
      if(c != DataExternalComm::DataChunkTag) {
        logger.msg(ERROR, "failed to read data tag");
        break;
      }
      DataExternalComm::DataChunkClient dataChunk;
      logger.msg(VERBOSE, "waiting for data chunk");
      if(!dataChunk.read(instream)) {
        logger.msg(ERROR, "failed to read data chunk");
        buffer.error_read(true);
        break;
      }
      unsigned long long int chunk_offset = dataChunk.getOffset();
      unsigned long long int chunk_length = dataChunk.getSize();
      char* chunk_data = reinterpret_cast<char*>(dataChunk.get());
      if(chunk_length == 0) chunk_data = NULL;
      logger.msg(VERBOSE, "data chunk: %llu %llu", chunk_offset, chunk_length);
      // Must fit one buffer into another of different size
      while(chunk_length > 0) {
        int handle;
        unsigned int length;
        if(!buffer.for_read(handle, length, true)) {
          buffer.error_read(true);
          break;
        }
        if(length > chunk_length)
          length = chunk_length;
        std::memcpy(buffer[handle], chunk_data, length);
        buffer.is_read(handle, length, chunk_offset);
        chunk_length -= length;
        chunk_offset += length;
        chunk_data += length;
      }
      if(chunk_length > 0) break;
      if(dataChunk.getEOF()) break; // no more chunks
    }
    buffer.eof_read(true);
    plugin->StopWriting();
    plugin->FinishReading(buffer.error());

    if(buffer.error())
      return DataStatus::WriteError;

    return DataStatus::Success;
  }

  DataStatus DataExternalHelper::Stat(DataPoint::DataPointInfoType verb) {
    if (!plugin)
      return DataStatus::NotInitializedError;

    FileInfo file;
    DataStatus result = plugin->Stat(file, verb);
    if(result)
      DataExternalComm::OutEntry(outstream<<DataExternalComm::FileInfoTag, file);
    return result;
  }

  DataStatus DataExternalHelper::List(DataPoint::DataPointInfoType verb) {
    if (!plugin)
      return DataStatus::NotInitializedError;

    std::list<FileInfo> files;
    DataStatus result = plugin->List(files, verb);
    for (std::list<FileInfo>::iterator i = files.begin(); i != files.end(); ++i) {
      DataExternalComm::OutEntry(outstream<<DataExternalComm::FileInfoTag, *i);
    }
    return result;
  }

  DataStatus DataExternalHelper::Rename(const URL& newurl) {
    if (!plugin)
      return DataStatus::NotInitializedError;
    return plugin->Rename(newurl);
  }

  DataStatus DataExternalHelper::Transfer(const URL& otherendpoint, bool source, DataPoint::TransferCallback callback) {
    if (!plugin)
      return DataStatus::NotInitializedError;
    return plugin->Transfer(otherendpoint, source, callback);
  }

  DataStatus DataExternalHelper::Transfer3rdParty(const URL& source, const URL& destination, DataPoint::TransferCallback callback) {
    if (!plugin)
      return DataStatus::NotInitializedError;

    class DataPoint3rdParty: public DataPoint {
     public:
      DataStatus execute(const URL& source, const URL& destination, DataPoint::TransferCallback callback) {
        return Transfer3rdParty(source, destination, callback);
      }
    }; 

    return static_cast<DataPoint3rdParty*>(plugin)->execute(source, destination, callback);
  }

  DataExternalHelper::DataExternalHelper(char const * path, char const * name, const URL& url, const UserConfig& usercfg, std::istream& instream, std::ostream& outstream):
      plugin(NULL),
      plugins(NULL),
      instream(instream),
      outstream(outstream),
      //ftp_active(false),
      //url(url),
      //usercfg(usercfg),
      //instream(instream),
      //outstream(outstream),
      //cbarg(new CBArg(this)),
      //force_secure(true),
      //force_passive(true),
      threads(1),
      bufsize(65536),
      range_start(0),
      range_end(0),
      force_secure(false),
      force_passive(false)
      //allow_out_of_order(true),
      //credential(NULL),
      //ftp_eof_flag(false),
      //check_received_length(0),
      //data_error(false)
  {
    XMLNode cfg(NS(), "ArcConfig");
    cfg.NewChild("ModuleManager").NewChild("Path") = path;
    plugins = new PluginsFactory(cfg);
    if(plugins->load(name,"HED:DMC",name)) {
      DataPointPluginArgument arg(url, usercfg);
      plugin = dynamic_cast<DataPoint*>(plugins->get_instance("HED:DMC","",&arg,false));
    }
  } 

  DataExternalHelper::~DataExternalHelper() {
    delete plugin;
    delete plugins;
  }

} // namespace Arc

static void TransferCallback(unsigned long long int bytes_transferred) {
  Arc::DataExternalComm::TransferStatus transfer_status(bytes_transferred);
  Arc::DataExternalComm::OutEntry(std::cout<<Arc::DataExternalComm::TransferStatusTag, transfer_status);
}

int main(int argc, char* argv[]) {
  using namespace Arc;
  // Ignore some signals
  signal(SIGTTOU,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  signal(SIGHUP,SIG_IGN);
  // Debug code for setting different logging formats
  char const * log_time_format = ::getenv("ARC_LOGGER_TIME_FORMAT");
  if(log_time_format) {
    if(strcmp(log_time_format,"USER") == 0) {
      Arc::Time::SetFormat(Arc::UserTime);
    } else if(strcmp(log_time_format,"USEREXT") == 0) {
      Arc::Time::SetFormat(Arc::UserExtTime);
    } else if(strcmp(log_time_format,"ELASTIC") == 0) {
      Arc::Time::SetFormat(Arc::ElasticTime);
    } else if(strcmp(log_time_format,"MDS") == 0) {
      Arc::Time::SetFormat(Arc::MDSTime);
    } else if(strcmp(log_time_format,"ASC") == 0) {
      Arc::Time::SetFormat(Arc::ASCTime);
    } else if(strcmp(log_time_format,"ISO") == 0) {
      Arc::Time::SetFormat(Arc::ISOTime);
    } else if(strcmp(log_time_format,"UTC") == 0) {
      Arc::Time::SetFormat(Arc::UTCTime);
    } else if(strcmp(log_time_format,"RFC1123") == 0) {
      Arc::Time::SetFormat(Arc::RFC1123Time);
    } else if(strcmp(log_time_format,"EPOCH") == 0) {
      Arc::Time::SetFormat(Arc::EpochTime);
    };
  };
  // Temporary stderr destination for error messages
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  if((argc>0) && (argv[0])) Arc::ArcLocation::Init(argv[0]);
  std::list<std::string> params;
  int bufsize = 1024*1024;
  int range_start = 0;
  int range_end = 0;
  std::string logger_verbosity;
  int logger_format = -1;
  int secure = 1;
  int passive = 1;

  try {
    /* Create options parser */
    Arc::OptionParser options;
    bool version(false);
    options.AddOption('v', "version", "print version information", version);
    options.AddOption('b', "bufsize", "size of buffer (per stream)", "buffer size", bufsize);
    options.AddOption('S', "rangestart", "range start", "start", range_start);
    options.AddOption('E', "rangeend", "range end", "end", range_end);
    options.AddOption('V', "verbosity", "logger verbosity level", "level", logger_verbosity);
    options.AddOption('F', "format", "logger output format", "format", logger_format);
    options.AddOption('s', "secure", "force secure data connection", "boolean", secure);
    options.AddOption('p', "passive", "force passive data connection", "boolean", passive);

    params = options.Parse(argc, argv);
    if (params.empty()) {
      if (version) {
        std::cout << Arc::IString("%s version %s", "arc-dmc", VERSION) << std::endl;
        exit(0);
      }
      std::cerr << Arc::IString("Expecting Module, Command and URL provided");
      exit(1);
    }
  } catch(...) {
    std::cerr << "Arguments parsing error" << std::endl;
  }
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting Command module path among arguments"); exit(1);
  }
  std::string modulepath = params.front(); params.pop_front();
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting Command module name among arguments"); exit(1);
  }
  std::string modulename = params.front(); params.pop_front();
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting Command among arguments"); exit(1);
  }
  std::string command = params.front(); params.pop_front();
  if(params.empty()) {
    std::cerr << Arc::IString("Expecting URL among arguments"); exit(1);
  }
  std::string url_str = params.front(); params.pop_front();

  if(logger_format >= 0) {
    logcerr.setFormat(static_cast<Arc::LogFormat>(logger_format));
  }
  logcerr.setPrefix("[external] ");
  if(!logger_verbosity.empty()) {
    Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(logger_verbosity));
    Arc::DataExternalHelper::logger.setThreshold(Arc::string_to_level(logger_verbosity));
  }

  try {
    Arc::UserConfig usercfg;
    if(!DataExternalComm::InEntry(std::cin, usercfg)) {
      throw Arc::DataStatus(Arc::DataStatus::GenericError, "Failed to receive configuration");
    }

    DataExternalHelper* handler = new DataExternalHelper(modulepath.c_str(), modulename.c_str(), url_str, usercfg, std::cin, std::cout);
    if(!*handler) {
      throw Arc::DataStatus(Arc::DataStatus::GenericError, "Failed to start protocol handler for URL "+url_str);
    }
    handler->SetBufferSize(bufsize);
    handler->SetRange(range_start, range_end);
    handler->SetSecure(secure);
    handler->SetPassive(passive);
    Arc::DataStatus result(Arc::DataStatus::Success);
    if(command == Arc::DataPointDelegate::RenameCommand) {
      if(params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Expecting new URL among arguments");
      }
      std::string new_url_str = params.front(); params.pop_front();
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Rename(new_url_str);
    } else if(command == Arc::DataPointDelegate::ListCommand) {
      Arc::DataPoint::DataPointInfoType verb = Arc::DataPoint::INFO_TYPE_ALL;
      if(!params.empty()) {
        verb = static_cast<Arc::DataPoint::DataPointInfoType>(Arc::stringtoi(params.front()));
        params.pop_front();
      }
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->List(verb);
    } else if(command == Arc::DataPointDelegate::StatCommand) {
      Arc::DataPoint::DataPointInfoType verb = Arc::DataPoint::INFO_TYPE_ALL;
      if(!params.empty()) {
        verb = static_cast<Arc::DataPoint::DataPointInfoType>(Arc::stringtoi(params.front()));
        params.pop_front();
      }
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Stat(verb);
    } else if(command == Arc::DataPointDelegate::TransferFromCommand) {
      if(params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Expecting other URL among arguments");
      }
      std::string other_url_str = params.front(); params.pop_front();
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Transfer(other_url_str,true,&TransferCallback);
    } else if(command == Arc::DataPointDelegate::TransferToCommand) {
      if(params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Expecting other URL among arguments");
      }
      std::string other_url_str = params.front(); params.pop_front();
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Transfer(other_url_str,false,&TransferCallback);
    } else if(command == Arc::DataPointDelegate::Transfer3rdCommand) {
      if(params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Expecting other URL among arguments");
      }
      std::string other_url_str = params.front(); params.pop_front();
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      result = handler->Transfer3rdParty(url_str,other_url_str,&TransferCallback);
    } else {
      if(!params.empty()) {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unexpected arguments");
      }
      if(command == Arc::DataPointDelegate::ReadCommand) {
        result = handler->Read();
      } else if(command == Arc::DataPointDelegate::WriteCommand) {
        result = handler->Write();
      } else if(command == Arc::DataPointDelegate::CheckCommand) {
        result = handler->Check();
      } else if(command == Arc::DataPointDelegate::RemoveCommand) {
        result = handler->Remove();
      } else if(command == Arc::DataPointDelegate::MkdirCommand) {
        result = handler->CreateDirectory(false);
      } else if(command == Arc::DataPointDelegate::MkdirRecursiveCommand) {
        result = handler->CreateDirectory(true);
      } else {
        throw Arc::DataStatus(Arc::DataStatus::GenericError, "Unknown command "+command);
      }
    }
    DataExternalComm::OutEntry(std::cout<<DataExternalComm::DataStatusTag, result);
    std::cerr.flush();
    std::cout.flush();
    _exit(0);
  } catch(Arc::DataStatus const& status) {
    DataExternalComm::OutEntry(std::cout<<DataExternalComm::DataStatusTag, status);
    std::cerr.flush();
    std::cout.flush();
    _exit(0);
  } catch(...) {
  }
  std::cerr.flush();
  std::cout.flush();
  _exit(-1);
}


