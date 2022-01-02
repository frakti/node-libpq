#include "addon.h"

// Initialize the node addon
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return Connection::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
