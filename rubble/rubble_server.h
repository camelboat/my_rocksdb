#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string> 
#include <memory>
// #include <nlohmann/json.hpp>
#include <unordered_map>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/alarm.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include "rubble_kv_store.grpc.pb.h"
#include "reply_client.h"
#include "forwarder.h"

#include "rocksdb/db.h"
#include "port/port_posix.h"
#include "port/port.h"
#include "db/version_edit.h"
#include "db/db_impl/db_impl.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "util/aligned_buffer.h"
#include "file/read_write_util.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerReaderWriter;
using grpc::ServerAsyncReaderWriter;
using grpc::Status;
using grpc::StatusCode;

using rubble::RubbleKvStoreService;
using rubble::SyncRequest;
using rubble::SyncReply;
using rubble::Op;
using rubble::OpReply;
using rubble::SingleOp;
using rubble::SingleOpReply;
using rubble::SingleOp_OpType_Name;

// using json = nlohmann::json;
using std::chrono::time_point;
using std::chrono::high_resolution_clock;

int g_thread_num = 16;
int g_cq_num = 8;
int g_pool = 1;

std::unordered_map<std::thread::id, int> map;

//implements the Sync rpc call 
class SyncServiceImpl final : public  RubbleKvStoreService::WithAsyncMethod_DoOp<RubbleKvStoreService::Service> {
  public:
    explicit SyncServiceImpl(rocksdb::DB* db)
      :db_(db), impl_((rocksdb::DBImpl*)db_), 
       mu_(impl_->mutex()),
       version_set_(impl_->TEST_GetVersionSet()),
       db_options_(version_set_->db_options()),
    //    is_rubble_(db_options_->is_rubble),
       is_head_(db_options_->is_primary),
       is_tail_(db_options_->is_tail),
       db_path_(db_options_->db_paths.front()),
    //    sync_client_(db_options_->sync_client),
       column_family_set_(version_set_->GetColumnFamilySet()),
       default_cf_(column_family_set_->GetDefault()),
       ioptions_(default_cf_->ioptions()),
       cf_options_(default_cf_->GetCurrentMutableCFOptions()),
       fs_(ioptions_->fs){
        //  if(is_rubble_ && !is_head_){
        //    ios_ = CreateSstPool();
        //    if(!ios_.ok()){
        //      std::cout << "allocate sst pool failed \n";
        //      assert(false);
        //    }
        //  }
    };

    ~SyncServiceImpl() {
      delete db_;
    }
  
    //an Unary RPC call used by the non-tail node to sync Version(view of sst files) states to the downstream node 
  Status Sync(ServerContext* context, const SyncRequest* request, 
                          SyncReply* reply) override
  {
        return Status::OK;
  }

  private:
   
    // a db op request we get from the client
    Op request_;
    //reply we get back to the client for a db op
    OpReply reply_;

    std::atomic<uint64_t> op_counter_{0};
    time_point<high_resolution_clock> start_time_;
    time_point<high_resolution_clock> end_time_;
    // db instance
    rocksdb::DB* db_ = nullptr;
    rocksdb::DBImpl* impl_ = nullptr;
    // db's mutex
    rocksdb::InstrumentedMutex* mu_;
    // db status after processing an operation
    rocksdb::Status s_;
    rocksdb::IOStatus ios_;

    // rocksdb's version set
    rocksdb::VersionSet* version_set_;

    rocksdb::ColumnFamilySet* column_family_set_;
    // db's default columnfamily data 
    rocksdb::ColumnFamilyData* default_cf_;
    // rocksdb internal immutable db options
    const rocksdb::ImmutableDBOptions* db_options_;
    // rocksdb internal immutable column family options
    const rocksdb::ImmutableCFOptions* ioptions_;
    const rocksdb::MutableCFOptions* cf_options_;
  
    // right now, just put all sst files under one path
    rocksdb::DbPath db_path_;

    // maintain a mapping between sst_number and slot_number
    // sst_bit_map_[i] = j means sst_file with number j occupies the i-th slot
    // secondary node will update it when received a Sync rpc call from the upstream node
    std::unordered_map<int, uint64_t> sst_bit_map_;
    
    rocksdb::FileSystem* fs_;

    std::atomic<uint64_t> log_apply_counter_;

    // client for making Sync rpc call to downstream node
    // std::shared_ptr<SyncClient> sync_client_;

    // is rubble mode? If set to false, server runs a vanilla rocksdb
    bool is_rubble_ = false;
    bool is_head_ = false;
    bool is_tail_ = false;

