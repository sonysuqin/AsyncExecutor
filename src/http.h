#ifndef BEE_HTTP_H
#define BEE_HTTP_H

#include <functional>
#include <future>
#include <stdint.h>
#include <string>
#include <vector>

#include "cronet_c.h"

namespace bee {

class HttpEngine {
 public:
  HttpEngine();
  virtual ~HttpEngine();
  Cronet_EnginePtr GetEngine();

 private:
  Cronet_EnginePtr engine_ = nullptr;
};

class HttpExecutor {
 public:
  HttpExecutor();
  ~HttpExecutor();

  Cronet_ExecutorPtr GetExecutor();

  // virtual functions.
  virtual void ExecuteRunnable(Cronet_RunnablePtr runnable) = 0;

  virtual void ExecuteFunction(std::function<void()> function) = 0;

  virtual bool IsExecutorThread() = 0;

  // Implementation of Cronet_Executor methods.
  static void ExecuteInternal(Cronet_ExecutorPtr executor,
                              Cronet_RunnablePtr runnable);

  static HttpExecutor* GetThis(Cronet_ExecutorPtr executor);

 private:
  Cronet_ExecutorPtr const executor_;
};

class HttpCallback {
 public:
  virtual void OnRedirectReceived(Cronet_UrlRequestPtr request,
                                  Cronet_UrlResponseInfoPtr info,
                                  Cronet_String newLocationUrl) = 0;

  virtual void OnResponseStarted(Cronet_UrlRequestPtr request,
                                 Cronet_UrlResponseInfoPtr info) = 0;

  virtual void OnReadCompleted(Cronet_UrlRequestPtr request,
                               Cronet_UrlResponseInfoPtr info,
                               Cronet_BufferPtr buffer,
                               uint64_t bytes_read) = 0;

  virtual void OnSucceeded(Cronet_UrlRequestPtr request,
                           Cronet_UrlResponseInfoPtr info) = 0;

  virtual void OnFailed(Cronet_UrlRequestPtr request,
                        Cronet_UrlResponseInfoPtr info,
                        Cronet_ErrorPtr error) = 0;

  virtual void OnCanceled(Cronet_UrlRequestPtr request,
                          Cronet_UrlResponseInfoPtr info) = 0;
};

class HttpProvider {
 public:
  virtual int64_t GetLength() = 0;

  virtual void Read(Cronet_UploadDataSinkPtr upload_data_sink,
                    Cronet_BufferPtr buffer) = 0;

  virtual void Rewind(Cronet_UploadDataSinkPtr upload_data_sink) = 0;

  virtual void Close() = 0;
};

struct HttpHeader {
  explicit HttpHeader(const std::string& name, const std::string& value)
      : name(name), value(value) {}
  explicit HttpHeader(const HttpHeader& from) = default;
  explicit HttpHeader(HttpHeader&& from) = default;
  ~HttpHeader() = default;

  std::string name;
  std::string value;
};

class Http {
 public:
  Http(std::shared_ptr<HttpEngine> engine,
       std::shared_ptr<HttpExecutor> executor);
  virtual ~Http();

  // Public interfaces.
  int32_t Get(const std::string& url, std::shared_ptr<HttpCallback> callback);

  int32_t Get(const std::string& url,
              const std::vector<HttpHeader>& headers,
              std::shared_ptr<HttpCallback> callback);

  int32_t Post(const std::string& url,
               std::shared_ptr<HttpProvider> provider,
               std::shared_ptr<HttpCallback> callback);

  int32_t Post(const std::string& url,
               const std::vector<HttpHeader>& headers,
               std::shared_ptr<HttpProvider> provider,
               std::shared_ptr<HttpCallback> callback);

  int32_t FollowRedirect();

  int32_t Read(Cronet_BufferPtr buffer);

  void Cancel();

  // Sync close Http object, must called from different thread
  // from io thread or executor thread.
  void Close();

  // Implementation of Cronet_UrlRequestCallback methods.
  static void OnRedirectReceivedInternal(Cronet_UrlRequestCallbackPtr callback,
                                         Cronet_UrlRequestPtr request,
                                         Cronet_UrlResponseInfoPtr info,
                                         Cronet_String newLocationUrl);

