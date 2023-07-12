// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CApiJsiRuntime.h"
#include <map>
#include <utility>

using namespace facebook;

#pragma warning(push)
#pragma warning(disable : 4702) // `RethrowJsiError(); throw;` triggers 'unreachable code' warnings in Release builds

namespace Microsoft::NodeApiJsi {

// The macro to simplify recording JSI exceptions.
// It looks strange to keep the normal structure of the try/catch in code.
#define JSI_RUNTIME_SET_ERROR(runtime)                                \
const jsi::JSError &jsError) {                                        \
    CApiJsiRuntime::getFromJsiRuntime(runtime)->setJsiError(jsError); \
    return jsi_status_error;                                          \
  }                                                                   \
  catch (const std::exception &ex) {                                  \
    CApiJsiRuntime::getFromJsiRuntime(runtime)->setJsiError(ex);      \
    return jsi_status_error;                                          \
  } catch (...

#define THROW_ON_ERROR(expr)       \
  do {                             \
    jsi_status status = expr;      \
    if (status != jsi_status_ok) { \
      throwJsiError();             \
    }                              \
  } while (0)

static constexpr size_t MaxCallArgCount = 32;

// Forward declare the jsi::Runtime implementation on top of ABI-safe JsiRuntime.
struct CApiJsiRuntime;

// // An ABI-safe wrapper for jsi::Buffer.
// struct JsiByteBufferWrapper : implements<JsiByteBufferWrapper, IJsiByteBuffer> {
//   JsiByteBufferWrapper(JsiRuntime const &runtime, std::shared_ptr<jsi::Buffer const> const &buffer)
//   noexcept; ~JsiByteBufferWrapper() noexcept; uint32_t Size(); void GetData(JsiByteArrayUser const &useBytes);

//  private:
//   JsiRuntime m_runtime;
//   std::shared_ptr<jsi::Buffer const> m_buffer;
// };

// // A wrapper for ABI-safe JsiPreparedJavaScript.
// struct JsiPreparedJavaScriptWrapper : jsi::PreparedJavaScript {
//   JsiPreparedJavaScriptWrapper(JsiPreparedJavaScript const &preparedScript) noexcept;
//   ~JsiPreparedJavaScriptWrapper() noexcept;
//   JsiPreparedJavaScript const &Get() const noexcept;

//  private:
//   JsiPreparedJavaScript m_preparedScript;
// };

// An ABI-safe wrapper for jsi::HostObject.
struct JsiHostObjectWrapper : JsiHostObject {
  JsiHostObjectWrapper(std::shared_ptr<jsi::HostObject> hostObject) noexcept;

  const std::shared_ptr<jsi::HostObject> &hostObject() noexcept;

 private:
  static const JsiHostObjectVTable *getVTable() noexcept;
  static jsi_status JSICALL destroy(JsiHostObject *hostObject);
  static jsi_status JSICALL get(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameID *name, JsiValue *result);
  static jsi_status JSICALL set(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameID *name, JsiValue *value);
  static jsi_status JSICALL
  getPropertyNames(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameIDSpanCallback getNames, void *receiver);

 private:
  std::shared_ptr<jsi::HostObject> hostObject_;
};

// The function object that wraps up the jsi::HostFunctionType
// struct JsiHostFunctionWrapper : JsiHostFunction {
//   // We only support new and move constructors.
//   JsiHostFunctionWrapper(jsi::HostFunctionType hostFunction) noexcept;

//   JsiValue operator()(JsiRuntime &runtime, const JsiValue &thisArg, JsiValue* args, size_t argCount);

//   const jsi::HostFunctionType &hostFunction() noexcept;

// private:
//   // static const JsiHostObjectVTable *getVTable() noexcept;
//   // static jsi_status JSICALL destroy(JsiHostFunction *hostFunction);
//   // static jsi_status JSICALL get(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameID *name, JsiValue
//   *result);

//  private:
//   jsi::HostFunctionType hostFunction_;
//   JsiObject* functionData_{};
// };

// JSI runtime implementation as a wrapper for the ABI-safe JsiRuntime.
struct CApiJsiRuntime : jsi::Runtime {
  CApiJsiRuntime(JsiRuntime &runtime) noexcept;
  ~CApiJsiRuntime() noexcept;

  // Get CApiJsiRuntime from JsiRuntime in current thread.
  static CApiJsiRuntime *getFromJsiRuntime(const JsiRuntime &runtime) noexcept;

  jsi::Value evaluateJavaScript(const std::shared_ptr<const jsi::Buffer> &buffer, const std::string &sourceURL)
      override;
  std::shared_ptr<const jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const jsi::Buffer> &buffer,
      std::string sourceURL) override;
  jsi::Value evaluatePreparedJavaScript(const std::shared_ptr<const jsi::PreparedJavaScript> &js) override;
#if JSI_VERSION >= 4
  bool drainMicrotasks(int maxMicrotasksHint = -1) override;
#endif
  jsi::Object global() override;
  std::string description() override;
  bool isInspectable() override;
  jsi::Instrumentation &instrumentation() override;

 protected:
  PointerValue *cloneSymbol(const PointerValue *pv) override;
#if JSI_VERSION >= 6
  PointerValue *cloneBigInt(const PointerValue *pv) override;
#endif
  PointerValue *cloneString(const PointerValue *pv) override;
  PointerValue *cloneObject(const PointerValue *pv) override;
  PointerValue *clonePropNameID(const PointerValue *pv) override;

  jsi::PropNameID createPropNameIDFromAscii(const char *str, size_t length) override;
  jsi::PropNameID createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) override;
  jsi::PropNameID createPropNameIDFromString(const jsi::String &str) override;
#if JSI_VERSION >= 5
  jsi::PropNameID createPropNameIDFromSymbol(const jsi::Symbol &sym) override;
#endif
  std::string utf8(const jsi::PropNameID &propertyNameId) override;
  bool compare(const jsi::PropNameID &left, const jsi::PropNameID &right) override;

  std::string symbolToString(const jsi::Symbol &symbol) override;

#if JSI_VERSION >= 8
  jsi::BigInt createBigIntFromInt64(int64_t value) override;
  jsi::BigInt createBigIntFromUint64(uint64_t value) override;
  bool bigintIsInt64(const jsi::BigInt &bigint) override;
  bool bigintIsUint64(const jsi::BigInt &bigint) override;
  uint64_t truncate(const jsi::BigInt &bigint) override;
  jsi::String bigintToString(const jsi::BigInt &bigint, int radix) override;
#endif

  jsi::String createStringFromAscii(const char *str, size_t length) override;
  jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length) override;
  std::string utf8(const jsi::String &str) override;

#if JSI_VERSION >= 2
  jsi::Value createValueFromJsonUtf8(const uint8_t *json, size_t length) override;
#endif

  jsi::Object createObject() override;
  jsi::Object createObject(std::shared_ptr<jsi::HostObject> ho) override;
  std::shared_ptr<jsi::HostObject> getHostObject(const jsi::Object &obj) override;
  jsi::HostFunctionType &getHostFunction(const jsi::Function &func) override;