    /* variables below are used for Sync method */
    // if true, means version edit received indicates a flush job
    bool is_flush_ = false; 
    // indicates if version edit corresponds to a trivial move compaction
    bool is_trivial_move_compaction_ = false;
    // number of added sst files
    int num_of_added_files_ = 0;
    // number of memtables get flushed in a flush job
    int batch_count_ = 0;
    // get the next file num of secondary, which is the maximum file number of the AddedFiles in the shipped vesion edit plus 1
    int next_file_num_ = 0;
};

class CallDataBase {
public:
  CallDataBase(SyncServiceImpl* service, 
                ServerCompletionQueue* cq, 
                rocksdb::DB* db, 
                std::shared_ptr<Channel> channel)
   :service_(service), cq_(cq), db_(db), 
    channel_(channel){

   }

  virtual void Proceed(bool ok) = 0;

  virtual void HandleOp() = 0;

protected:

  // db instance
  rocksdb::DB* db_;

  // status of the db after performing an operation.
  rocksdb::Status s_;

  const rocksdb::ImmutableDBOptions* db_options_;

  std::shared_ptr<Channel> channel_ = nullptr;
  std::shared_ptr<Forwarder> forwarder_ = nullptr;
  // client for sending the reply back to the replicator
  std::shared_ptr<ReplyClient> reply_client_ = nullptr;
  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  SyncServiceImpl* service_;
  // The producer-consumer queue where for asynchronous server notifications.
  ServerCompletionQueue* cq_;

  // Context for the rpc, allowing to tweak aspects of it such as the use
  // of compression, authentication, as well as to send metadata back to the
  // client.
  ServerContext ctx_;

  // What we get from the client.
  Op request_;
  // What we send back to the client.
  OpReply reply_;

};

// CallData for Bidirectional streaming rpc 
class CallDataBidi : CallDataBase {

 public:

  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallDataBidi(SyncServiceImpl* service, 
                ServerCompletionQueue* cq, 
                rocksdb::DB* db,
                std::shared_ptr<Channel> channel)
   :CallDataBase(service, cq, db, channel),rw_(&ctx_){
    // Invoke the serving logic right away.

    status_ = BidiStatus::CONNECT;

    ctx_.AsyncNotifyWhenDone((void*)this);

    // As part of the initial CREATE state, we *request* that the system
    // start processing DoOp requests. In this request, "this" acts are
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    service_->RequestDoOp(&ctx_, &rw_, cq_, cq_, (void*)this);
    db_options_ = ((rocksdb::DBImpl*)db_)->TEST_GetVersionSet()->db_options(); 
    if(db_options_->is_tail){
        // reply_client_ =std::make_shared<ReplyClient>(channel_);
    }else{
      // std::cout << " creating forwarder for thread  " << map[std::this_thread::get_id()] << " \n";
      forwarder_ = std::make_shared<Forwarder>(channel_);
      // std::cout << " Forwarder created \n";
    }
  }
  // async version of DoOp
  void Proceed(bool ok) override {

    std::unique_lock<std::mutex> _wlock(this->m_mutex);

    switch (status_) {
    case BidiStatus::READ:
        // std::cout << "I'm at READ state ! \n";
        //Meaning client said it wants to end the stream either by a 'writedone' or 'finish' call.
        if (!ok) {
            std::cout << "thread:" << map[std::this_thread::get_id()] << " tag:" << this << " CQ returned false." << std::endl;
            Status _st(StatusCode::OUT_OF_RANGE,"test error msg");
            // rw_.Write(reply_, (void*)this);
            rw_.Finish(_st,(void*)this);
            status_ = BidiStatus::DONE;
            std::cout << "thread:" << map[std::this_thread::get_id()] << " tag:" << this << " after call Finish(), cancelled:" << this->ctx_.IsCancelled() << std::endl;
            break;
        }

        // std::cout << "thread:" << std::setw(2) << map[std::this_thread::get_id()] << " Received a new " << Op_OpType_Name(request_.type()) << " op witk key : " << request_.key() << std::endl;
        // Handle a db operation
        // std::cout << "entering HandleOp\n";
        HandleOp();
        /* chain replication */
        // Forward the request to the downstream node in the chain if it's not a tail node
        if(!db_options_->is_tail){
          // std::cout << "Forward an op " << request_.key() << "\n";
          assert(request_.ops_size());
          forwarder_->Forward(request_);
          // std::cout << "Complete forwarding an op " << request_.key() << "\n";
        }else {
          // tail node should be responsible for sending the reply back to replicator
          // use the sync stream to write the reply back
          if (reply_client_ == nullptr) {
            // std::cout << "init the reply client" << "\n";
            reply_client_ = std::make_shared<ReplyClient>(grpc::CreateChannel(
            db_options_->target_address, grpc::InsecureChannelCredentials()));
          }
	  reply_client_->SendReply(reply_);
          reply_.clear_replies();

        }
        rw_.Read(&request_, (void*)this);
        status_ = BidiStatus::READ;
        break;

    case BidiStatus::WRITE:
        std::cout << "I'm at WRITE state ! \n";
        // std::cout << "thread:" << map[std::this_thread::get_id()] << " tag:" << this << " Get For key : " << request_.key() << " , status : " << reply_.status() << std::endl;
        // For a get request, return a reply back to the client
        rw_.Read(&request_, (void*)this);

        status_ = BidiStatus::READ;
        break;

    case BidiStatus::CONNECT:
        std::cout << "thread:" << map[std::this_thread::get_id()] << " tag:" << this << " connected:" << std::endl;
        // Spawn a new CallData instance to serve new clients while we process
        // the one for this CallData. The instance will deallocate itself as
        // part of its FINISH state.
        new CallDataBidi(service_, cq_, db_, channel_);
        rw_.Read(&request_, (void*)this);
        // request_.set_type(Op::PUT);
        // std::cout << "thread:" << std::setw(2) << map[std::this_thread::get_id()] << " Received a new " << Op_OpType_Name(request_.type()) << " op witk key : " << request_.key() << std::endl;
        status_ = BidiStatus::READ;
        break;

    case BidiStatus::DONE:
        std::cout << "thread:" << std::this_thread::get_id() << " tag:" << this
                << " Server done, cancelled:" << this->ctx_.IsCancelled() << std::endl;
        status_ = BidiStatus::FINISH;
        break;

    case BidiStatus::FINISH:
        std::cout << "thread:" << map[std::this_thread::get_id()] <<  "tag:" << this << " Server finish, cancelled:" << this->ctx_.IsCancelled() << std::endl;
        _wlock.unlock();
        delete this;
        break;

    default:
        std::cerr << "Unexpected tag " << int(status_) << std::endl;
        assert(false);
    }
  }

