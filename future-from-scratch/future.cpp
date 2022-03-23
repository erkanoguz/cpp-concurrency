#include <future>
#include <thread>
#include <chrono>
#include <iostream>
#include <condition_variable>
#include <memory>

template <typename T>
struct SharedState 
{
    T _value;
    std::exception_ptr _exception;
    bool _ready;
    std::mutex _mtx;
    std::condition_variable _cv;
};

template <typename T>
class Future
{
public:
    void wait() const 
    {
        if(!_state) throw "No State!\n";
        std::unique_lock<std::mutex> lck(_state->_mtx);
        while(!_state->_ready)
            _state->_cv.wait(lck);
    }

    T get() 
    {
        wait();
        auto statePtr = std::move(_state);
        if (statePtr->_exception) 
            std::rethrow_exception(statePtr->_exception);
        
        return std::move(statePtr->_value);
    }

    bool valid() const 
    {
        return (state != nullptr);
    }

    bool ready() const 
    {
        if (!_state)
            return false;
        std::lock_guard<std::mutex> lck(_state->_mtx);
        return _state->_ready;
    }

private:
    std::shared_ptr<SharedState<T>> _state;
};


template <typename T> 
class Promise
{
public:
    Promise() = default;
    ~Promise(){}

    void set_value(T value)
    {
        if (false == _state) throw "No State!\n";
        std::lock_guard<std::mutex> lck(_state->mtx);
        if (true == _state->ready) throw "Promise already satisfied!\n";
        _state->_value = std::move(value);
        _state->_ready = true;
        _state->_cv.notify_all();
    }

    void set_exception(std::exception_ptr e)
    {
        if (false == _state) throw "No State!\n";
        std::lock_guard<std::mutex> lck(_state->mtx);
        if (true == _state->ready) throw "Promise already satisfied!\n";
        _state->_exception = std::move(e);
        _state->_ready = true;
        _state->_cv.notify_all();
    }

    Future<T> get_future() 
    {
        if (false == _state) throw "No State!\n";
        if (true == future_retreived) throw "Future already retrived!\n";
        future_retreived = true;
        return Future<T>(_state);
    }
private:
    std::shared_ptr<SharedState<T>> _state;
    bool future_retreived{false};
};


int main() 
{
}
