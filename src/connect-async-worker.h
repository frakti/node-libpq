#ifndef NODE_LIBPQ_CONNECT_ASYNC_WORKER
#define NODE_LIBPQ_CONNECT_ASYNC_WORKER

#include "addon.h"

class ConnectAsyncWorker : public Napi::AsyncWorker {
public:
  ConnectAsyncWorker(const char* paramString, Connection* conn, Napi::Function& callback);
  virtual ~ConnectAsyncWorker(){};
  void Execute();
  void OnOK();

private:
  Connection* conn;
  const char* paramString;
};

#endif