 private:

  void HandleOp() override {
    std::string value;
    if(!op_counter_.load()){
      start_time_ = high_resolution_clock::now();
    }

    if(op_counter_.load() && op_counter_.load()%100000 == 0){
      end_time_ = high_resolution_clock::now();
      auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(end_time_ - start_time_);
      std::cout << "Throughput : handled 100000 ops in " << millisecs.count() << " millisecs\n";
      start_time_ = end_time_;
    }

    // ASSUME that each batch is with the same type of operation
    assert(request_.ops_size() > 0);
    op_counter_ += request_.ops_size();
    SingleOpReply* reply;
    reply_.clear_replies();
    // std::cout << "thread: " << map[std::this_thread::get_id()] << " counter: " << op_counter_ 
    //     <<  " first key in batch: " << request_.ops(0).key() << " size: " << request_.ops_size() << "\n";
    switch (request_.ops(0).type())
    {
      case SingleOp::GET:
        for(const auto& request: request_.ops()) {
          reply = reply_.add_replies();
          reply->set_id(request.id());
          s_ = db_->Get(rocksdb::ReadOptions(), request.key(), &value);
          reply->set_key(request.key());
          reply->set_type(SingleOpReply::GET);
          reply->set_status(s_.ToString());
          if(s_.ok()){
            reply->set_ok(true);
            reply->set_value(value);
          }else{
            reply->set_ok(false);
          }
        }
        break;
      case SingleOp::PUT:
        for(const auto& request: request_.ops()) {
            opcount++;
          start_put_ = high_resolution_clock::now();
          s_ = db_->Put(rocksdb::WriteOptions(), request.key(), request.value());
          end_put_ = high_resolution_clock::now();
          putcounter_ += std::chrono::duration_cast<std::chrono::nanoseconds>(end_put_ - start_put_).count();
          if (opcount > 0 && opcount %10000 == 0) {
            std::cout << "thread:" << map[std::this_thread::get_id()] << " opcount: " <<  opcount << "putcounter: " << putcounter_ *(1.0) / 10000 << "\n";
            putcounter_ = 0;
          }


        //   if (op_counter_.load() && op_counter_.load()%10000 == 0) {
        //     std::cout << "last put time difference:" << std::chrono::duration_cast<std::chrono::nanoseconds>(end_put_ - start_put_).count() << " nanoseconds" 
        //       << "\n average in past 10000 " << putcounter_/10000.0 << std::endl;
        //   }
        
          assert(s_.ok());
        //   std::cout << "Put ok with key: " << request.key() << "\n";
          // return to replicator if tail
          if(db_options_->is_tail){
            reply = reply_.add_replies();
            reply->set_id(request.id());
            reply->set_type(SingleOpReply::PUT);
            reply->set_key(request.key());
            if(s_.ok()){
              // std::cout << "Put : (" << request.key() /* << " ," << request_.value() */ << ")\n"; 
              reply->set_ok(true);
            }else{
              std::cout << "Put Failed : " << s_.ToString() << std::endl;
              reply->set_ok(false);
              reply->set_status(s_.ToString());
            }
          }
        }
        break;
      case SingleOp::DELETE:
        //TODO
        break;

      case SingleOp::UPDATE:
        // std::cout << "in UPDATE " << request_.ops(0).key() << "\n"; 
        for(const auto& request: request_.ops()) {
          reply = reply_.add_replies();
          reply->set_id(request.id());
          s_ = db_->Get(rocksdb::ReadOptions(), request.key(), &value);
          s_ = db_->Put(rocksdb::WriteOptions(), request.key(), request.value());
          reply->set_key(request.key());
          reply->set_type(SingleOpReply::UPDATE);
          reply->set_status(s_.ToString());
          if(s_.ok()){
            reply->set_ok(true);
            reply->set_value(value);
          }else{
            reply->set_ok(false);
          }
        }
        break;

      default:
        std::cerr << "Unsupported Operation \n";
        break;
    }
  }
  
