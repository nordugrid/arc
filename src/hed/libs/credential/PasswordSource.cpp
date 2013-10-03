#include <iostream>
#include <string.h>

#include <openssl/ui.h>
#include <openssl/err.h>

#include "PasswordSource.h"

namespace Arc {

static int ssl_err_cb(const char *str, size_t, void *u) {
  std::cerr << "OpenSSL error string: " << str << std::endl;
  return 1;
}

PasswordSource::Result PasswordSourceNone::Get(std::string& password, int minsize, int maxsize) {
  return NO_PASSWORD;
}

PasswordSourceString::PasswordSourceString(const std::string& password):
                                                         password_(password) {
}

PasswordSource::Result PasswordSourceString::Get(std::string& password, int minsize, int maxsize) {
  password = password_;
  return PASSWORD;
}

PasswordSourceStream::PasswordSourceStream(std::istream* password):
                                                         password_(password) {
}

PasswordSource::Result PasswordSourceStream::Get(std::string& password, int minsize, int maxsize) {
  if(!password_) return NO_PASSWORD;
  std::getline(*password_, password);
  return PASSWORD;
}

PasswordSourceInteractive::PasswordSourceInteractive(const std::string& prompt, bool verify):
                                                          prompt_(prompt),verify_(verify) {
}

PasswordSource::Result PasswordSourceInteractive::Get(std::string& password, int minsize, int maxsize) {
  UI *ui = UI_new();
  if (!ui) return CANCEL;

  int res = 0;
  int ok = 0;
  char *buf1 = NULL;
  char *buf2 = NULL;
  int ui_flags = 0;
  char *prompt = NULL;
  int bufsiz = maxsize;

  if(bufsiz <= 0) bufsiz = 256;
  ++bufsiz; // for \0

  prompt = UI_construct_prompt(ui, "pass phrase", prompt_.c_str());
  ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
//  UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);

  if (ok >= 0) {
    ok = -1;
    if((buf1 = (char *)OPENSSL_malloc(bufsiz)) != NULL) {
      memset(buf1,0,(unsigned int)bufsiz);
      ok = UI_add_input_string(ui,prompt,ui_flags,buf1,(minsize>0)?minsize:0,bufsiz-1);
    }
  }
  if (ok >= 0 && verify_) {
    ok = -1;
    if((buf2 = (char *)OPENSSL_malloc(bufsiz)) != NULL) {
      memset(buf2,0,(unsigned int)bufsiz);
      ok = UI_add_verify_string(ui,prompt,ui_flags,buf2,(minsize>0)?minsize:0,bufsiz-1,buf1);
    }
  }
  if (ok >= 0) do {
    ok = UI_process(ui);
    if(ok == -2) break; // Abort request
    if(ok == -1) { // Password error
      unsigned long errcode = ERR_get_error();
      const char* errstr = ERR_reason_error_string(errcode);
      if(errstr == NULL) {
        std::cerr << "Password input error - code " << errcode << std::endl;
      } else if(strstr(errstr,"result too small")) {
        std::cerr << "Password is too short, need at least " << minsize << " charcters" << std::endl;
      } else if(strstr(errstr,"result too large")) {
        std::cerr << "Password is too long, need at most " << bufsiz-1 << " characters" << std::endl;
      } else {
        std::cerr << errstr << std::endl;
      };
    };
  } while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));

  if (buf2){
    memset(buf2,0,(unsigned int)bufsiz);
    OPENSSL_free(buf2);
  }

  if (ok >= 0) {
    if(buf1) {
      buf1[bufsiz-1] = 0;
      res = strlen(buf1);
      password.assign(buf1,res);
    }
  }

  if (buf1){
    memset(buf1,0,(unsigned int)bufsiz);
    OPENSSL_free(buf1);
  }

  if (ok == -1){
    std::cerr << "User interface error" << std::endl;
    ERR_print_errors_cb(&ssl_err_cb, NULL);
    memset((void*)password.c_str(),0,(unsigned int)password.length());
    password.resize(0);
    res = 0;
  } else if (ok == -2) {
    memset((void*)password.c_str(),0,(unsigned int)password.length());
    password.resize(0);
    res = 0;
  }
  UI_free(ui);
  OPENSSL_free(prompt);

  return (res>0)?PasswordSource::PASSWORD:PasswordSource::CANCEL;
}

} // namespace Arc

