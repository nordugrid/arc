#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <string>

class Index;

class Server {
 public:
  Server(const std::string& fifo, const std::string& conf);
  ~Server();
  void Start();
  void Bind(const std::string& file);
  void Add(const std::string& file, const std::list<std::string>& query);
  void Search(const std::string& file, const std::list<std::string>& query);
 private:
  const std::string fifo;
  std::list<Index> indices;
};

#endif // SERVER_H
