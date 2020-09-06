#include "http.h"

#include "bee_define.h"

namespace bee {

const char kUserAgent[] = "Roblin";

HttpEngine::HttpEngine() {
  engine_ = Cronet_Engine_Create();
  Cronet_EngineParamsPtr params = Cronet_EngineParams_Create();
  Cronet_EngineParams_user_agent_set(params, kUserAgent);
  Cronet_EngineParams_enable_http2_set(params, true);
  Cronet_EngineParams_enable_quic_set(params, true);
  Cronet_Engine_StartWithParams(engine_, params);
  Cronet_EngineParams_Destroy(params);
}

HttpEngine::~HttpEngine() {
  if (engine_ != nullptr) {
    Cronet_Engine_Shutdown(engine_);
    Cronet_Engine_Destroy(engine_);
    engine_ = nullptr;
  }
}

Cronet_EnginePtr HttpEngine::GetEngine() {
  return engine_;
}

HttpExecutor::HttpExecutor()
    : executor_(Cronet_Executor_CreateWith(HttpExecutor::ExecuteInternal)) {
  Cronet_Executor_SetClientContext(executor_, this);
}

HttpExecutor::~HttpExecutor() {
  Cronet_Executor_Destroy(executor_);
}

Cronet_ExecutorPtr HttpExecutor::GetExecutor() {
  return executor_;
}

void HttpExecutor::ExecuteInternal(Cronet_ExecutorPtr executor,
                                   Cronet_RunnablePtr runnable) {
  GetThis(executor)->ExecuteRunnable(runnable);
}

HttpExecutor* HttpExecutor::GetThis(Cronet_ExecutorPtr executor) {
  return static_cast<HttpExecutor*>(Cronet_Executor_GetClientContext(executor));
}

Http::Http(std::shared_ptr<HttpEngine> engine,
           std::shared_ptr<HttpExecutor> executor)
    : engine_(engine),
      executor_(executor),
      finished_(false),
      cancelling_(false) {}

Http::~Http() {
  Close();
}

int32_t Http::Get(const std::string& url,
                  std::shared_ptr<HttpCallback> callback) {
  return Get(url, std::vector<HttpHeader>(), callback);
}

int32_t Http::Get(const std::string& url,
                  const std::vector<HttpHeader>& headers,
                  std::shared_ptr<HttpCallback> callback) {
  int32_t ret = kBeeErrorCode_Success;
  Cronet_UrlRequestParamsPtr params = nullptr;
  do {
    if (request_ != nullptr) {
      ret = kBeeErrorCode_Invalid_State;
      break;
    }

    std::shared_ptr<HttpEngine> engine = engine_.lock();
    std::shared_ptr<HttpExecutor> executor = executor_.lock();
    if (url.empty() || callback == nullptr || engine == nullptr ||
        executor == nullptr) {
      ret = kBeeErrorCode_Invalid_Param;
      break;
    }

    printf("Http request %s\n", url.c_str());

    http_callback_ = callback;
    request_ = Cronet_UrlRequest_Create();
    params = Cronet_UrlRequestParams_Create();
    Cronet_UrlRequestParams_http_method_set(params, "GET");

    for (auto& header : headers) {
      Cronet_HttpHeaderPtr cronet_header = Cronet_HttpHeader_Create();
      Cronet_HttpHeader_name_set(cronet_header, header.name.c_str());
      Cronet_HttpHeader_value_set(cronet_header, header.value.c_str());
      Cronet_UrlRequestParams_request_headers_add(params, cronet_header);
      Cronet_HttpHeader_Destroy(cronet_header);
    }

    cronet_callback_ = Cronet_UrlRequestCallback_CreateWith(
        Http::OnRedirectReceivedInternal, Http::OnResponseStartedInternal,
        Http::OnReadCompletedInternal, Http::OnSucceededInternal,
        Http::OnFailedInternal, Http::OnCanceledInternal);
    Cronet_UrlRequestCallback_SetClientContext(cronet_callback_, this);

    Cronet_RESULT cronet_res = Cronet_UrlRequest_InitWithParams(
        request_, engine->GetEngine(), url.c_str(), params, cronet_callback_,
        executor->GetExecutor());
    if (cronet_res != Cronet_RESULT_SUCCESS) {
      // LOG
      ret = kBeeErrorCode_Cronet_Request_Init_Failed;
      break;
    }

    cronet_res = Cronet_UrlRequest_Start(request_);
    if (cronet_res != Cronet_RESULT_SUCCESS) {
      // LOG
      ret = kBeeErrorCode_Cronet_Request_Start_Failed;
      break;
    }
  } while (0);
  if (params != nullptr) {
    Cronet_UrlRequestParams_Destroy(params);
  }
  return ret;
}

int32_t Http::Post(const std::string& url,
                   std::shared_ptr<HttpProvider> provider,
                   std::shared_ptr<HttpCallback> callback) {
  return Post(url, std::vector<HttpHeader>(), provider, callback);
}

int32_t Http::Post(const std::string& url,
                   const std::vector<HttpHeader>& headers,
                   std::shared_ptr<HttpProvider> provider,
                   std::shared_ptr<HttpCallback> callback) {
  int32_t ret = kBeeErrorCode_Success;
  Cronet_UrlRequestParamsPtr params = nullptr;
  do {
    if (request_ != nullptr) {
      ret = kBeeErrorCode_Invalid_State;
      break;
    }

    std::shared_ptr<HttpEngine> engine = engine_.lock();
    std::shared_ptr<HttpExecutor> executor = executor_.lock();
    if (url.empty() || provider == nullptr || callback == nullptr ||
        engine == nullptr || executor == nullptr) {
      ret = kBeeErrorCode_Invalid_Param;
      break;
    }

    http_callback_ = callback;
    http_provider_ = provider;
    request_ = Cronet_UrlRequest_Create();
    params = Cronet_UrlRequestParams_Create();
    Cronet_UrlRequestParams_http_method_set(params, "POST");

    upload_provider_ = Cronet_UploadDataProvider_CreateWith(
        Http::UploadProviderGetLengthInternal, Http::UploadProviderReadInternal,
        Http::UploadProviderRewindInternal, Http::UploadProviderCloseInternal);
    Cronet_UploadDataProvider_SetClientContext(upload_provider_, this);
    Cronet_UrlRequestParams_upload_data_provider_set(params, upload_provider_);

    for (auto& header : headers) {
      Cronet_HttpHeaderPtr cronet_header = Cronet_HttpHeader_Create();
      Cronet_HttpHeader_name_set(cronet_header, header.name.c_str());
      Cronet_HttpHeader_value_set(cronet_header, header.value.c_str());
      Cronet_UrlRequestParams_request_headers_add(params, cronet_header);
      Cronet_HttpHeader_Destroy(cronet_header);
    }

    cronet_callback_ = Cronet_UrlRequestCallback_CreateWith(
        Http::OnRedirectReceivedInternal, Http::OnResponseStartedInternal,
        Http::OnReadCompletedInternal, Http::OnSucceededInternal,
        Http::OnFailedInternal, Http::OnCanceledInternal);
    Cronet_UrlRequestCallback_SetClientContext(cronet_callback_, this);

    Cronet_RESULT cronet_res = Cronet_UrlRequest_InitWithParams(
        request_, engine->GetEngine(), url.c_str(), params, cronet_callback_,
        executor->GetExecutor());
    if (cronet_res != Cronet_RESULT_SUCCESS) {
      // LOG
      ret = kBeeErrorCode_Cronet_Request_Init_Failed;
      break;
    }

    cronet_res = Cronet_UrlRequest_Start(request_);
    if (cronet_res != Cronet_RESULT_SUCCESS) {
      // LOG
      ret = kBeeErrorCode_Cronet_Request_Start_Failed;
      break;
    }
  } while (0);
  if (params != nullptr) {
    Cronet_UrlRequestParams_Destroy(params);
  }
  return ret;
}

int32_t Http::FollowRedirect() {
  if (finished_ && request_ == nullptr) {
    return kBeeErrorCode_Invalid_State;
  }

  if (Cronet_UrlRequest_FollowRedirect(request_) != Cronet_RESULT_SUCCESS) {
    // LOG.
    return kBeeErrorCode_Cronet_Request_Redirect_Failed;
  }

  return kBeeErrorCode_Success;
}

int32_t Http::Read(Cronet_BufferPtr buffer) {
  if (finished_ && request_ == nullptr) {
    return kBeeErrorCode_Invalid_State;
  }

  if (Cronet_UrlRequest_Read(request_, buffer) != Cronet_RESULT_SUCCESS) {
    // LOG.
    return kBeeErrorCode_Cronet_Request_Read_Failed;
  }

  return kBeeErrorCode_Success;
}

void Http::Cancel() {
  if (!finished_ && !cancelling_ && request_ != nullptr) {
    cancelling_ = true;
    Cronet_UrlRequest_Cancel(request_);
  }
}

void Http::Close() {
  do {
    if (finished_ || request_ == nullptr) {
      break;
    }

    std::shared_ptr<HttpExecutor> executor = executor_.lock();
    if (executor == nullptr) {
      break;
    }

    // Should not wait in executor thread.
    if (executor->IsExecutorThread()) {
      // LOG ERROR
      break;
    }

    // Create close promise.
    auto close_promise = std::make_shared<std::promise<int32_t>>();

    // Reference count++.
    close_promise_ = close_promise;

    // Cancel request if not cancelling.
    if (!cancelling_) {
      Cancel();
    }

    // Check finished state again in executor thread for safe,
    // assume OnSucceeded/OnFailed/OnCanceled won't be called twice.
    executor->ExecuteFunction([this] {
      // OnSucceeded/OnFailed/OnCanceled could be called before actual Cancel.
      if (finished_ && close_promise_ != nullptr) {
        close_promise_->set_value(0);
        close_promise_ = nullptr;
      }
    });

    // Sync wait for close result.
    std::future<int32_t> future = close_promise->get_future();
    future.wait_until(std::chrono::system_clock::now() +
                      std::chrono::milliseconds(100));
  } while (0);

  if (cronet_callback_ != nullptr) {
    Cronet_UrlRequestCallback_Destroy(cronet_callback_);
    cronet_callback_ = nullptr;
  }

  if (upload_provider_ != nullptr) {
    Cronet_UploadDataProvider_Destroy(upload_provider_);
    upload_provider_ = nullptr;
  }

  if (request_ != nullptr) {
    Cronet_UrlRequest_Destroy(request_);
    request_ = nullptr;
  }
}

void Http::OnRedirectReceivedInternal(Cronet_UrlRequestCallbackPtr callback,
                                      Cronet_UrlRequestPtr request,
                                      Cronet_UrlResponseInfoPtr info,
                                      Cronet_String newLocationUrl) {
  GetThis(callback)->OnRedirectReceived(request, info, newLocationUrl);
}

void Http::OnResponseStartedInternal(Cronet_UrlRequestCallbackPtr callback,
                                     Cronet_UrlRequestPtr request,
                                     Cronet_UrlResponseInfoPtr info) {
  GetThis(callback)->OnResponseStarted(request, info);
}

void Http::OnReadCompletedInternal(Cronet_UrlRequestCallbackPtr callback,
                                   Cronet_UrlRequestPtr request,
                                   Cronet_UrlResponseInfoPtr info,
                                   Cronet_BufferPtr buffer,
                                   uint64_t bytesRead) {
  GetThis(callback)->OnReadCompleted(request, info, buffer, bytesRead);
}

void Http::OnSucceededInternal(Cronet_UrlRequestCallbackPtr callback,
                               Cronet_UrlRequestPtr request,
                               Cronet_UrlResponseInfoPtr info) {
  GetThis(callback)->OnSucceeded(request, info);
}

void Http::OnFailedInternal(Cronet_UrlRequestCallbackPtr callback,
                            Cronet_UrlRequestPtr request,
                            Cronet_UrlResponseInfoPtr info,
                            Cronet_ErrorPtr error) {
  GetThis(callback)->OnFailed(request, info, error);
}

void Http::OnCanceledInternal(Cronet_UrlRequestCallbackPtr callback,
                              Cronet_UrlRequestPtr request,
                              Cronet_UrlResponseInfoPtr info) {
  GetThis(callback)->OnCanceled(request, info);
}

int64_t Http::UploadProviderGetLengthInternal(
    Cronet_UploadDataProviderPtr provider) {
  return GetThis(provider)->UploadProviderGetLength();
}

void Http::UploadProviderReadInternal(Cronet_UploadDataProviderPtr provider,
                                      Cronet_UploadDataSinkPtr upload_data_sink,
                                      Cronet_BufferPtr buffer) {
  GetThis(provider)->UploadProviderRead(upload_data_sink, buffer);
}

void Http::UploadProviderRewindInternal(
    Cronet_UploadDataProviderPtr provider,
    Cronet_UploadDataSinkPtr upload_data_sink) {
  GetThis(provider)->UploadProviderRewind(upload_data_sink);
}

void Http::UploadProviderCloseInternal(Cronet_UploadDataProviderPtr provider) {
  GetThis(provider)->UploadProviderClose();
}

Http* Http::GetThis(Cronet_UrlRequestCallbackPtr callback) {
  return static_cast<Http*>(
      Cronet_UrlRequestCallback_GetClientContext(callback));
}

Http* Http::GetThis(Cronet_UploadDataProviderPtr provider) {
  return static_cast<Http*>(
      Cronet_UploadDataProvider_GetClientContext(provider));
}

void Http::OnRedirectReceived(Cronet_UrlRequestPtr request,
                              Cronet_UrlResponseInfoPtr info,
                              Cronet_String newLocationUrl) {
  std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
  if (http_callback != nullptr) {
    http_callback->OnRedirectReceived(request, info, newLocationUrl);
  }
}

void Http::OnResponseStarted(Cronet_UrlRequestPtr request,
                             Cronet_UrlResponseInfoPtr info) {
  std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
  if (http_callback != nullptr) {
    http_callback->OnResponseStarted(request, info);
  }
}

void Http::OnReadCompleted(Cronet_UrlRequestPtr request,
                           Cronet_UrlResponseInfoPtr info,
                           Cronet_BufferPtr buffer,
                           uint64_t bytes_read) {
  std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
  if (http_callback != nullptr) {
    http_callback->OnReadCompleted(request, info, buffer, bytes_read);
  }
}

void Http::OnSucceeded(Cronet_UrlRequestPtr request,
                       Cronet_UrlResponseInfoPtr info) {
  finished_ = true;
  std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
  if (http_callback != nullptr) {
    http_callback->OnSucceeded(request, info);
  }
}

void Http::OnFailed(Cronet_UrlRequestPtr request,
                    Cronet_UrlResponseInfoPtr info,
                    Cronet_ErrorPtr error) {
  finished_ = true;
  std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
  if (http_callback != nullptr) {
    http_callback->OnFailed(request, info, error);
  }
}

void Http::OnCanceled(Cronet_UrlRequestPtr request,
                      Cronet_UrlResponseInfoPtr info) {
  finished_ = true;
  // Do not report OnCanceled if close directly.
  if (close_promise_ != nullptr) {
    close_promise_->set_value(0);
    close_promise_ = nullptr;
  } else {
    std::shared_ptr<HttpCallback> http_callback = http_callback_.lock();
    if (http_callback != nullptr) {
      http_callback->OnCanceled(request, info);
    }
  }
}

int64_t Http::UploadProviderGetLength() {
  std::shared_ptr<HttpProvider> http_provider = http_provider_.lock();
  if (http_provider != nullptr) {
    return http_provider->GetLength();
  }
  return 0;
}

void Http::UploadProviderRead(Cronet_UploadDataSinkPtr upload_data_sink,
                              Cronet_BufferPtr buffer) {
  std::shared_ptr<HttpProvider> http_provider = http_provider_.lock();
  if (http_provider != nullptr) {
    http_provider->Read(upload_data_sink, buffer);
  }
}

void Http::UploadProviderRewind(Cronet_UploadDataSinkPtr upload_data_sink) {
  std::shared_ptr<HttpProvider> http_provider = http_provider_.lock();
  if (http_provider != nullptr) {
    http_provider->Rewind(upload_data_sink);
  }
}

void Http::UploadProviderClose() {
  std::shared_ptr<HttpProvider> http_provider = http_provider_.lock();
  if (http_provider != nullptr) {
    http_provider->Close();
  }
}

}  // namespace bee