#if JSI_VERSION >= 7
  bool hasNativeState(const jsi::Object &) override;
  std::shared_ptr<jsi::NativeState> getNativeState(const jsi::Object &) override;
  void setNativeState(const jsi::Object &, std::shared_ptr<jsi::NativeState> state) override;
#endif

  jsi::Value getProperty(const jsi::Object &obj, const jsi::PropNameID &name) override;
  jsi::Value getProperty(const jsi::Object &obj, const jsi::String &name) override;
  bool hasProperty(const jsi::Object &obj, const jsi::PropNameID &name) override;
  bool hasProperty(const jsi::Object &obj, const jsi::String &name) override;
  void setPropertyValue(JSI_CONST_10 jsi::Object &obj, const jsi::PropNameID &name, const jsi::Value &value) override;
  void setPropertyValue(JSI_CONST_10 jsi::Object &obj, const jsi::String &name, const jsi::Value &value) override;

  bool isArray(const jsi::Object &obj) const override;
  bool isArrayBuffer(const jsi::Object &obj) const override;
  bool isFunction(const jsi::Object &obj) const override;
  bool isHostObject(const jsi::Object &obj) const override;
  bool isHostFunction(const jsi::Function &func) const override;
  jsi::Array getPropertyNames(const jsi::Object &obj) override;

  jsi::WeakObject createWeakObject(const jsi::Object &obj) override;
  jsi::Value lockWeakObject(JSI_NO_CONST_3 JSI_CONST_10 jsi::WeakObject &weakObj) override;

  jsi::Array createArray(size_t length) override;

#if JSI_VERSION >= 9
  jsi::ArrayBuffer createArrayBuffer(std::shared_ptr<jsi::MutableBuffer> buffer) override;
#endif
  size_t size(const jsi::Array &arr) override;
  size_t size(const jsi::ArrayBuffer &arrayBuffer) override;
  uint8_t *data(const jsi::ArrayBuffer &arrayBuffer) override;
  jsi::Value getValueAtIndex(const jsi::Array &arr, size_t i) override;
  void setValueAtIndexImpl(JSI_CONST_10 jsi::Array &arr, size_t i, const jsi::Value &value) override;

  jsi::Function createFunctionFromHostFunction(
      const jsi::PropNameID &name,
      unsigned int paramCount,
      jsi::HostFunctionType func) override;
  jsi::Value call(const jsi::Function &func, const jsi::Value &jsThis, const jsi::Value *args, size_t count) override;
  jsi::Value callAsConstructor(const jsi::Function &func, const jsi::Value *args, size_t count) override;

  ScopeState *pushScope() override;
  void popScope(ScopeState *scope) override;

  bool strictEquals(const jsi::Symbol &a, const jsi::Symbol &b) const override;
#if JSI_VERSION >= 6
  bool strictEquals(const jsi::BigInt &a, const jsi::BigInt &b) const override;
#endif
  bool strictEquals(const jsi::String &a, const jsi::String &b) const override;
  bool strictEquals(const jsi::Object &a, const jsi::Object &b) const override;

  bool instanceOf(const jsi::Object &o, const jsi::Function &f) override;

  void RethrowJsiError() const;
  void throwJsiError() const;

  void setJsiError(const jsi::JSError &jsError) noexcept;
  void setJsiError(const std::exception &nativeException) noexcept;

 private: // Convert JSI to ABI-safe JSI values
  static const JsiSymbol *asJsiSymbol(const PointerValue *pv) noexcept;
  static const JsiBigInt *asJsiBigInt(const PointerValue *pv) noexcept;
  static const JsiString *asJsiString(const PointerValue *pv) noexcept;
  static const JsiObject *asJsiObject(const PointerValue *pv) noexcept;
  static const JsiPropNameID *asJsiPropNameID(const PointerValue *pv) noexcept;
  static const JsiWeakObject *asJsiWeakObject(const PointerValue *pv) noexcept;

  static const JsiSymbol *asJsiSymbol(const jsi::Symbol &symbol) noexcept;
  static const JsiBigInt *asJsiBigInt(const jsi::BigInt &bigInt) noexcept;
  static const JsiString *asJsiString(const jsi::String &str) noexcept;
  static const JsiObject *asJsiObject(const jsi::Object &obj) noexcept;
  static const JsiPropNameID *asJsiPropNameID(const jsi::PropNameID &name) noexcept;
  static const JsiWeakObject *asJsiWeakObject(const jsi::WeakObject &weakObject) noexcept;
  static JsiValue asJsiValue(const jsi::Value &value) noexcept;

  //   static JsiPropertyIdRef DetachJsiPropertyIdRef(jsi::PropNameID &&name) noexcept;
  static JsiValue detachJsiValue(jsi::Value &&value) noexcept;

 private: // Convert ABI-safe JSI to JSI values
  static PointerValue *asPointerValue(JsiSymbol *symbol) noexcept;
  static PointerValue *asPointerValue(JsiBigInt *bigInt) noexcept;
  static PointerValue *asPointerValue(JsiString *str) noexcept;
  static PointerValue *asPointerValue(JsiObject *obj) noexcept;
  static PointerValue *asPointerValue(JsiPropNameID *name) noexcept;
  static PointerValue *asPointerValue(JsiWeakObject *obj) noexcept;

  static jsi::Symbol makeSymbol(JsiSymbol *symbol) noexcept;
  static jsi::BigInt makeBigInt(JsiBigInt *bigInt) noexcept;
  static jsi::String makeString(JsiString *str) noexcept;
  static jsi::Object makeObject(JsiObject *obj) noexcept;
  static jsi::PropNameID makePropNameID(JsiPropNameID *name) noexcept;
  jsi::WeakObject makeWeakObject(JsiWeakObject *weakObject) const noexcept;
  jsi::Array makeArray(JsiObject *arr) noexcept;
  jsi::ArrayBuffer makeArrayBuffer(JsiObject *arr) noexcept;
  jsi::Function makeFunction(JsiObject *func) noexcept;
  static jsi::Value makeValue(JsiValue &value) noexcept;

  // Allow access to the helper function
  friend struct JsiByteBufferWrapper;
  friend struct JsiHostObjectWrapper;
  friend struct JsiHostFunctionWrapper;
  friend struct AbiJSError;
  friend struct AbiJSINativeException;

  //  private: // PointerValue structures
  //   struct DataPointerValue : PointerValue {
  //     DataPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, uint64_t data) noexcept;
  //     DataPointerValue(uint64_t data) noexcept;
  //     void invalidate() override;

  //     uint64_t m_data;
  //     winrt::weak_ref<JsiRuntime> m_weakRuntime;
  //   };

  //   struct SymbolPointerValue : DataPointerValue {
  //     SymbolPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, JsiSymbolRef &&symbol) noexcept;
  //     void invalidate() override;
  //     static JsiSymbolRef const &GetData(PointerValue const *pv) noexcept;
  //     static JsiSymbolRef Detach(PointerValue const *pv) noexcept;
  //   };

  //   struct BigIntPointerValue : DataPointerValue {
  //     BigIntPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, JsiBigIntRef &&bigInt) noexcept;
  //     void invalidate() override;
  //     static JsiBigIntRef const &GetData(PointerValue const *pv) noexcept;
  //     static JsiBigIntRef Detach(PointerValue const *pv) noexcept;
  //   };

  //   struct StringPointerValue : DataPointerValue {
  //     StringPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, JsiStringRef &&str) noexcept;
  //     void invalidate() override;
  //     static JsiStringRef const &GetData(PointerValue const *pv) noexcept;
  //     static JsiStringRef Detach(PointerValue const *pv) noexcept;
  //   };

  //   struct ObjectPointerValue : DataPointerValue {
  //     ObjectPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, JsiObjectRef &&obj) noexcept;
  //     void invalidate() override;
  //     static JsiObjectRef const &GetData(PointerValue const *pv) noexcept;
  //     static JsiObjectRef Detach(PointerValue const *pv) noexcept;
  //   };

  //   struct PropNameIDPointerValue : DataPointerValue {
  //     PropNameIDPointerValue(winrt::weak_ref<JsiRuntime> &&weakRuntime, JsiPropertyIdRef &&name) noexcept;
  //     void invalidate() override;
  //     static JsiPropertyIdRef const &GetData(PointerValue const *pv) noexcept;
  //     static JsiPropertyIdRef Detach(PointerValue const *pv) noexcept;
  //   };

  // This type is to represent a reference to Value based on JsiValueData.
  // It avoids extra memory allocation by using an in-place storage.
  // It does not release the underlying pointer on invalidate() call
  // by proving null as runtime pointer.
  // struct ValueRef {
  //   ValueRef(JsiValue const &data) noexcept;
  //   ~ValueRef() noexcept;
  //   operator jsi::Value const &() const noexcept;

  //   using StoreType = std::aligned_storage_t<sizeof(DataPointerValue)>;
  //   static void InitValueRef(JsiValue const &data, jsi::Value *value, StoreType *store) noexcept;

  //  private:
  //   StoreType m_pointerStore{};
  //   jsi::Value m_value{};
  // };

  // struct ValueRefArray {
  //   ValueRefArray(array_view<JsiValue const> args) noexcept;
  //   jsi::Value const *Data() const noexcept;
  //   size_t Size() const noexcept;

  //  private:
  //   std::array<ValueRef::StoreType, MaxCallArgCount> m_pointerStoreArray{};
  //   std::array<jsi::Value, MaxCallArgCount> m_valueArray{};
  //   size_t m_size{};
  // };

  // struct PropNameIDRef {
  //   PropNameIDRef(JsiPropertyIdRef const &data) noexcept;
  //   ~PropNameIDRef() noexcept;
  //   operator jsi::PropNameID const &() const noexcept;

  //   using StoreType = std::aligned_storage_t<sizeof(DataPointerValue)>;

  //  private:
  //   StoreType m_pointerStore{};
  //   jsi::PropNameID m_propertyId;
  // };

 private:
  JsiRuntime &runtime_;

  bool m_pendingJSError{false};
};

