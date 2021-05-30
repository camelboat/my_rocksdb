// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: rubble_kv_store.proto

#include "rubble_kv_store.pb.h"
#include "rubble_kv_store.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace rubble {

static const char* RubbleKvStoreService_method_names[] = {
  "/rubble.RubbleKvStoreService/Sync",
  "/rubble.RubbleKvStoreService/DoOp",
  "/rubble.RubbleKvStoreService/SendReply",
};

std::unique_ptr< RubbleKvStoreService::Stub> RubbleKvStoreService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< RubbleKvStoreService::Stub> stub(new RubbleKvStoreService::Stub(channel));
  return stub;
}

RubbleKvStoreService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Sync_(RubbleKvStoreService_method_names[0], ::grpc::internal::RpcMethod::BIDI_STREAMING, channel)
  , rpcmethod_DoOp_(RubbleKvStoreService_method_names[1], ::grpc::internal::RpcMethod::BIDI_STREAMING, channel)
  , rpcmethod_SendReply_(RubbleKvStoreService_method_names[2], ::grpc::internal::RpcMethod::BIDI_STREAMING, channel)
  {}

::grpc::ClientReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* RubbleKvStoreService::Stub::SyncRaw(::grpc::ClientContext* context) {
  return ::grpc::internal::ClientReaderWriterFactory< ::rubble::SyncRequest, ::rubble::SyncReply>::Create(channel_.get(), rpcmethod_Sync_, context);
}

void RubbleKvStoreService::Stub::experimental_async::Sync(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::SyncRequest,::rubble::SyncReply>* reactor) {
  ::grpc::internal::ClientCallbackReaderWriterFactory< ::rubble::SyncRequest,::rubble::SyncReply>::Create(stub_->channel_.get(), stub_->rpcmethod_Sync_, context, reactor);
}

::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* RubbleKvStoreService::Stub::AsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::SyncRequest, ::rubble::SyncReply>::Create(channel_.get(), cq, rpcmethod_Sync_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::rubble::SyncRequest, ::rubble::SyncReply>* RubbleKvStoreService::Stub::PrepareAsyncSyncRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::SyncRequest, ::rubble::SyncReply>::Create(channel_.get(), cq, rpcmethod_Sync_, context, false, nullptr);
}

::grpc::ClientReaderWriter< ::rubble::Op, ::rubble::OpReply>* RubbleKvStoreService::Stub::DoOpRaw(::grpc::ClientContext* context) {
  return ::grpc::internal::ClientReaderWriterFactory< ::rubble::Op, ::rubble::OpReply>::Create(channel_.get(), rpcmethod_DoOp_, context);
}

void RubbleKvStoreService::Stub::experimental_async::DoOp(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::Op,::rubble::OpReply>* reactor) {
  ::grpc::internal::ClientCallbackReaderWriterFactory< ::rubble::Op,::rubble::OpReply>::Create(stub_->channel_.get(), stub_->rpcmethod_DoOp_, context, reactor);
}

::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>* RubbleKvStoreService::Stub::AsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::Op, ::rubble::OpReply>::Create(channel_.get(), cq, rpcmethod_DoOp_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::rubble::Op, ::rubble::OpReply>* RubbleKvStoreService::Stub::PrepareAsyncDoOpRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::Op, ::rubble::OpReply>::Create(channel_.get(), cq, rpcmethod_DoOp_, context, false, nullptr);
}

::grpc::ClientReaderWriter< ::rubble::OpReply, ::rubble::Reply>* RubbleKvStoreService::Stub::SendReplyRaw(::grpc::ClientContext* context) {
  return ::grpc::internal::ClientReaderWriterFactory< ::rubble::OpReply, ::rubble::Reply>::Create(channel_.get(), rpcmethod_SendReply_, context);
}

void RubbleKvStoreService::Stub::experimental_async::SendReply(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::rubble::OpReply,::rubble::Reply>* reactor) {
  ::grpc::internal::ClientCallbackReaderWriterFactory< ::rubble::OpReply,::rubble::Reply>::Create(stub_->channel_.get(), stub_->rpcmethod_SendReply_, context, reactor);
}

::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>* RubbleKvStoreService::Stub::AsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::OpReply, ::rubble::Reply>::Create(channel_.get(), cq, rpcmethod_SendReply_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::rubble::OpReply, ::rubble::Reply>* RubbleKvStoreService::Stub::PrepareAsyncSendReplyRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::rubble::OpReply, ::rubble::Reply>::Create(channel_.get(), cq, rpcmethod_SendReply_, context, false, nullptr);
}

RubbleKvStoreService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RubbleKvStoreService_method_names[0],
      ::grpc::internal::RpcMethod::BIDI_STREAMING,
      new ::grpc::internal::BidiStreamingHandler< RubbleKvStoreService::Service, ::rubble::SyncRequest, ::rubble::SyncReply>(
          [](RubbleKvStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReaderWriter<::rubble::SyncReply,
             ::rubble::SyncRequest>* stream) {
               return service->Sync(ctx, stream);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RubbleKvStoreService_method_names[1],
      ::grpc::internal::RpcMethod::BIDI_STREAMING,
      new ::grpc::internal::BidiStreamingHandler< RubbleKvStoreService::Service, ::rubble::Op, ::rubble::OpReply>(
          [](RubbleKvStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReaderWriter<::rubble::OpReply,
             ::rubble::Op>* stream) {
               return service->DoOp(ctx, stream);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      RubbleKvStoreService_method_names[2],
      ::grpc::internal::RpcMethod::BIDI_STREAMING,
      new ::grpc::internal::BidiStreamingHandler< RubbleKvStoreService::Service, ::rubble::OpReply, ::rubble::Reply>(
          [](RubbleKvStoreService::Service* service,
             ::grpc::ServerContext* ctx,
             ::grpc::ServerReaderWriter<::rubble::Reply,
             ::rubble::OpReply>* stream) {
               return service->SendReply(ctx, stream);
             }, this)));
}

RubbleKvStoreService::Service::~Service() {
}

::grpc::Status RubbleKvStoreService::Service::Sync(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::SyncReply, ::rubble::SyncRequest>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status RubbleKvStoreService::Service::DoOp(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::OpReply, ::rubble::Op>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status RubbleKvStoreService::Service::SendReply(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::rubble::Reply, ::rubble::OpReply>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace rubble

