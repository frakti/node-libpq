#include "addon.h"

// Initialize the node addon
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "PQ", {
    //connection initialization & management functions
    Connection::InstanceMethod("$connectSync", &Connection::ConnectSync),
    Connection::InstanceMethod("$connect", &Connection::Connect),
    Connection::InstanceMethod("$finish", &Connection::Finish),
    Connection::InstanceMethod("$markAsFinished", &Connection::MarkAsFinished),

    Connection::InstanceMethod("$getLastErrorMessage", &Connection::GetLastErrorMessage),
    Connection::InstanceMethod("$resultErrorFields", &Connection::ResultErrorFields),
    Connection::InstanceMethod("$socket", &Connection::Socket),
    Connection::InstanceMethod("$serverVersion", &Connection::ServerVersion),

    //sync query functions
    Connection::InstanceMethod("$exec", &Connection::Exec),
    Connection::InstanceMethod("$execParams", &Connection::ExecParams),
    Connection::InstanceMethod("$prepare", &Connection::Prepare),
    Connection::InstanceMethod("$execPrepared", &Connection::ExecPrepared),

    //async query functions
    Connection::InstanceMethod("$sendQuery", &Connection::SendQuery),
    Connection::InstanceMethod("$sendQueryParams", &Connection::SendQueryParams),
    Connection::InstanceMethod("$sendPrepare", &Connection::SendPrepare),
    Connection::InstanceMethod("$sendQueryPrepared", &Connection::SendQueryPrepared),
    Connection::InstanceMethod("$getResult", &Connection::GetResult),

    //async i/o control functions
    Connection::InstanceMethod("$startRead", &Connection::StartRead),
    Connection::InstanceMethod("$stopRead", &Connection::StopRead),
    Connection::InstanceMethod("$startWrite", &Connection::StartWrite),
    Connection::InstanceMethod("$consumeInput", &Connection::ConsumeInput),
    Connection::InstanceMethod("$isBusy", &Connection::IsBusy),
    Connection::InstanceMethod("$setNonBlocking", &Connection::SetNonBlocking),
    Connection::InstanceMethod("$isNonBlocking", &Connection::IsNonBlocking),
    Connection::InstanceMethod("$flush", &Connection::Flush),

    //result accessor functions
    Connection::InstanceMethod("$clear", &Connection::Clear),
    Connection::InstanceMethod("$ntuples", &Connection::Ntuples),
    Connection::InstanceMethod("$nfields", &Connection::Nfields),
    Connection::InstanceMethod("$fname", &Connection::Fname),
    Connection::InstanceMethod("$ftype", &Connection::Ftype),
    Connection::InstanceMethod("$getvalue", &Connection::Getvalue),
    Connection::InstanceMethod("$getisnull", &Connection::Getisnull),
    Connection::InstanceMethod("$cmdStatus", &Connection::CmdStatus),
    Connection::InstanceMethod("$cmdTuples", &Connection::CmdTuples),
    Connection::InstanceMethod("$resultStatus", &Connection::ResultStatus),
    Connection::InstanceMethod("$resultErrorMessage", &Connection::ResultErrorMessage),

    //string escaping functions
  #ifdef ESCAPE_SUPPORTED
    Connection::InstanceMethod("$escapeLiteral", &Connection::EscapeLiteral),
    Connection::InstanceMethod("$escapeIdentifier", &Connection::EscapeIdentifier),
  #endif

    //async notifications
    Connection::InstanceMethod("$notifies", &Connection::Notifies),

    //COPY IN/OUT
    Connection::InstanceMethod("$putCopyData", &Connection::PutCopyData),
    Connection::InstanceMethod("$putCopyEnd", &Connection::PutCopyEnd),
    Connection::InstanceMethod("$getCopyData", &Connection::GetCopyData),

    //Cancel
    Connection::InstanceMethod("$cancel", &Connection::Cancel)
  });

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("PQ", func);
  return exports;
}

NODE_API_MODULE(addon, InitAll)