//===========================================================================
// AbiJSError implementation
//===========================================================================

// TODO:
//  struct AbiJSError : jsi::JSError {
//    AbiJSError(CApiJsiRuntime &rt, JsiError &&jsiError) noexcept
//        : jsi::
//              JSError{to_string(jsiError.ErrorDetails()), rt, jsi::Value(rt,
//              CApiJsiRuntime::ValueRef(jsiError.Value()))},
//          m_jsiError{std::move(jsiError)} {}

//  private:
//   JsiError m_jsiError;
// };

//===========================================================================
// AbiJSINativeException implementation
//===========================================================================

// struct AbiJSINativeException : jsi::JSINativeException {
//   AbiJSINativeException(JsiError &&jsiError) noexcept
//       : jsi::JSINativeException{to_string(jsiError.ErrorDetails())}, m_jsiError{std::move(jsiError)} {}

//  private:
//   JsiError m_jsiError;
// };

//===========================================================================
// JsiBufferWrapper implementation
//===========================================================================

// JsiByteBufferWrapper::JsiByteBufferWrapper(
//     JsiRuntime const &runtime,
//     std::shared_ptr<Buffer const> const &buffer) noexcept
//     : m_runtime{runtime}, m_buffer{buffer} {}

// JsiByteBufferWrapper::~JsiByteBufferWrapper() = default;

// uint32_t JsiByteBufferWrapper::Size() try {
//   return static_cast<uint32_t>(m_buffer->size());
// } catch (JSI_RUNTIME_SET_ERROR(m_runtime)) {
//   throw;
// }

// void JsiByteBufferWrapper::GetData(JsiByteArrayUser const &useBytes) try {
//   useBytes(winrt::array_view<uint8_t const>{m_buffer->data(), m_buffer->data() + m_buffer->size()});
// } catch (JSI_RUNTIME_SET_ERROR(m_runtime)) {
//   throw;
// }

//===========================================================================
// JsiPreparedJavaScriptWrapper implementation
//===========================================================================

// JsiPreparedJavaScriptWrapper::JsiPreparedJavaScriptWrapper(JsiPreparedJavaScript const &preparedScript) noexcept
//     : m_preparedScript{preparedScript} {}

// JsiPreparedJavaScriptWrapper::~JsiPreparedJavaScriptWrapper() = default;

// JsiPreparedJavaScript const &JsiPreparedJavaScriptWrapper::Get() const noexcept {
//   return m_preparedScript;
// }

//===========================================================================
// JsiHostObjectWrapper implementation
//===========================================================================

JsiHostObjectWrapper::JsiHostObjectWrapper(std::shared_ptr<jsi::HostObject> hostObject) noexcept
    : JsiHostObject(getVTable()), hostObject_(std::move(hostObject)) {}

const std::shared_ptr<jsi::HostObject> &JsiHostObjectWrapper::hostObject() noexcept {
  return hostObject_;
}

/*static*/ const JsiHostObjectVTable *JsiHostObjectWrapper::getVTable() noexcept {
  static JsiHostObjectVTable vtable{destroy, get, set, getPropertyNames};
  return &vtable;
}

/*static*/ jsi_status JSICALL JsiHostObjectWrapper::destroy(JsiHostObject *hostObject) {
  delete static_cast<JsiHostObjectWrapper *>(hostObject);
  return jsi_status_ok;
}

/*static*/ jsi_status JSICALL
JsiHostObjectWrapper::get(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameID *name, JsiValue *result) try {
  jsi::HostObject *ho = static_cast<JsiHostObjectWrapper *>(hostObject)->hostObject_.get();
  CApiJsiRuntime *rt = CApiJsiRuntime::getFromJsiRuntime(*runtime);
  jsi::PropNameID propId = CApiJsiRuntime::makePropNameID(name);
  *result = CApiJsiRuntime::detachJsiValue(ho->get(*rt, propId));
  return jsi_status_ok;
} catch (JSI_RUNTIME_SET_ERROR(*runtime)) {
  return jsi_status_error;
}

