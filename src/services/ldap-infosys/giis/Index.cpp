#include "Index.h"

#include <cstdio>

Index::Index(const std::string& name) : name(name) {
  pthread_mutex_init(&lock, NULL);
}

Index::Index(const Index& ix) : name(ix.name),
				policies(ix.policies),
				entries(ix.entries) {
  pthread_mutex_init(&lock, NULL);  
}

Index::~Index() {
  pthread_mutex_destroy(&lock);
}

const std::string& Index::Name() const {
  return name;
}

void Index::ListEntries(FILE *f) {
  pthread_mutex_lock(&lock);
  for(std::list<Entry>::const_iterator e = entries.begin();
      e != entries.end(); e++)
    fprintf(f, e->SearchEntry().c_str());
  pthread_mutex_unlock(&lock);
}

bool Index::AddEntry(const Entry& entry) {
  bool accept = false;
  for(std::list<Policy>::const_iterator p = policies.begin();
      p != policies.end(); p++)
    if(p->Check(entry.Host(), entry.Port(), entry.Suffix())) {
      accept = true;
      break;
    }
  if(accept) {
    bool found = false;
    pthread_mutex_lock(&lock);
    for(std::list<Entry>::iterator e = entries.begin();
	e != entries.end(); e++) {
      if(e->Host() == entry.Host() &&
	 e->Port() == entry.Port() &&
	 e->Suffix() == entry.Suffix()) {
	*e = entry;
	found = true;
	break;
      }
    }
    if(!found)
      entries.push_back(entry);
    pthread_mutex_unlock(&lock);
  }
  return accept;
}

void Index::AllowReg(const std::string& areg) {
  policies.push_back(areg);
}
