#include "job.h"

bool ARexJob::is_allowed(void) {
  return false;
}

ARexJob::ARexJob(const std::string& id):id_(id),allowed_to_see(false),allowed_to_maintain(false) {
  if(!is_allowed()) { id.clear(); return; };
  if(!(allowed_to_see_ || allowed_to_maintain_)) { id.clear(); return; };
}

ARexJob::ARexJob(XMLNode jsdl) {
}

void ARexJob::GetDescription(XMLNode& jsdl) {
}

bool ARexJob::Cancel(void) {
  if(id_.empty()) return false;
  return false;
}

bool ARexJob::Resume(void) {
  if(id_.empty()) return false;
  return false;
}

std::string State(void) {
  if(id_.empty()) return "";
  return "";
}