/*static*/ jsi_status JSICALL
JsiHostObjectWrapper::set(JsiHostObject *hostObject, JsiRuntime *runtime, JsiPropNameID *name, JsiValue *value) try {
  jsi::HostObject *ho = static_cast<JsiHostObjectWrapper *>(hostObject)->hostObject_.get();
  CApiJsiRuntime *rt = CApiJsiRuntime::getFromJsiRuntime(*runtime);
  jsi::PropNameID propId = CApiJsiRuntime::makePropNameID(name);
  jsi::Value val = CApiJsiRuntime::makeValue(*value);
  ho->set(*rt, propId, val);
  return jsi_status_ok;
} catch (JSI_RUNTIME_SET_ERROR(*runtime)) {
  return jsi_status_error;
}

/*static*/ jsi_status JSICALL JsiHostObjectWrapper::getPropertyNames(
    JsiHostObject *hostObject,
    JsiRuntime *runtime,
    JsiPropNameIDSpanCallback getNames,
    void *receiver) try {
  jsi::HostObject *ho = static_cast<JsiHostObjectWrapper *>(hostObject)->hostObject_.get();
  CApiJsiRuntime *rt = CApiJsiRuntime::getFromJsiRuntime(*runtime);
  std::vector<jsi::PropNameID> propertyNames = ho->getPropertyNames(*rt);
  if (propertyNames.size() != 0) {
    getNames(nullptr, 0, receiver);
  } else {
    getNames(reinterpret_cast<const JsiPropNameID **>(&propertyNames[0]), propertyNames.size(), receiver);
  }
  return jsi_status_ok;
} catch (JSI_RUNTIME_SET_ERROR(*runtime)) {
  return jsi_status_error;
}

//===========================================================================
// JsiHostFunctionWrapper implementation
//===========================================================================

// JsiHostFunctionWrapper::JsiHostFunctionWrapper(HostFunctionType &&hostFunction) noexcept
//     : m_hostFunction{std::move(hostFunction)} {}

// JsiValue JsiHostFunctionWrapper::operator()(
//     JsiRuntime const &runtime,
//     JsiValue const &thisArg,
//     array_view<JsiValue const> args) try {
//   CApiJsiRuntime *rt{CApiJsiRuntime::GetFromJsiRuntime(runtime)};
//   CApiJsiRuntime::ValueRefArray valueRefArgs{args};
//   return CApiJsiRuntime::DetachJsiValueRef(
//       m_hostFunction(*rt, CApiJsiRuntime::ValueRef{thisArg}, valueRefArgs.Data(), valueRefArgs.Size()));
// } catch (JSI_RUNTIME_SET_ERROR(runtime)) {
//   throw;
// }

// /*static*/ HostFunctionType &JsiHostFunctionWrapper::GetHostFunction(JsiHostFunction const &hostFunction) noexcept {
//   void *hostFunctionAbi = get_abi(hostFunction);
//   JsiHostFunctionWrapper *self =
//       static_cast<impl::delegate<JsiHostFunction, JsiHostFunctionWrapper> *>(hostFunctionAbi);
//   return self->m_hostFunction;
// }

//===========================================================================
// CApiJsiRuntime implementation
//===========================================================================

// The tls_jsiAbiRuntimeMap map allows us to associate CApiJsiRuntime with JsiRuntime.
// The association is thread-specific and DLL-specific.
// It is thread specific because we want to have the safe access only in JS thread.
// It is DLL-specific because CApiJsiRuntime is not ABI-safe and each module DLL will
// have their own CApiJsiRuntime instance.
static thread_local std::map<const void *, CApiJsiRuntime *> *tls_jsiRuntimeMap{nullptr};

CApiJsiRuntime::CApiJsiRuntime(JsiRuntime &runtime) noexcept : runtime_{runtime} {
  if (!tls_jsiRuntimeMap) {
    tls_jsiRuntimeMap = new std::map<const void *, CApiJsiRuntime *>();
  }
  tls_jsiRuntimeMap->try_emplace(&runtime, this);
}

CApiJsiRuntime::~CApiJsiRuntime() {
  tls_jsiRuntimeMap->erase(&runtime_);
  if (tls_jsiRuntimeMap->empty()) {
    delete tls_jsiRuntimeMap;
    tls_jsiRuntimeMap = nullptr;
  }
}

/*static*/ CApiJsiRuntime *CApiJsiRuntime::getFromJsiRuntime(const JsiRuntime &runtime) noexcept {
  if (tls_jsiRuntimeMap) {
    auto it = tls_jsiRuntimeMap->find(&runtime);
    if (it != tls_jsiRuntimeMap->end()) {
      return it->second;
    }
  }
  return nullptr;
}

jsi::Value CApiJsiRuntime::evaluateJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    const std::string &sourceURL) {
  //     runtime_->evaluateJavaScript(runtime_, )
  // try {
  //   return makeValue(
  //       m_runtime.EvaluateJavaScript(winrt::make<JsiByteBufferWrapper>(m_runtime, buffer), to_hstring(sourceURL)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  // }
  return jsi::Value();
}

std::shared_ptr<const jsi::PreparedJavaScript> CApiJsiRuntime::prepareJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    std::string sourceURL) {
  //       try {
  //   return std::make_shared<JsiPreparedJavaScriptWrapper>(
  //       m_runtime.PrepareJavaScript(winrt::make<JsiByteBufferWrapper>(m_runtime, buffer), to_hstring(sourceURL)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return nullptr;
}

jsi::Value CApiJsiRuntime::evaluatePreparedJavaScript(const std::shared_ptr<const jsi::PreparedJavaScript> &js) {
  //   try {
  //   return makeValue(
  //       m_runtime.EvaluatePreparedJavaScript(std::static_pointer_cast<JsiPreparedJavaScriptWrapper
  //       const>(js)->Get()));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return jsi::Value();
}

