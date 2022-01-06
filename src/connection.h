#ifndef NODE_LIBPQ_CONNECTION_V2
#define NODE_LIBPQ_CONNECTION_V2

#include <napi.h>
#include <libpq-fe.h>

#define NAPI_METHOD(name)                                                       \
    name(const Napi::CallbackInfo& info)

class Connection : public Napi::ObjectWrap<Connection> {
  public:
    Connection(const Napi::CallbackInfo& info);
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    bool ConnectDB(const char* paramString);
    bool ConnectDB_v2(const char* paramString);
    void InitiateSocket();
    char* ErrorMessage();

  private:
    PGresult* lastResult;
    PGconn* pq;
    uv_poll_t poll_watcher;
    bool is_reffed;
    bool is_reading;
    bool is_success_poll_init;
    int fd;

    // static Napi::Value NAPI_METHOD(Create);
    Napi::Value NAPI_METHOD(ConnectSync); // Boolean
    void NAPI_METHOD(Connect);
    Napi::Value NAPI_METHOD(ServerVersion);
    Napi::Value NAPI_METHOD(Socket);
    Napi::Value NAPI_METHOD(GetLastErrorMessage);
    void NAPI_METHOD(Finish);
    void NAPI_METHOD(MarkAsFinished);
    void NAPI_METHOD(Exec);
    void NAPI_METHOD(ExecParams);
    void NAPI_METHOD(Prepare);
    void NAPI_METHOD(ExecPrepared);
    void NAPI_METHOD(Clear);
    Napi::Value NAPI_METHOD(Ntuples);
    Napi::Value NAPI_METHOD(Nfields);
    Napi::Value NAPI_METHOD(Fname); // string or null
    Napi::Value NAPI_METHOD(Ftype);
    Napi::Value NAPI_METHOD(Getvalue); // string or null
    Napi::Value NAPI_METHOD(Getisnull); // Boolean
    Napi::Value NAPI_METHOD(CmdStatus);
    Napi::Value NAPI_METHOD(CmdTuples);
    Napi::Value NAPI_METHOD(ResultStatus);
    Napi::Value NAPI_METHOD(ResultErrorMessage);
    Napi::Value NAPI_METHOD(ResultErrorFields); // object or null
    Napi::Value NAPI_METHOD(SendQuery); // Boolean
    Napi::Value NAPI_METHOD(SendQueryParams); // Boolean
    Napi::Value NAPI_METHOD(SendPrepare); // Boolean
    Napi::Value NAPI_METHOD(SendQueryPrepared); // Boolean
    Napi::Value NAPI_METHOD(GetResult); // Boolean
    Napi::Value NAPI_METHOD(ConsumeInput); // Boolean
    Napi::Value NAPI_METHOD(IsBusy); // Boolean
    void NAPI_METHOD(StartRead);
    void NAPI_METHOD(StopRead);
    void NAPI_METHOD(StartWrite);
    Napi::Value NAPI_METHOD(SetNonBlocking); // Boolean
    Napi::Value NAPI_METHOD(IsNonBlocking); // Boolean
    Napi::Value NAPI_METHOD(Flush);
#ifdef ESCAPE_SUPPORTED
    Napi::Value NAPI_METHOD(EscapeLiteral); // string or null
    Napi::Value NAPI_METHOD(EscapeIdentifier); // string or null
#endif
    Napi::Value NAPI_METHOD(Notifies); // object or null
    // Napi::Value NAPI_METHOD(PutCopyData);
    Napi::Value NAPI_METHOD(PutCopyEnd);
    // Napi::Value NAPI_METHOD(GetCopyData); // TODO verify if return type is correct
    // NAPI_METHOD(Cancel);



    static void on_io_readable(uv_poll_t* handle, int status, int revents);
    static void on_io_writable(uv_poll_t* handle, int status, int revents);
    void ReadStart();
    void ReadStop();
    void WriteStart();
    void WriteStop();
    void ClearLastResult();
    void SetLastResult(PGresult* result);
    static char* NewCString(Napi::Env env, Napi::Value val);
    static char** NewCStringArray(Napi::Env env, Napi::Array jsParams);
    static void DeleteCStringArray(char** array, int length);
    void Emit(const char* message);
};

#endif
