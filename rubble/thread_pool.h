#pragma once

#include <list>
#include "boost/thread/thread.hpp"
#include "boost/thread/future.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include "boost/foreach.hpp"
#include "boost/utility/result_of.hpp"
#include <vector>

template <typename Task>
class thread_pool {

    typedef typename Task::result_type Result;
    typedef boost::shared_future<Result>  SharedFuture;
    typedef std::list<SharedFuture > Futures;

    public:
    ///@brief Constructor.
    thread_pool(std::size_t pool_size =  boost::thread::hardware_concurrency())
        : work_(io_service_)
        //   available_(pool_size)
        {


            for (std::size_t i = 0; i < pool_size; ++i){
                threads_.create_thread(boost::bind(&boost::asio::io_service::run,
                                                    &io_service_));
            }

            std::cout << "thread pool initialized , size : " << pool_size << std::endl;
        }

    /// @brief Destructor.
    ~thread_pool(){
        // Force all threads to return from io_service::run().
        io_service_.stop();

        // Suppress all exceptions.
        try{
            threads_.join_all();
        }
        catch (const std::exception&) {}
    }

    /// @brief Adds a task to the thread pool if a thread is currently available.
    // this call is gonna block if there is no avilable thread
    void run_task(Task task){

        WaitForAvailableThread();

        typedef boost::packaged_task<Result> PackagedTask;
        boost::shared_ptr<PackagedTask> packaged_task_ptr = boost::make_shared<PackagedTask>(boost::bind(task));
        io_service_.post(boost::bind(&PackagedTask::operator(), packaged_task_ptr));

        boost::shared_future<Result> future(packaged_task_ptr->get_future());
        futures_.push_back(future);
    }

    // Returns the number of tasks that are queued but not yet completed.
    uint NumPendingTasks(){
        uint num_pending = 0;
        for(auto const& i: futures_){
            if (!i.is_ready()){
                ++num_pending;
            }
        }
        return num_pending;
    }

    void WaitForAvailableThread(){
    if (NumPendingTasks() >= threads_.size()){
            boost::wait_for_any(futures_.begin(), futures_.end());
        }
    }

    //get the number of completed task
    std::vector<Result> GetCompletedTaskResult(){
        std::vector<Result> results;
        typename Futures::iterator iter;
        for (iter = futures_.begin(); iter != futures_.end(); ++iter){
            if (iter->is_ready()){
                results.push_back(iter->get());
                futures_.erase(iter);
            }
        }

    return results;
  }

    private:
        boost::asio::io_service io_service_;
        boost::asio::io_service::work work_;
        boost::thread_group threads_;
        Futures futures_;
        boost::mutex mutex_;

};