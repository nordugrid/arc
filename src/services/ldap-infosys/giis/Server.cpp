#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <cstdlib>

#include "Server.h"
#include "Index.h"

static void Error(const std::string& file, int code, const std::string& info) {
  int fd = open(file.c_str(), O_WRONLY);
  if(fd != -1) {
    FILE *f = fdopen(fd, "w");
    fprintf(f, "RESULT\n");
    fprintf(f, "info:%s\n", info.c_str());
    fprintf(f, "code:%d\n", code);
    fclose(f);
  }
}

struct process_arg_t {
  Server *server;
  std::string file;
  std::list<std::string> query;
};

void* process(void *a) {
  process_arg_t *arg = (process_arg_t *)a;
  if(arg->query.empty())
    Error(arg->file, 53, "Empty query");
  else if(*arg->query.begin() == "BIND")
    arg->server->Bind(arg->file);
  else if(*arg->query.begin() == "ADD")
    arg->server->Add(arg->file, arg->query);
  else if(*arg->query.begin() == "SEARCH")
    arg->server->Search(arg->file, arg->query);
  else
    Error(arg->file, 53, "Unimplemented query type: " + *arg->query.begin());
  delete arg;
  return NULL;
}

Server::Server(const std::string& fifo, const std::string& conf) : fifo(fifo) {
  mkfifo(fifo.c_str(), S_IRUSR | S_IWUSR);
  std::ifstream cfg(conf.c_str());
  std::string line;
  while(getline(cfg, line)) {
    if(line[0] == '[' && line[line.size() - 1] == ']') {
      while(!line.empty() &&
	    line.substr(1, 14) == "infosys/index/" &&
	    line.find('/', 15) == std::string::npos) {
	Index ix(line.substr(15, line.size() - 16));
	while(getline(cfg, line)) {
	  if(line[0] == '[' && line[line.size() - 1] == ']')
	    break;
	  if(line.substr(0, 9) == "allowreg=") {
	    if(line[9] == '"' && line[line.size() - 1] == '"')
	      ix.AllowReg(line.substr(10, line.size() - 11));
	    else
	      ix.AllowReg(line.substr(9));
	  }
	}
	indices.push_back(ix);
      }
    }
  }
}

Server::~Server() {
  unlink(fifo.c_str());
}

void Server::Start() {
  bool running = true;
  int sfd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK);
  FILE *sf = fdopen(sfd, "r");
  while(running) {
    fd_set fs;
    FD_ZERO(&fs);
    FD_SET(sfd, &fs);
    if(select(sfd + 1, &fs, NULL, NULL, NULL) > 0) {
      char buf[2048];
      if (!fgets(buf, 2048, sf)) {
        running = false;
        fclose(sf);
        return;
      };
      std::string file(buf, strlen(buf) - 1);
      if(file == "STOP")
	running = false;
      else {
	process_arg_t *arg = new process_arg_t;
	arg->server = this;
	arg->file = file;
	std::string line;
	while(fgets(buf, 2048, sf)) {
	  if(buf[0] == ' ')
	    line.append(&buf[1], strlen(buf) - 2);
	  else {
	    if(!line.empty())
	      arg->query.push_back(line);
	    line = std::string(buf, strlen(buf) - 1);
	  }
	}
	fclose(sf);
	sfd = open(fifo.c_str(), O_RDONLY | O_NONBLOCK);
	sf = fdopen(sfd, "r");
	if(!line.empty())
	  arg->query.push_back(line);
	pthread_t pid;
	pthread_create(&pid, NULL, process, arg);
	pthread_detach(pid);
      }
    }
  }
  fclose(sf);
}

void Server::Bind(const std::string& file) {
  Error(file, 0, "Binding");
}

void Server::Add(const std::string& file,
		 const std::list<std::string>& query) {
  std::string name;

  for(std::list<std::string>::const_iterator it = query.begin();
      it != query.end(); it++)
    if(it->substr(0, 8) == "suffix: ") {
      std::string::size_type pos1 = it->find("Mds-Vo-name=", 8);
      if(pos1 != std::string::npos) {
	pos1 += 12;
	std::string::size_type pos2 = it->find(',', pos1);
	if(pos2 != std::string::npos)
	  name = it->substr(pos1, pos2 - pos1);
	else
	  name = it->substr(pos1);
	break;
      }
    }

  if(name.empty()) {
    Error(file, 53, "MDS VO name not found in LDAP suffix");
    return;
  }

  std::list<Index>::iterator ix;
  for(ix = indices.begin(); ix != indices.end(); ix++)
    if(strcasecmp(ix->Name().c_str(), name.c_str()) == 0)
      break;
  if(ix == indices.end()) {
    Error(file, 53, "No such index (" + name + ")");
    return;
  }

  Entry e(query);
  if(!e) {
    Error(file, 53, "Incomplete query");
    return;
  }

  if(!ix->AddEntry(e)) {
    Error(file, 53, "Not authorized to add - check policies");
    return;
  }

  Error(file, 0, "Registration succeded");
}

void Server::Search(const std::string& file,
		    const std::list<std::string>& query) {
  std::string name;
  int scope = 0;
  std::string attrs;

  for(std::list<std::string>::const_iterator it = query.begin();
      it != query.end(); it++)
    if(it->substr(0, 8) == "suffix: ") {
      std::string::size_type pos1 = it->find("Mds-Vo-name=", 8);
      if(pos1 != std::string::npos) {
	pos1 += 12;
	std::string::size_type pos2 = it->find(',', pos1);
	if(pos2 != std::string::npos)
	  name = it->substr(pos1, pos2 - pos1);
	else
	  name = it->substr(pos1);
      }
    }
    else if(it->substr(0, 7) == "scope: ")
      scope = atoi(it->substr(7).c_str());
    else if(it->substr(0, 7) == "attrs: ")
      attrs = it->substr(7);

  if(name.empty()) {
    Error(file, 53, "MDS VO name not found in LDAP suffix");
    return;
  }

  if(scope != 0) {
    Error(file, 53, "Search scope not supported");
    return;
  }

  // if(attrs != "giisregistrationstatus") {
  //   Error(file, 53, "Unsupported query attribute (" + attrs + +")");
  //   return;
  // }

  std::list<Index>::iterator ix;
  for(ix = indices.begin(); ix != indices.end(); ix++)
    if(strcasecmp(ix->Name().c_str(), name.c_str()) == 0)
      break;
  if(ix == indices.end()) {
    Error(file, 53, "No such index (" + name + ")");
    return;
  }

  int fd = open(file.c_str(), O_WRONLY);
  if(fd != -1) {
    FILE *f = fdopen(fd, "w");
    ix->ListEntries(f);
    fprintf(f, "RESULT\n");
    fprintf(f, "info:Successful query\n");
    fprintf(f, "code:0\n");
    fclose(f);
  }
}