  // The means to get back to the client.
  ServerAsyncReaderWriter<OpReply, Op>  rw_;

  // Let's implement a tiny state machine with the following states.
  enum class BidiStatus { READ = 1, WRITE = 2, CONNECT = 3, DONE = 4, FINISH = 5 };
  BidiStatus status_;

  std::mutex   m_mutex;

  std::atomic<uint64_t> op_counter_{0};
  int reply_counter_{0};
  time_point<high_resolution_clock> start_time_;
  time_point<high_resolution_clock> end_time_;
  time_point<high_resolution_clock> start_put_;
  time_point<high_resolution_clock> end_put_;
  int putcounter_{0};
  int opcount{0};
};

class ServerImpl final {
  public:
  ServerImpl(const std::string& server_addr, rocksdb::DB* db, SyncServiceImpl* service)
   :server_addr_(server_addr), db_(db), service_(service){
      auto db_options = ((rocksdb::DBImpl*)db_)->TEST_GetVersionSet()->db_options(); 
      std::cout << "target address : " << db_options->target_address << std::endl;
      if(db_options->target_address != ""){
        channel_ = grpc::CreateChannel(db_options->target_address, grpc::InsecureChannelCredentials());
        assert(channel_ != nullptr);
      }
  }
  ~ServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    for (const auto& _cq : m_cq)
        _cq->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {

    builder_.AddListeningPort(server_addr_, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an asynchronous DoOp service and synchronous Sync service
    builder_.RegisterService(service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
   
    for (int i = 0; i < g_cq_num; ++i) {
        //cq_ = builder.AddCompletionQueue();
        m_cq.emplace_back(builder_.AddCompletionQueue());
    }

    // Finally assemble the server.
    server_ = builder_.BuildAndStart();
  
    // Proceed to the server's main loop.
    std::vector<std::thread*> _vec_threads;

    for (int i = 0; i < g_thread_num; ++i) {
        int _cq_idx = i % g_cq_num;
        for (int j = 0; j < g_pool; ++j) {
            new CallDataBidi(service_, m_cq[_cq_idx].get(), db_, channel_);
        }
        auto new_thread =  new std::thread(&ServerImpl::HandleRpcs, this, _cq_idx);
        map[new_thread->get_id()] = i;
        std::cout <<  std::setw(2) << i << " th thread spawned \n";
        _vec_threads.emplace_back(new_thread);
    }

    std::cout << g_thread_num << " working aysnc threads spawned" << std::endl;

    for (const auto& _t : _vec_threads)
        _t->join();
  }

 private:

  // This can be run in multiple threads if needed.
  void HandleRpcs(int cq_idx) {
    // Spawn a new CallDataUnary instance to serve new clients.
    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallDataUnary instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(m_cq[cq_idx]->Next(&tag, &ok));

      CallDataBase* _p_ins = (CallDataBase*)tag;
      _p_ins->Proceed(ok);
    }
  }

  std::shared_ptr<Channel> channel_ = nullptr;
  std::vector<std::unique_ptr<ServerCompletionQueue>>  m_cq;
  SyncServiceImpl* service_;
  std::unique_ptr<Server> server_;
  const std::string& server_addr_;
  ServerBuilder builder_;
  rocksdb::DB* db_;
};

void RunServer(rocksdb::DB* db, const std::string& server_addr, int thread_num = 16) {
  
  SyncServiceImpl service(db);
  g_thread_num = 16;
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  // ServerBuilder builder;

  // builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
  std::cout << "Server listening on " << server_addr << std::endl;
  // builder.RegisterService(&service);
  ServerImpl server_impl(server_addr, db, &service);
  server_impl.Run();
}

