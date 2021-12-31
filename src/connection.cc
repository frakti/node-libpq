#include "addon.h"

Connection::Connection(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Connection>(info) {
  TRACE("Connection::Constructor");
  pq = NULL;
  lastResult = NULL;
  is_reading = false;
  is_reffed = false;
  is_success_poll_init = false;
  poll_watcher.data = this;
  fd = -1;
}

// Napi::Value NAPI_METHOD(Connection::Create) {
//   TRACE("Building new instance");
//   Connection* conn = new Connection();
//   conn->Wrap(info.This());
//
//   info.GetReturnValue().Set(info.This());
// }

Napi::Value NAPI_METHOD(Connection::ConnectSync) {
  TRACE("Connection::ConnectSync::begin");

  Connection* self = NODE_THIS();

  self->Ref();
  self->is_reffed = true;
  bool success = self->ConnectDB(info[0].As<Napi::String>().Utf8Value().c_str());

  return Napi::Boolean::New(info.Env(), success);
}

void NAPI_METHOD(Connection::Connect) {
  TRACE("Connection::Connect");

  Connection* self = NODE_THIS();

  Napi::Function callback = info[1].As<Napi::Function>()
  LOG("About to instantiate worker");
  ConnectAsyncWorker* worker = new ConnectAsyncWorker(info[0].As<Napi::String>().Utf8Value().c_str(), self, callback);
  LOG("Instantiated worker, running it...");
  self->Ref();
  self->is_reffed = true;
  worker->Queue();
}

Napi::Value NAPI_METHOD(Connection::Socket) {
  TRACE("Connection::Socket");

  Connection *self = NODE_THIS();
  int fd = PQsocket(self->pq);
  TRACEF("Connection::Socket::fd: %d\n", fd);

  return Napi::Number::New(info.Env(), fd);
}

Napi::Value NAPI_METHOD(Connection::GetLastErrorMessage) {
  Connection *self = NODE_THIS();
  char* errorMessage = PQerrorMessage(self->pq);

  return Napi::String::New(info.Env(), errorMessage);
}

void NAPI_METHOD(Connection::Finish) {
  TRACE("Connection::Finish::finish");

  Connection *self = NODE_THIS();

  if (self->is_success_poll_init) {
    int fd = PQsocket(self->pq);

    if (fd != self->fd) {
      printf("[libpq][finish][error] Socket id has changed! Prev: %d, New: %d\n", self->fd, fd);
    }

    uv_close((uv_handle_t*) &self->poll_watcher, NULL);
    self->is_reading = false;
  }
  self->ClearLastResult();

  PQfinish(self->pq);
  self->pq = NULL;
  if(self->is_reffed) {
    self->is_reffed = false;
  }
}

void NAPI_METHOD(Connection::MarkAsFinished) {
  TRACE("Connection::Finish::finish");
  Connection *self = NODE_THIS();
  self->Unref();

  printf("[libpq][MarkAsFinished] fired\n");
}

Napi::Value NAPI_METHOD(Connection::ServerVersion) {
  TRACE("Connection::ServerVersion");
  Connection* self = NODE_THIS();
  return Napi::Number::New(info.Env(), PQserverVersion(self->pq));
}

void NAPI_METHOD(Connection::Exec) {
  Connection *self = NODE_THIS();
  std::string commandText = info[0].As<Napi::String>().Utf8Value();

  TRACEF("Connection::Exec: %s\n", commandText);
  PGresult* result = PQexec(self->pq, commandText.c_str());

  self->SetLastResult(result);
}

void NAPI_METHOD(Connection::ExecParams) {
  Connection *self = NODE_THIS();

  std::string commandText = info[0].As<Napi::String>().Utf8Value();
  TRACEF("Connection::Exec: %s\n", commandText);

  Napi::Array jsParams = info[1].As<Napi::Array>();

  int numberOfParams = jsParams.Length();
  char **parameters = NewCStringArray(info.Env(), jsParams);

  PGresult* result = PQexecParams(
      self->pq,
      commandText.c_str(),
      numberOfParams,
      NULL, //const Oid* paramTypes[],
      parameters, //const char* const* paramValues[]
      NULL, //const int* paramLengths[]
      NULL, //const int* paramFormats[],
      0 //result format of text
      );

  DeleteCStringArray(parameters, numberOfParams);

  self->SetLastResult(result);
}

void NAPI_METHOD(Connection::Prepare) {
  Connection *self = NODE_THIS();

  std::string statementName = info[0].As<Napi::String>().Utf8Value();
  std::string commandText = info[1].As<Napi::String>().Utf8Value();
  int numberOfParams = info[2].As<Napi::Number>().Int32Value();

  TRACEF("Connection::Prepare: %s\n", *statementName);

  PGresult* result = PQprepare(
      self->pq,
      statementName.c_str(),
      commandText.c_str(),
      numberOfParams,
      NULL //const Oid* paramTypes[]
      );

  self->SetLastResult(result);
}

void NAPI_METHOD(Connection::ExecPrepared) {
  Connection *self = NODE_THIS();

  std::string statementName = info[0].As<Napi::String>().Utf8Value();

  TRACEF("Connection::ExecPrepared: %s\n", *statementName);

  Napi::Array jsParams = info[1].As<Napi::Array>();

  int numberOfParams = jsParams.Length();
  char** parameters = NewCStringArray(info.Env(), jsParams);

  PGresult* result = PQexecPrepared(
      self->pq,
      statementName.c_str(),
      numberOfParams,
      parameters, //const char* const* paramValues[]
      NULL, //const int* paramLengths[]
      NULL, //const int* paramFormats[],
      0 //result format of text
      );

  DeleteCStringArray(parameters, numberOfParams);

  self->SetLastResult(result);
}


void NAPI_METHOD(Connection::Clear) {
  TRACE("Connection::Clear");
  Connection *self = NODE_THIS();

  self->ClearLastResult();
}

Napi::Value NAPI_METHOD(Connection::Ntuples) {
  TRACE("Connection::Ntuples");
  Connection *self = NODE_THIS();
  PGresult* res = self->lastResult;
  int numTuples = PQntuples(res);

  return Napi::Number::New(info.Env(), numTuples);
}

Napi::Value NAPI_METHOD(Connection::Nfields) {
  TRACE("Connection::Nfields");
  Connection *self = NODE_THIS();
  PGresult* res = self->lastResult;
  int numFields = PQnfields(res);

  return Napi::Number::New(info.Env(), numFields);
}

Napi::Value NAPI_METHOD(Connection::Fname) {
  TRACE("Connection::Fname");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  char* colName = PQfname(res, info[0].As<Napi::Number>().Int32Value());

  if(colName == NULL) {
    return info.Env().Null();
  }

  return Napi::String::New(info.Env(), colName);
}

Napi::Value NAPI_METHOD(Connection::Ftype) {
  TRACE("Connection::Ftype");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  int colType = PQftype(res, info[0].As<Napi::Number>().Int32Value());

  return Napi::Number::New(info.Env(), colType);
}

Napi::Value NAPI_METHOD(Connection::Getvalue) {
  TRACE("Connection::Getvalue");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  int rowNumber = info[0].As<Napi::Number>().Int32Value();
  int colNumber = info[1].As<Napi::Number>().Int32Value();

  char* rowValue = PQgetvalue(res, rowNumber, colNumber);

  if(rowValue == NULL) {
    return info.Env().Null();
  }

  return Napi::String::New(info.Env(), rowValue);
}

Napi::Value NAPI_METHOD(Connection::Getisnull) {
  TRACE("Connection::Getisnull");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  int rowNumber = info[0].As<Napi::Number>().Int32Value();
  int colNumber = info[1].As<Napi::Number>().Int32Value();

  int rowValue = PQgetisnull(res, rowNumber, colNumber);

  return Napi::Boolean::New(info.Env(), rowValue == 1);
}

Napi::Value NAPI_METHOD(Connection::CmdStatus) {
  TRACE("Connection::CmdStatus");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;
  char* status = PQcmdStatus(res);

  return Napi::String::New(info.Env(), status);
}

Napi::Value NAPI_METHOD(Connection::CmdTuples) {
  TRACE("Connection::CmdTuples");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;
  char* tuples = PQcmdTuples(res);

  return Napi::String::New(info.Env(), tuples);
}

Napi::Value NAPI_METHOD(Connection::ResultStatus) {
  TRACE("Connection::ResultStatus");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  char* status = PQresStatus(PQresultStatus(res));

  return Napi::String::New(info.Env(), status);
}

Napi::Value NAPI_METHOD(Connection::ResultErrorMessage) {
  TRACE("Connection::ResultErrorMessage");
  Connection *self = NODE_THIS();

  PGresult* res = self->lastResult;

  char* resultErrorMessage = PQresultErrorMessage(res);

  return Napi::String::New(info.Env(), resultErrorMessage);
}

# define SET_E(key, name) \
  field = PQresultErrorField(self->lastResult, key); \
  if(field != NULL) { \
    result.Set(name, field); \
  }

Napi::Value NAPI_METHOD(Connection::ResultErrorFields) {
  Connection *self = NODE_THIS();

  if(self->lastResult == NULL) {
    return info.Env().Null();
  }

  Napi::Object result = Napi::Object::New(info.Env());
  char* field;
  SET_E(PG_DIAG_SEVERITY, "severity");
  SET_E(PG_DIAG_SQLSTATE, "sqlState");
  SET_E(PG_DIAG_MESSAGE_PRIMARY, "messagePrimary");
  SET_E(PG_DIAG_MESSAGE_DETAIL, "messageDetail");
  SET_E(PG_DIAG_MESSAGE_HINT, "messageHint");
  SET_E(PG_DIAG_STATEMENT_POSITION, "statementPosition");
  SET_E(PG_DIAG_INTERNAL_POSITION, "internalPosition");
  SET_E(PG_DIAG_INTERNAL_QUERY, "internalQuery");
  SET_E(PG_DIAG_CONTEXT, "context");
#ifdef MORE_ERROR_FIELDS_SUPPORTED
  SET_E(PG_DIAG_SCHEMA_NAME, "schemaName");
  SET_E(PG_DIAG_TABLE_NAME, "tableName");
  SET_E(PG_DIAG_COLUMN_NAME, "columnName");
  SET_E(PG_DIAG_DATATYPE_NAME, "dataTypeName");
  SET_E(PG_DIAG_CONSTRAINT_NAME, "constraintName");
#endif
  SET_E(PG_DIAG_SOURCE_FILE, "sourceFile");
  SET_E(PG_DIAG_SOURCE_LINE, "sourceLine");
  SET_E(PG_DIAG_SOURCE_FUNCTION, "sourceFunction");

  return result;
}

Napi::Value NAPI_METHOD(Connection::SendQuery) {
  TRACE("Connection::SendQuery");

  Connection *self = NODE_THIS();
  std::string commandText = info[0].As<Napi::String>().Utf8Value();

  TRACEF("Connection::SendQuery: %s\n", commandText); // TODO test this
  int success = PQsendQuery(self->pq, commandText.c_str());

  return Napi::Boolean::New(info.Env(), success == 1);
}

Napi::Value NAPI_METHOD(Connection::SendQueryParams) {
  TRACE("Connection::SendQueryParams");

  Connection *self = NODE_THIS();

  std::string commandText = info[0].As<Napi::String>().Utf8Value();
  TRACEF("Connection::SendQueryParams: %s\n", commandText);

  Napi::Array jsParams = info[1].As<Napi::Array>();

  int numberOfParams = jsParams.Length();
  char** parameters = NewCStringArray(info.Env(), jsParams);

  int success = PQsendQueryParams(
      self->pq,
      commandText.c_str(),
      numberOfParams,
      NULL, //const Oid* paramTypes[],
      parameters, //const char* const* paramValues[]
      NULL, //const int* paramLengths[]
      NULL, //const int* paramFormats[],
      0 //result format of text
      );

  DeleteCStringArray(parameters, numberOfParams);

  return Napi::Boolean::New(info.Env(), success == 1);
}

Napi::Value NAPI_METHOD(Connection::SendPrepare) {
  TRACE("Connection::SendPrepare");

  Connection *self = NODE_THIS();

  std::string statementName = info[0].As<Napi::String>().Utf8Value();
  std::string commandText = info[1].As<Napi::String>().Utf8Value();
  int numberOfParams = info[2].As<Napi::Number>().Int32Value();

  TRACEF("Connection::SendPrepare: %s\n", statementName);
  int success = PQsendPrepare(
      self->pq,
      statementName.c_str(),
      commandText.c_str(),
      numberOfParams,
      NULL //const Oid* paramTypes
      );

  return Napi::Boolean::New(info.Env(), success == 1);
}

Napi::Value NAPI_METHOD(Connection::SendQueryPrepared) {
  TRACE("Connection::SendQueryPrepared");

  Connection *self = NODE_THIS();

  std::string statementName = info[0].As<Napi::String>().Utf8Value();
  TRACEF("Connection::SendQueryPrepared: %s\n", statementName);

  Napi::Array jsParams = info[1].As<Napi::Array>();

  int numberOfParams = jsParams.Length();
  char** parameters = NewCStringArray(info.Env(), jsParams);

  int success = PQsendQueryPrepared(
      self->pq,
      statementName.c_str(),
      numberOfParams,
      parameters, //const char* const* paramValues[]
      NULL, //const int* paramLengths[]
      NULL, //const int* paramFormats[],
      0 //result format of text
      );

  DeleteCStringArray(parameters, numberOfParams);

  return Napi::Boolean::New(info.Env(), success == 1);
}

Napi::Value NAPI_METHOD(Connection::GetResult) {
  TRACE("Connection::GetResult");

  Connection *self = NODE_THIS();
  PGresult *result = PQgetResult(self->pq);

  if(result == NULL) {
    return Napi::Boolean::New(info.Env(), false);
  }

  self->SetLastResult(result);
  return Napi::Boolean::New(info.Env(), true);
}

Napi::Value NAPI_METHOD(Connection::ConsumeInput) {
  TRACE("Connection::ConsumeInput");

  Connection *self = NODE_THIS();

  int success = PQconsumeInput(self->pq);
  return Napi::Boolean::New(info.Env(), success == 1);
}

Napi::Value NAPI_METHOD(Connection::IsBusy) {
  TRACE("Connection::IsBusy");

  Connection *self = NODE_THIS();

  int isBusy = PQisBusy(self->pq);
  TRACEF("Connection::IsBusy: %d\n", isBusy);

  return Napi::Boolean::New(info.Env(), isBusy == 1);
}

void NAPI_METHOD(Connection::StartRead) {
  TRACE("Connection::StartRead");

  Connection* self = NODE_THIS();

  self->ReadStart();
}

void NAPI_METHOD(Connection::StopRead) {
  TRACE("Connection::StopRead");

  Connection* self = NODE_THIS();

  self->ReadStop();
}

void NAPI_METHOD(Connection::StartWrite) {
  TRACE("Connection::StartWrite");

  Connection* self = NODE_THIS();

  self->WriteStart();
}

Napi::Value NAPI_METHOD(Connection::SetNonBlocking) {
  TRACE("Connection::SetNonBlocking");

  Connection* self = NODE_THIS();

  int ok = PQsetnonblocking(self->pq, info[0].As<Napi::Number>().Int32Value());

  return Napi::Boolean::New(info.Env(), ok == 1);
}

Napi::Value NAPI_METHOD(Connection::IsNonBlocking) {
  TRACE("Connection::IsNonBlocking");

  Connection* self = NODE_THIS();

  int status = PQisnonblocking(self->pq);

  return Napi::Boolean::New(info.Env(), status == 1);
}

Napi::Value NAPI_METHOD(Connection::Flush) {
  TRACE("Connection::Flush");

  Connection* self = NODE_THIS();

  int status = PQflush(self->pq);

  return Napi::Number::New(info.Env(), status);
}

#ifdef ESCAPE_SUPPORTED
Napi::Value NAPI_METHOD(Connection::EscapeLiteral) {
  TRACE("Connection::EscapeLiteral");

  Connection* self = NODE_THIS();

  std::string str = info[0].As<Napi::String>().Utf8Value();

  TRACEF("Connection::EscapeLiteral:input %s\n", str);
  char* result = PQescapeLiteral(self->pq, str.c_str(), str.length());
  TRACEF("Connection::EscapeLiteral:output %s\n", result);

  if(result == NULL) {
    return info.Env().Null();
  }

  Napi::String returnedResult = Napi::String::New(info.Env(), result);
  PQfreemem(result);
  return returnedResult;
}

Napi::Value NAPI_METHOD(Connection::EscapeIdentifier) {
  TRACE("Connection::EscapeIdentifier");

  Connection* self = NODE_THIS();

  std::string str = info[0].As<Napi::String>();

  TRACEF("Connection::EscapeIdentifier:input %s\n", str.c_str());
  char* result = PQescapeIdentifier(self->pq, str.c_str(), str.length());
  TRACEF("Connection::EscapeIdentifier:output %s\n", result);

  if(result == NULL) {
    return info.Env().Null();
  }

  Napi::String returnedResult = Napi::String::New(info.Env(), result);
  PQfreemem(result);
  return returnedResult;
}
#endif

Napi::Value NAPI_METHOD(Connection::Notifies) {
  LOG("Connection::Notifies");

  Connection* self = NODE_THIS();

  PGnotify* msg = PQnotifies(self->pq);

  if(msg == NULL) {
    LOG("No notification");
    return info.Env().Null();
  }

  Napi::Object result = Napi::Object::New(info.Env());
  result.Set("relname", msg->relname);
  result.Set("extra", msg->extra);
  result.Set("be_pid", msg->be_pid);

  PQfreemem(msg);

  return result;
};

// Napi::Value NAPI_METHOD(Connection::PutCopyData) {
//   LOG("Connection::PutCopyData");
//
//   Connection* self = NODE_THIS();
//
//   Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>(); /// TODO SPECIFY <T>
//
//   char* data = node::Buffer::Data(buffer);
//   int length = node::Buffer::Length(buffer);
//
//   int result = PQputCopyData(self->pq, data, length);
//
//   return Napi::Number::New(info.Env(), result);
// }

Napi::Value NAPI_METHOD(Connection::PutCopyEnd) {
  LOG("Connection::PutCopyEnd");

  Connection* self = NODE_THIS();

  //optional error message

  bool sendErrorMessage = info.Length() > 0;
  int result;
  if(sendErrorMessage) {
    std::string msg = info[0].As<Napi::String>();
    TRACEF("Connection::PutCopyEnd:%s\n", msg);
    result = PQputCopyEnd(self->pq, msg.c_str());
  } else {
    result = PQputCopyEnd(self->pq, NULL);
  }

  return Napi::Number::New(info.Env(), result);
}

static void FreeBuffer(char *buffer, void *) {
  PQfreemem(buffer);
}

// TEMPORARLY COMMENTED OUT TO VERIFY IF SEGFAULT IS GONE, ONCE CONFIRMED THAT WILL REVWIRTE THOSE METHODS TO NAPI

// Napi::Value NAPI_METHOD(Connection::GetCopyData) {
//   LOG("Connection::GetCopyData");
//
//   Connection* self = NODE_THIS();
//
//   char* buffer = NULL;
//   int async = info[0]->IsTrue() ? 1 : 0; // TODO check if this works
//
//   TRACEF("Connection::GetCopyData:async %d\n", async);
//
//   int length = PQgetCopyData(self->pq, &buffer, async);
//
//   //some sort of failure or not-ready condition
//   if(length < 1) {
//     return Napi::Number::New(info.Env(), length);
//   }
//
//   return Napi::Buffer::New(info.Env(), buffer, length, FreeBuffer, NULL);
// }

// Napi::Value NAPI_METHOD(Connection::Cancel) { ////////// TODO
//   LOG("Connection::Cancel");
//
//   Connection* self = NODE_THIS();
//
//   PGcancel *cancelStuct = PQgetCancel(self->pq);
//
//   if(cancelStuct == NULL) {
// Napi::Error::New(env, uv_strerror(status)).Value()

//     info.GetReturnValue().Set(Nan::Error("Unable to allocate cancel struct"));
//     return;
//   }
//
//   char* errBuff = new char[255];
//
//   LOG("PQcancel");
//   int result = PQcancel(cancelStuct, errBuff, 255);
//
//   LOG("PQfreeCancel");
//   PQfreeCancel(cancelStuct);
//
//   if(result == 1) {
//     delete[] errBuff;
//     return info.GetReturnValue().Set(true);
//   }
//
//   info.GetReturnValue().Set(Nan::New(errBuff).ToLocalChecked());
//   delete[] errBuff;
// }

bool Connection::ConnectDB(const char* paramString) {
  TRACEF("Connection::ConnectDB:Connection parameters: %s\n", paramString);
  this->pq = PQconnectdb(paramString);

  ConnStatusType status = PQstatus(this->pq);

  if(status != CONNECTION_OK) {
    return false;
  }

  int fd = PQsocket(this->pq);
  this->fd = fd;
  int socketInitStatus = uv_poll_init_socket(uv_default_loop(), &(this->poll_watcher), fd);

  if (socketInitStatus == 0) {
    is_success_poll_init = true;
  }

  TRACE("Connection::ConnectSync::Success");
  return true;
}

char * Connection::ErrorMessage() {
  return PQerrorMessage(this->pq);
}

void Connection::on_io_readable(uv_poll_t* handle, int status, int revents) {
  LOG("Connection::on_io_readable");
  TRACEF("Connection::on_io_readable:status %d\n", status);
  TRACEF("Connection::on_io_readable:revents %d\n", revents);
  if(revents & UV_READABLE) {
    LOG("Connection::on_io_readable UV_READABLE");
    Connection* self = (Connection*) handle->data;
    LOG("Got connection pointer");
    self->Emit("readable");
  }
}

void Connection::on_io_writable(uv_poll_t* handle, int status, int revents) {
  LOG("Connection::on_io_writable");
  TRACEF("Connection::on_io_writable:status %d\n", status);
  TRACEF("Connection::on_io_writable:revents %d\n", revents);
  if(revents & UV_WRITABLE) {
    LOG("Connection::on_io_readable UV_WRITABLE");
    Connection* self = (Connection*) handle->data;
    self->WriteStop();
    self->Emit("writable");
  }
}

void Connection::ReadStart() {
  LOG("Connection::ReadStart:starting read watcher");
  is_reading = true;
  int pollStartStatus = uv_poll_start(&poll_watcher, UV_READABLE, on_io_readable);
  if (pollStartStatus != 0) {
    printf("[lippq][readStart][error] Non-zero ReadPoolStart status. Status: %s\n", uv_strerror(pollStartStatus));
  }
  LOG("Connection::ReadStart:started read watcher");
}

void Connection::ReadStop() {
  LOG("Connection::ReadStop:stoping read watcher");
  if(!is_reading) return;
  is_reading = false;
  int pollStopStatus = uv_poll_stop(&poll_watcher);
  if (pollStopStatus != 0) {
    printf("[lippq][readStop][error] Non-zero ReadPoolStop status. Status: %s\n", uv_strerror(pollStopStatus));
  }
  LOG("Connection::ReadStop:stopped read watcher");
}

void Connection::WriteStart() {
  LOG("Connection::WriteStart:starting write watcher");
  int pollStartStatus = uv_poll_start(&poll_watcher, UV_WRITABLE, on_io_writable);
  if (pollStartStatus != 0) {
    printf("[lippq][writeStart][error] Non-zero WritePoolStart status. Status: %s\n", uv_strerror(pollStartStatus));
  }
  LOG("Connection::WriteStart:started write watcher");
}

void Connection::WriteStop() {
  LOG("Connection::WriteStop:stoping write watcher");
  int pollStopStatus = uv_poll_stop(&poll_watcher);
  if (pollStopStatus != 0) {
    printf("[lippq][writeStop][error] Non-zero WritePoolStop status. Status: %s\n", uv_strerror(pollStopStatus));
  }
}


void Connection::ClearLastResult() {
  LOG("Connection::ClearLastResult");
  if(lastResult == NULL) return;
  PQclear(lastResult);
  lastResult = NULL;
}

void Connection::SetLastResult(PGresult* result) {
  LOG("Connection::SetLastResult");
  ClearLastResult();
  lastResult = result;
}

char* Connection::NewCString(Napi::Env env, Napi::Value val) {
  Napi::HandleScope scope(env);

  std::string str = val.As<Napi::String>();
  char* buffer = new char[str.length() + 1];
  strcpy(buffer, str.c_str());

  return buffer;
}

char** Connection::NewCStringArray(Napi::Env env, Napi::Array jsParams) {
  Napi::HandleScope scope(env);

  int numberOfParams = jsParams.Length();

  char** parameters = new char*[numberOfParams];

  for(int i = 0; i < numberOfParams; i++) {
    Napi::Value val = jsParams[i];
    // v8::Local<v8::Value> val = Nan::Get(jsParams, i).ToLocalChecked();
    if(val.IsNull()) {
      parameters[i] = NULL;
      continue;
    }
    //expect every other value to be a string...
    //make sure aggresive type checking is done
    //on the JavaScript side before calling
    parameters[i] = NewCString(env, val);
  }

  return parameters;
}

void Connection::DeleteCStringArray(char** array, int length) {
  for(int i = 0; i < length; i++) {
    delete [] array[i];
  }
  delete [] array;
}

void Connection::Emit(const char* message) {
  Napi::Env env = this->Env();
  Napi::HandleScope scope(env);

  TRACE("ABOUT TO EMIT EVENT");
  Napi::Function emit = this->Value().As<Napi::Object>().Get("emit").As<Napi::Function>();
  emit.Call(this->Value(), { Napi::String::New(env, message) });


  // v8::Local<v8::Object> jsInstance = handle();
  // TRACE("GETTING 'emit' FUNCTION INSTANCE");
  // v8::Local<v8::Value> emit_v = Nan::Get(jsInstance, Nan::New<v8::String>("emit").ToLocalChecked()).ToLocalChecked();
  // assert(emit_v->IsFunction());
  // v8::Local<v8::Function> emit_f = emit_v.As<v8::Function>();

  // v8::Local<v8::String> eventName = ;
  // v8::Local<v8::Value> info[1] = {
  //   Nan::New<v8::String>(message).ToLocalChecked()
  // };

  // TRACE("CALLING EMIT");
  // Nan::TryCatch tc;
  // Nan::AsyncResource *async_emit_f = new Nan::AsyncResource("libpq:connection:emit");
  // async_emit_f->runInAsyncScope(handle(), "emit", 1, info);
  // delete async_emit_f;
  // if(tc.HasCaught()) {
  //   Nan::FatalException(tc);
  // }
}