  static void OnResponseStartedInternal(Cronet_UrlRequestCallbackPtr callback,
                                        Cronet_UrlRequestPtr request,
                                        Cronet_UrlResponseInfoPtr info);

  static void OnReadCompletedInternal(Cronet_UrlRequestCallbackPtr callback,
                                      Cronet_UrlRequestPtr request,
                                      Cronet_UrlResponseInfoPtr info,
                                      Cronet_BufferPtr buffer,
                                      uint64_t bytesRead);

  static void OnSucceededInternal(Cronet_UrlRequestCallbackPtr callback,
                                  Cronet_UrlRequestPtr request,
                                  Cronet_UrlResponseInfoPtr info);

  static void OnFailedInternal(Cronet_UrlRequestCallbackPtr callback,
                               Cronet_UrlRequestPtr request,
                               Cronet_UrlResponseInfoPtr info,
                               Cronet_ErrorPtr error);

  static void OnCanceledInternal(Cronet_UrlRequestCallbackPtr callback,
                                 Cronet_UrlRequestPtr request,
                                 Cronet_UrlResponseInfoPtr info);

  // Implementation of Cronet_UploadDataProvider methods.
  static int64_t UploadProviderGetLengthInternal(
      Cronet_UploadDataProviderPtr provider);

  static void UploadProviderReadInternal(
      Cronet_UploadDataProviderPtr provider,
      Cronet_UploadDataSinkPtr upload_data_sink,
      Cronet_BufferPtr buffer);

  static void UploadProviderRewindInternal(
      Cronet_UploadDataProviderPtr provider,
      Cronet_UploadDataSinkPtr upload_data_sink);

  static void UploadProviderCloseInternal(
      Cronet_UploadDataProviderPtr provider);

 private:
  // Get this from Cronet_UrlRequestCallbackPtr.
  static Http* GetThis(Cronet_UrlRequestCallbackPtr callback);

  // Get this from Cronet_UploadDataProviderPtr.
  static Http* GetThis(Cronet_UploadDataProviderPtr provider);

  // Http callbacks.
  void OnRedirectReceived(Cronet_UrlRequestPtr request,
                          Cronet_UrlResponseInfoPtr info,
                          Cronet_String newLocationUrl);

  void OnResponseStarted(Cronet_UrlRequestPtr request,
                         Cronet_UrlResponseInfoPtr info);

  void OnReadCompleted(Cronet_UrlRequestPtr request,
                       Cronet_UrlResponseInfoPtr info,
                       Cronet_BufferPtr buffer,
                       uint64_t bytes_read);

  void OnSucceeded(Cronet_UrlRequestPtr request,
                   Cronet_UrlResponseInfoPtr info);

  void OnFailed(Cronet_UrlRequestPtr request,
                Cronet_UrlResponseInfoPtr info,
                Cronet_ErrorPtr error);

  void OnCanceled(Cronet_UrlRequestPtr request, Cronet_UrlResponseInfoPtr info);

  // Http upload provider callbacks.
  int64_t UploadProviderGetLength();

  void UploadProviderRead(Cronet_UploadDataSinkPtr upload_data_sink,
                          Cronet_BufferPtr buffer);

  void UploadProviderRewind(Cronet_UploadDataSinkPtr upload_data_sink);

  void UploadProviderClose();

 private:
  std::weak_ptr<HttpEngine> engine_;
  std::weak_ptr<HttpExecutor> executor_;
  std::weak_ptr<HttpCallback> http_callback_;
  std::weak_ptr<HttpProvider> http_provider_;
  Cronet_UrlRequestCallbackPtr cronet_callback_ = nullptr;
  Cronet_UploadDataProviderPtr upload_provider_ = nullptr;
  Cronet_UrlRequestPtr request_ = nullptr;
  std::atomic<bool> finished_;
  std::atomic<bool> cancelling_;
  std::shared_ptr<std::promise<int32_t>> close_promise_;
};

}  // namespace bee

#endif  // BEE_HTTP_H
