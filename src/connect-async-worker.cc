//helper class to perform async connection
#include "addon.h"

ConnectAsyncWorker::ConnectAsyncWorker(std::string paramString, Connection* conn, Napi::Function& callback)
  : Napi::AsyncWorker(callback), conn(conn), paramString(paramString) { }

  //this method fires within the threadpool and does not
  //block the main node run loop
  void ConnectAsyncWorker::Execute() {
    TRACE("ConnectAsyncWorker::Execute");

    bool success = conn->ConnectDB(paramString.c_str());

    if(!success) {
      SetError(conn->ErrorMessage());
    }
  }