bool CApiJsiRuntime::drainMicrotasks(int maxMicrotasksHint) {
  //   try {
  //   return m_runtime.DrainMicrotasks(maxMicrotasksHint);
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

jsi::Object CApiJsiRuntime::global() {
  JsiObject *obj;
  THROW_ON_ERROR(runtime_.getGlobal(&obj));
  return makeObject(obj);
}

std::string CApiJsiRuntime::description() {
  //   try {
  //   return to_string(m_runtime.Description());
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return {};
}

bool CApiJsiRuntime::isInspectable() {
  // try { return m_runtime.IsInspectable(); } catch (hresult_error const &) {
  // RethrowJsiError();
  // throw;
  return {};
}

jsi::Instrumentation &CApiJsiRuntime::instrumentation() {
  //   try {
  //   // TODO: implement
  //   VerifyElseCrash(false);
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  // VerifyElseCrash(false);
  throw std::exception();
}

jsi::Runtime::PointerValue *CApiJsiRuntime::cloneSymbol(const jsi::Runtime::PointerValue *pv) {
  JsiSymbol *result{};
  THROW_ON_ERROR(runtime_.cloneSymbol(asJsiSymbol(pv), &result));
  return asPointerValue(result);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::cloneBigInt(const jsi::Runtime::PointerValue *pv) {
  JsiBigInt *result{};
  THROW_ON_ERROR(runtime_.cloneBigInt(asJsiBigInt(pv), &result));
  return asPointerValue(result);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::cloneString(const jsi::Runtime::PointerValue *pv) {
  JsiString *result{};
  THROW_ON_ERROR(runtime_.cloneString(asJsiString(pv), &result));
  return asPointerValue(result);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::cloneObject(const jsi::Runtime::PointerValue *pv) {
  JsiObject *result{};
  THROW_ON_ERROR(runtime_.cloneObject(asJsiObject(pv), &result));
  return asPointerValue(result);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::clonePropNameID(const jsi::Runtime::PointerValue *pv) {
  JsiPropNameID *result{};
  THROW_ON_ERROR(runtime_.clonePropNameID(asJsiPropNameID(pv), &result));
  return asPointerValue(result);
}

jsi::PropNameID CApiJsiRuntime::createPropNameIDFromAscii(const char *str, size_t length) {
  JsiPropNameID *result{};
  THROW_ON_ERROR(runtime_.createPropNameIDFromAscii(str, length, &result));
  return makePropNameID(result);
}

jsi::PropNameID CApiJsiRuntime::createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) {
  JsiPropNameID *result{};
  THROW_ON_ERROR(runtime_.createPropNameIDFromUtf8(utf8, length, &result));
  return makePropNameID(result);
}

jsi::PropNameID CApiJsiRuntime::createPropNameIDFromString(const jsi::String &str) {
  JsiPropNameID *result{};
  THROW_ON_ERROR(runtime_.createPropNameIDFromString(asJsiString(str), &result));
  return makePropNameID(result);
}

#if JSI_VERSION >= 5
jsi::PropNameID CApiJsiRuntime::createPropNameIDFromSymbol(const jsi::Symbol &sym) {
  JsiPropNameID *result{};
  THROW_ON_ERROR(runtime_.createPropNameIDFromSymbol(asJsiSymbol(sym), &result));
  return makePropNameID(result);
}
#endif

std::string CApiJsiRuntime::utf8(const jsi::PropNameID &name) {
  std::string result;
  THROW_ON_ERROR(runtime_.propNameIDToUtf8(
      asJsiPropNameID(name),
      [](const uint8_t *utf8, size_t size, void *receiver) {
        reinterpret_cast<std::string *>(receiver)->assign(reinterpret_cast<char const *>(utf8), size);
      },
      &result));
  return result;
}

bool CApiJsiRuntime::compare(const jsi::PropNameID &left, const jsi::PropNameID &right) {
  bool result;
  THROW_ON_ERROR(runtime_.propNameIDEquals(asJsiPropNameID(left), asJsiPropNameID(right), &result));
  return result;
}

std::string CApiJsiRuntime::symbolToString(const jsi::Symbol &symbol) {
  std::string result;
  THROW_ON_ERROR(runtime_.symbolToUtf8(
      asJsiSymbol(symbol),
      [](const uint8_t *utf8, size_t size, void *receiver) {
        reinterpret_cast<std::string *>(receiver)->assign(reinterpret_cast<char const *>(utf8), size);
      },
      &result));
  return result;
}

#if JSI_VERSION >= 8
jsi::BigInt CApiJsiRuntime::createBigIntFromInt64(int64_t val) {
  JsiBigInt *result;
  THROW_ON_ERROR(runtime_.createBigIntFromInt64(val, &result));
  return makeBigInt(result);
}

jsi::BigInt CApiJsiRuntime::createBigIntFromUint64(uint64_t val) {
  JsiBigInt *result;
  THROW_ON_ERROR(runtime_.createBigIntFromUInt64(val, &result));
  return makeBigInt(result);
}

bool CApiJsiRuntime::bigintIsInt64(const jsi::BigInt &bigint) {
  bool result;
  THROW_ON_ERROR(runtime_.bigIntIsInt64(asJsiBigInt(bigint), &result));
  return result;
}

bool CApiJsiRuntime::bigintIsUint64(const jsi::BigInt &bigint) {
  bool result;
  THROW_ON_ERROR(runtime_.bigIntIsUInt64(asJsiBigInt(bigint), &result));
  return result;
}

uint64_t CApiJsiRuntime::truncate(const jsi::BigInt &bigint) {
  uint64_t result;
  THROW_ON_ERROR(runtime_.truncateBigInt(asJsiBigInt(bigint), &result));
  return result;
}

jsi::String CApiJsiRuntime::bigintToString(const jsi::BigInt &bigint, int radix) {
  JsiString *result;
  THROW_ON_ERROR(runtime_.bigIntToString(asJsiBigInt(bigint), radix, &result));
  return makeString(result);
}
#endif

jsi::String CApiJsiRuntime::createStringFromAscii(const char *str, size_t length) {
  JsiString *result;
  THROW_ON_ERROR(runtime_.createStringFromAscii(str, length, &result));
  return makeString(result);
}

jsi::String CApiJsiRuntime::createStringFromUtf8(const uint8_t *utf8, size_t length) {
  JsiString *result;
  THROW_ON_ERROR(runtime_.createStringFromUtf8(utf8, length, &result));
  return makeString(result);
}

std::string CApiJsiRuntime::utf8(const jsi::String &str) {
  std::string result;
  THROW_ON_ERROR(runtime_.stringToUtf8(
      asJsiString(str),
      [](const uint8_t *utf8, size_t size, void *receiver) {
        reinterpret_cast<std::string *>(receiver)->assign(reinterpret_cast<char const *>(utf8), size);
      },
      &result));
  return result;
}

jsi::Value CApiJsiRuntime::createValueFromJsonUtf8(const uint8_t *json, size_t length) {
  JsiValue result;
  THROW_ON_ERROR(runtime_.createValueFromJsonUtf8(json, length, &result));
  return makeValue(result);
}

jsi::Object CApiJsiRuntime::createObject() {
  JsiObject *result;
  THROW_ON_ERROR(runtime_.createObject(&result));
  return makeObject(result);
}

jsi::Object CApiJsiRuntime::createObject(std::shared_ptr<jsi::HostObject> ho) {
  auto hostObjectWrapper = std::make_unique<JsiHostObjectWrapper>(std::move(ho));
  JsiObject *result;
  THROW_ON_ERROR(runtime_.createObjectWithHostObject(hostObjectWrapper.release(), &result));
  return makeObject(result);
}

std::shared_ptr<jsi::HostObject> CApiJsiRuntime::getHostObject(const jsi::Object &obj) {
  JsiHostObject *hostObject;
  THROW_ON_ERROR(runtime_.getHostObject(asJsiObject(obj), &hostObject));
  return static_cast<JsiHostObjectWrapper *>(hostObject)->hostObject();
}

jsi::HostFunctionType &CApiJsiRuntime::getHostFunction(const jsi::Function &func) {
  // JsiHostFunction *hostFunction;
  // THROW_ON_ERROR(runtime_.getHostFunction(asJsiObject(func), &hostFunction));
  //  return static_cast<JsiHostFunctionWrapper*>(hostFunction)->hostFunction();
  static jsi::HostFunctionType x;
  return x;
}

#if JSI_VERSION >= 7
bool CApiJsiRuntime::hasNativeState(const jsi::Object &obj) {
  bool result;
  THROW_ON_ERROR(runtime_.hasNativeState(asJsiObject(obj), &result));
  return result;
}

std::shared_ptr<jsi::NativeState> CApiJsiRuntime::getNativeState(const jsi::Object &obj) {
  JsiNativeState result;
  THROW_ON_ERROR(runtime_.getNativeState(asJsiObject(obj), &result));
  return result ? *reinterpret_cast<std::shared_ptr<jsi::NativeState> *>(result) : nullptr;
}

void CApiJsiRuntime::setNativeState(const jsi::Object &obj, std::shared_ptr<jsi::NativeState> state) {
  auto *ptr = reinterpret_cast<JsiNativeState>(new std::shared_ptr<jsi::NativeState>(std::move(state)));
  THROW_ON_ERROR(runtime_.setNativeState(
      asJsiObject(obj), ptr, [](void *data) { delete reinterpret_cast<std::shared_ptr<jsi::NativeState> *>(data); }));
}
#endif

jsi::Value CApiJsiRuntime::getProperty(const jsi::Object &obj, const jsi::PropNameID &name) {
  JsiValue result;
  THROW_ON_ERROR(runtime_.getProperty(asJsiObject(obj), asJsiPropNameID(name), &result));
  return makeValue(result);
}

jsi::Value CApiJsiRuntime::getProperty(const jsi::Object &obj, const jsi::String &name) {
  JsiValue result;
  THROW_ON_ERROR(runtime_.getPropertyWithStringKey(asJsiObject(obj), asJsiString(name), &result));
  return makeValue(result);
}

bool CApiJsiRuntime::hasProperty(const jsi::Object &obj, const jsi::PropNameID &name) {
  bool result;
  THROW_ON_ERROR(runtime_.hasProperty(asJsiObject(obj), asJsiPropNameID(name), &result));
  return result;
}

bool CApiJsiRuntime::hasProperty(const jsi::Object &obj, const jsi::String &name) {
  bool result;
  THROW_ON_ERROR(runtime_.hasPropertyWithStringKey(asJsiObject(obj), asJsiString(name), &result));
  return result;
}

void CApiJsiRuntime::setPropertyValue(
    JSI_CONST_10 jsi::Object &obj,
    const jsi::PropNameID &name,
    const jsi::Value &value) {
  THROW_ON_ERROR(runtime_.setProperty(asJsiObject(obj), asJsiPropNameID(name), &asJsiValue(value)));
}

void CApiJsiRuntime::setPropertyValue(JSI_CONST_10 jsi::Object &obj, const jsi::String &name, const jsi::Value &value) {
  THROW_ON_ERROR(runtime_.setPropertyWithStringKey(asJsiObject(obj), asJsiString(name), &asJsiValue(value)));
}

bool CApiJsiRuntime::isArray(const jsi::Object &obj) const {
  //   return m_runtime.IsArray(asJsiObject(obj));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::isArrayBuffer(const jsi::Object &obj) const {
  //   return m_runtime.IsArrayBuffer(asJsiObject(obj));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::isFunction(const jsi::Object &obj) const {
  //   return m_runtime.IsFunction(asJsiObject(obj));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::isHostObject(const jsi::Object &obj) const {
  //   return m_runtime.IsHostObject(asJsiObject(obj));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::isHostFunction(const jsi::Function &func) const {
  //   return m_runtime.IsHostFunction(asJsiObject(func));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

jsi::Array CApiJsiRuntime::getPropertyNames(const jsi::Object &obj) {
  //   return makeArray(m_runtime.GetPropertyIdArray(asJsiObject(obj)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return makeArray(nullptr);
}

jsi::WeakObject CApiJsiRuntime::createWeakObject(const jsi::Object &obj) {
  //   return makeWeakObject(m_runtime.CreateWeakObject(asJsiObject(obj)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return makeWeakObject(nullptr);
}

jsi::Value CApiJsiRuntime::lockWeakObject(JSI_NO_CONST_3 JSI_CONST_10 jsi::WeakObject &weakObj) {
  //   return makeValue(m_runtime.LockWeakObject(AsJsiWeakObjectRef(weakObj)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return jsi::Value();
}

jsi::Array CApiJsiRuntime::createArray(size_t length) {
  //   return makeArray(m_runtime.CreateArray(static_cast<uint32_t>(length)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return makeArray(nullptr);
}

#if JSI_VERSION >= 9
jsi::ArrayBuffer CApiJsiRuntime::createArrayBuffer(std::shared_ptr<jsi::MutableBuffer> buffer) {
  //   // TODO: implement
  //   UNREFERENCED_PARAMETER(buffer);
  //   VerifyElseCrash(false);
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return makeArrayBuffer(nullptr);
}
#endif

size_t CApiJsiRuntime::size(const jsi::Array &arr) {
  //   return m_runtime.GetArraySize(asJsiObject(arr));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return 0;
}

size_t CApiJsiRuntime::size(const jsi::ArrayBuffer &arrayBuffer) {
  //   return m_runtime.GetArrayBufferSize(asJsiObject(arrayBuffer));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return 0;
}

uint8_t *CApiJsiRuntime::data(const jsi::ArrayBuffer &arrayBuffer) {
  //   uint8_t *dataResult{};
  //   m_runtime.GetArrayBufferData(asJsiObject(arrayBuffer), [&dataResult](array_view<uint8_t const> dataView) {
  //     dataResult = const_cast<uint8_t *>(dataView.data());
  //   });
  //   return dataResult;
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return nullptr;
}

jsi::Value CApiJsiRuntime::getValueAtIndex(const jsi::Array &arr, size_t i) {
  //   return makeValue(m_runtime.GetValueAtIndex(asJsiObject(arr), static_cast<uint32_t>(i)));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return jsi::Value();
}

void CApiJsiRuntime::setValueAtIndexImpl(JSI_CONST_10 jsi::Array &arr, size_t i, const jsi::Value &value) {
  //   m_runtime.SetValueAtIndex(asJsiObject(arr), static_cast<uint32_t>(i), AsJsiValueRef(value));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
}

jsi::Function CApiJsiRuntime::createFunctionFromHostFunction(
    const jsi::PropNameID &name,
    unsigned int paramCount,
    jsi::HostFunctionType func) {
  //   return makeFunction(m_runtime.CreateFunctionFromHostFunction(
  //       AsJsiPropertyIdRef(name), paramCount, JsiHostFunctionWrapper{std::move(func)}));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return makeFunction(nullptr);
}

jsi::Value
CApiJsiRuntime::call(const jsi::Function &func, const jsi::Value &jsThis, const jsi::Value *args, size_t count) {
  //   VerifyElseCrashSz(count <= MaxCallArgCount, "Argument count must not exceed the supported max arg count.");
  //   std::array<JsiValue, MaxCallArgCount> argsData;
  //   for (size_t i = 0; i < count; ++i) {
  //     argsData[i] = AsJsiValueRef(*(args + i));
  //   }

  //   return makeValue(
  //       m_runtime.Call(asJsiObject(func), AsJsiValueRef(jsThis), {argsData.data(), argsData.data() + count}));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return jsi::Value();
}

jsi::Value CApiJsiRuntime::callAsConstructor(const jsi::Function &func, const jsi::Value *args, size_t count) {
  //   VerifyElseCrashSz(count <= MaxCallArgCount, "Argument count must not exceed the supported max arg count.");
  //   std::array<JsiValue, MaxCallArgCount> argsData;
  //   for (size_t i = 0; i < count; ++i) {
  //     argsData[i] = AsJsiValueRef(*(args + i));
  //   }

  //   return makeValue(m_runtime.CallAsConstructor(asJsiObject(func), {argsData.data(), argsData.data() + count}));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return jsi::Value();
}

jsi::Runtime::ScopeState *CApiJsiRuntime::pushScope() {
  //   return reinterpret_cast<ScopeState *>(m_runtime.PushScope().Data);
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return nullptr;
}

void CApiJsiRuntime::popScope(jsi::Runtime::ScopeState *scope) {
  //   m_runtime.PopScope({reinterpret_cast<uint64_t>(scope)});
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
}

bool CApiJsiRuntime::strictEquals(const jsi::Symbol &a, const jsi::Symbol &b) const {
  //   return m_runtime.SymbolStrictEquals(AsJsiSymbolRef(a), AsJsiSymbolRef(b));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

#if JSI_VERSION >= 6
bool CApiJsiRuntime::strictEquals(const jsi::BigInt &a, const jsi::BigInt &b) const {
  //   return m_runtime.BigIntStrictEquals(AsJsiBigIntRef(a), AsJsiBigIntRef(b));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}
#endif

bool CApiJsiRuntime::strictEquals(const jsi::String &a, const jsi::String &b) const {
  //   return m_runtime.StringStrictEquals(AsJsiStringRef(a), AsJsiStringRef(b));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::strictEquals(const jsi::Object &a, const jsi::Object &b) const {
  //   return m_runtime.ObjectStrictEquals(asJsiObject(a), asJsiObject(b));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

bool CApiJsiRuntime::instanceOf(const jsi::Object &o, const jsi::Function &f) {
  //   return m_runtime.InstanceOf(asJsiObject(o), asJsiObject(f));
  // } catch (hresult_error const &) {
  //   RethrowJsiError();
  //   throw;
  return false;
}

template <typename T>
struct AutoRestore {
  AutoRestore(T *var, T value) : m_var{var}, m_value{std::exchange(*var, value)} {}

  ~AutoRestore() {
    *m_var = m_value;
  }

 private:
  T *m_var;
  T m_value;
};

void CApiJsiRuntime::RethrowJsiError() const {
  // auto jsiError = m_runtime.GetAndClearError();
  // if (!m_pendingJSError && jsiError.ErrorType() == JsiErrorType::JSError) {
  //   AutoRestore<bool> setValue{const_cast<bool *>(&m_pendingJSError), true};
  //   throw AbiJSError{*const_cast<CApiJsiRuntime *>(this), std::move(jsiError)};
  // } else {
  //   throw AbiJSINativeException{std::move(jsiError)};
  // }
}

[[noreturn]] void CApiJsiRuntime::throwJsiError() const {
  // TODO:
  //  auto jsiError = runtime_.getAndClearError();
  //  if (!m_pendingJSError && jsiError.ErrorType() == JsiErrorType::JSError) {
  //    AutoRestore<bool> setValue{const_cast<bool *>(&m_pendingJSError), true};
  //    throw AbiJSError{*const_cast<CApiJsiRuntime *>(this), std::move(jsiError)};
  //  } else {
  //    throw AbiJSINativeException{std::move(jsiError)};
  //  }
}

void CApiJsiRuntime::setJsiError(const jsi::JSError &jsError) noexcept {
  // runtime_.setError(JsiErrorType::JSError, jsError.what(), &asJsiValueRef(jsError.value()));
}

void CApiJsiRuntime::setJsiError(const std::exception &nativeException) noexcept {
  //  m_runtime.SetError(JsiErrorType::NativeException, nativeException.what(),
  //  &asJsiValueRef(jsi::Value::undefined()));
}

//===========================================================================
// CApiJsiRuntime utility functions implementation
//===========================================================================

/*static*/ const JsiSymbol *CApiJsiRuntime::asJsiSymbol(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiSymbol *>(pv);
}

/*static*/ const JsiBigInt *CApiJsiRuntime::asJsiBigInt(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiBigInt *>(pv);
}

/*static*/ const JsiString *CApiJsiRuntime::asJsiString(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiString *>(pv);
}

/*static*/ const JsiObject *CApiJsiRuntime::asJsiObject(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiObject *>(pv);
}

/*static*/ const JsiPropNameID *CApiJsiRuntime::asJsiPropNameID(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiPropNameID *>(pv);
}

/*static*/ const JsiWeakObject *CApiJsiRuntime::asJsiWeakObject(const jsi::Runtime::PointerValue *pv) noexcept {
  return reinterpret_cast<const JsiWeakObject *>(pv);
}

/*static*/ const JsiSymbol *CApiJsiRuntime::asJsiSymbol(const jsi::Symbol &symbol) noexcept {
  return asJsiSymbol(getPointerValue(symbol));
}

/*static*/ const JsiBigInt *CApiJsiRuntime::asJsiBigInt(const jsi::BigInt &bigInt) noexcept {
  return asJsiBigInt(getPointerValue(bigInt));
}

/*static*/ const JsiString *CApiJsiRuntime::asJsiString(const jsi::String &str) noexcept {
  return asJsiString(getPointerValue(str));
}

/*static*/ const JsiObject *CApiJsiRuntime::asJsiObject(const jsi::Object &obj) noexcept {
  return asJsiObject(getPointerValue(obj));
}

/*static*/ const JsiPropNameID *CApiJsiRuntime::asJsiPropNameID(const jsi::PropNameID &name) noexcept {
  return asJsiPropNameID(getPointerValue(name));
}

/*static*/ const JsiWeakObject *CApiJsiRuntime::asJsiWeakObject(const jsi::WeakObject &weakObject) noexcept {
  return asJsiWeakObject(getPointerValue(weakObject));
}

/*static*/ JsiValue CApiJsiRuntime::asJsiValue(const jsi::Value &value) noexcept {
  // We assume that the JsiValue and jsi::Value have the same layout.
  JsiValue result = reinterpret_cast<const JsiValue &>(value);
#if JSI_VERSION < 6
  // Compensate for the enum value shift related to BigInt
  if (result.kind >= JsiValueKind.BigIntKind) {
    result.kind = result.kind + 1;
  }
#endif
  return result;
}

// /*static*/ JsiPropertyIdRef CApiJsiRuntime::DetachJsiPropertyIdRef(PropNameID &&name) noexcept {
//   // This method detaches JsiPropertyIdRef from the PropNameID.
//   // It lets the PropNameIDPointerValue destructor run, but it must not destroy the underlying JS engine object.
//   return PropNameIDPointerValue::Detach(getPointerValue(name));
// }

/*static*/ JsiValue CApiJsiRuntime::detachJsiValue(jsi::Value &&value) noexcept {
  // Here we should move data from Value to JsiValue.
  // For Pointer types it means that we should allow running the PointerValue
  // destructor, but it must not call the Release method to keep the underlying
  // data alive. Thus, we must detach the value.
  // We assume that the JsiValue and jsi::Value have the same layout.
  JsiValue result;
  ::new (std::addressof(result)) jsi::Value{std::move(value)};
#if JSI_VERSION < 6
  // Compensate for the enum value shift related to BigInt
  if (result.kind >= JsiValueKind.BigIntKind) {
    result.kind = result.kind + 1;
  }
#endif
  return result;
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiSymbol *symbol) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(symbol);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiBigInt *bigInt) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(bigInt);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiString *str) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(str);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiObject *obj) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(obj);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiPropNameID *name) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(name);
}

jsi::Runtime::PointerValue *CApiJsiRuntime::asPointerValue(JsiWeakObject *obj) noexcept {
  return reinterpret_cast<jsi::Runtime::PointerValue *>(obj);
}

jsi::Symbol CApiJsiRuntime::makeSymbol(JsiSymbol *symbol) noexcept {
  return make<jsi::Symbol>(asPointerValue(symbol));
}

jsi::BigInt CApiJsiRuntime::makeBigInt(JsiBigInt *bigInt) noexcept {
  return make<jsi::BigInt>(asPointerValue(bigInt));
}

jsi::String CApiJsiRuntime::makeString(JsiString *str) noexcept {
  return make<jsi::String>(asPointerValue(str));
}

jsi::Object CApiJsiRuntime::makeObject(JsiObject *obj) noexcept {
  return make<jsi::Object>(asPointerValue(obj));
}

jsi::PropNameID CApiJsiRuntime::makePropNameID(JsiPropNameID *name) noexcept {
  return make<jsi::PropNameID>(asPointerValue(name));
}

jsi::WeakObject CApiJsiRuntime::makeWeakObject(JsiWeakObject *weakObject) const noexcept {
  return make<jsi::WeakObject>(asPointerValue(weakObject));
}

jsi::Array CApiJsiRuntime::makeArray(JsiObject *arr) noexcept {
  return makeObject(arr).getArray(*this);
}

jsi::ArrayBuffer CApiJsiRuntime::makeArrayBuffer(JsiObject *arr) noexcept {
  return makeObject(arr).getArrayBuffer(*this);
}

jsi::Function CApiJsiRuntime::makeFunction(JsiObject *func) noexcept {
  return makeObject(func).getFunction(*this);
}

/*static*/ jsi::Value CApiJsiRuntime::makeValue(JsiValue &value) noexcept {
  switch (value.kind) {
    case JsiValueKind::Undefined:
      return jsi::Value();
    case JsiValueKind::Null:
      return jsi::Value(nullptr);
    case JsiValueKind::Boolean:
      return jsi::Value(static_cast<bool>(value.data));
    case JsiValueKind::Number:
      return jsi::Value(*reinterpret_cast<double *>(&value.data));
    case JsiValueKind::Symbol:
      return jsi::Value(makeSymbol(reinterpret_cast<JsiSymbol *>(value.data)));
    case JsiValueKind::BigInt:
      return jsi::Value(makeBigInt(reinterpret_cast<JsiBigInt *>(value.data)));
    case JsiValueKind::String:
      return jsi::Value(makeString(reinterpret_cast<JsiString *>(value.data)));
    case JsiValueKind::Object:
      return jsi::Value(makeObject(reinterpret_cast<JsiObject *>(value.data)));
    default:
      // TODO:
      //   VerifyElseCrashSz(false, "Unexpected JsiValueKind value");
      return jsi::Value();
  }
}

//===========================================================================
// CApiJsiRuntime::ValueRef implementation
//===========================================================================

// CApiJsiRuntime::ValueRef::ValueRef(JsiValue const &data) noexcept {
//   InitValueRef(data, &m_value, std::addressof(m_pointerStore));
// }

// CApiJsiRuntime::ValueRef::~ValueRef() noexcept {}

// CApiJsiRuntime::ValueRef::operator Value const &() const noexcept {
//   return m_value;
// }

// /*static*/ void
// CApiJsiRuntime::ValueRef::InitValueRef(JsiValue const &data, Value *value, StoreType *store) noexcept {
//   // We assume that the JsiValue and Value have the same layout.
//   auto valueAsDataPtr = reinterpret_cast<JsiValue *>(value);
//   valueAsDataPtr->Kind = data.Kind;
//   // TODO: JSIVALUECONVERSION - Need to fix JSI kind mapping
//   switch (valueAsDataPtr->Kind) {
//     case JsiValueKind::Symbol:
//     case JsiValueKind::String:
//     case JsiValueKind::Object:
//       // Do in-place initialization in 'store'
//       valueAsDataPtr->Data = reinterpret_cast<uint64_t>(new (store) DataPointerValue(data.Data));
//       break;
//     default:
//       valueAsDataPtr->Data = data.Data;
//       break;
//   }
// }

//===========================================================================
// CApiJsiRuntime::ValueRefArray implementation
//===========================================================================

// CApiJsiRuntime::ValueRefArray::ValueRefArray(array_view<JsiValue const> args) noexcept : m_size{args.size()} {
//   VerifyElseCrashSz(m_size <= MaxCallArgCount, "Argument count must not exceed the MaxCallArgCount");
//   for (uint32_t i = 0; i < args.size(); ++i) {
//     ValueRef::InitValueRef(args[i], &m_valueArray[i], std::addressof(m_pointerStoreArray[i]));
//   }
// }

// jsi::Value const *CApiJsiRuntime::ValueRefArray::Data() const noexcept {
//   return m_valueArray.data();
// }

// size_t CApiJsiRuntime::ValueRefArray::Size() const noexcept {
//   return m_size;
// }

//===========================================================================
// CApiJsiRuntime::PropNameIDRef implementation
//===========================================================================

// CApiJsiRuntime::PropNameIDRef::PropNameIDRef(JsiPropertyIdRef const &data) noexcept
//     : m_propertyId{make<PropNameID>(new(std::addressof(m_pointerStore)) DataPointerValue(data.Data))} {}

// CApiJsiRuntime::PropNameIDRef::~PropNameIDRef() noexcept {}

// CApiJsiRuntime::PropNameIDRef::operator jsi::PropNameID const &() const noexcept {
//   return m_propertyId;
// }

} // namespace Microsoft::NodeApiJsi

#pragma warning(pop)
