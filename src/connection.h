#ifndef NODE_LIBPQ_CONNECTION_V2
#define NODE_LIBPQ_CONNECTION_V2

#include <napi.h>
#include <libpq-fe.h>

#define NAPI_METHOD(name)                                                       \
    name(const Napi::CallbackInfo& info)

class Connection : public Napi::ObjectWrap<Connection> {
  public:
    // static Napi::Value NAPI_METHOD(Create);
    static Napi::Value NAPI_METHOD(ConnectSync); // Boolean
    static void NAPI_METHOD(Connect);
    static Napi::Value NAPI_METHOD(ServerVersion);
    static Napi::Value NAPI_METHOD(Socket);
    static Napi::Value NAPI_METHOD(GetLastErrorMessage);
    static void NAPI_METHOD(Finish);
    static void NAPI_METHOD(MarkAsFinished);
    static void NAPI_METHOD(Exec);
    static void NAPI_METHOD(ExecParams);
    static void NAPI_METHOD(Prepare);
    static void NAPI_METHOD(ExecPrepared);
    static void NAPI_METHOD(Clear);
    static Napi::Value NAPI_METHOD(Ntuples);
    static Napi::Value NAPI_METHOD(Nfields);
    static Napi::Value NAPI_METHOD(Fname); // string or null
    static Napi::Value NAPI_METHOD(Ftype);
    static Napi::Value NAPI_METHOD(Getvalue); // string or null
    static Napi::Value NAPI_METHOD(Getisnull); // Boolean
    static Napi::Value NAPI_METHOD(CmdStatus);
    static Napi::Value NAPI_METHOD(CmdTuples);
    static Napi::Value NAPI_METHOD(ResultStatus);
    static Napi::Value NAPI_METHOD(ResultErrorMessage);
    static Napi::Value NAPI_METHOD(ResultErrorFields); // object or null
    static Napi::Value NAPI_METHOD(SendQuery); // Boolean
    static Napi::Value NAPI_METHOD(SendQueryParams); // Boolean
    static Napi::Value NAPI_METHOD(SendPrepare); // Boolean
    static Napi::Value NAPI_METHOD(SendQueryPrepared); // Boolean
    static Napi::Value NAPI_METHOD(GetResult); // Boolean
    static Napi::Value NAPI_METHOD(ConsumeInput); // Boolean
    static Napi::Value NAPI_METHOD(IsBusy); // Boolean
    static void NAPI_METHOD(StartRead);
    static void NAPI_METHOD(StopRead);
    static void NAPI_METHOD(StartWrite);
    static Napi::Value NAPI_METHOD(SetNonBlocking); // Boolean
    static Napi::Value NAPI_METHOD(IsNonBlocking); // Boolean
    static Napi::Value NAPI_METHOD(Flush);
#ifdef ESCAPE_SUPPORTED
    static Napi::Value NAPI_METHOD(EscapeLiteral); // string or null
    static Napi::Value NAPI_METHOD(EscapeIdentifier); // string or null
#endif
    static Napi::Value NAPI_METHOD(Notifies); // object or null
    // static Napi::Value NAPI_METHOD(PutCopyData);
    static Napi::Value NAPI_METHOD(PutCopyEnd);
    // static Napi::Value NAPI_METHOD(GetCopyData); // TODO verify if return type is correct
    // static NAPI_METHOD(Cancel);

    bool ConnectDB(const char* paramString);
    char* ErrorMessage();
    PGconn* pq;

  private:
    PGresult* lastResult;
    uv_poll_t poll_watcher;
    bool is_reffed;
    bool is_reading;
    bool is_success_poll_init;
    int fd;

    Connection(const Napi::CallbackInfo& info);

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
