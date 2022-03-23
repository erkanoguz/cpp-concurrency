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

    Future(std::shared_ptr<SharedState<T>> state)
        : _state(state)
    {}
    Future(const Future&) = delete;
    Future(Future&& other) noexcept
        : _state(other._state)
    {}
    ~Future(){}

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
        return (_state != nullptr);
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
    Promise()
        : _state(std::make_shared<SharedState<T>>())
    {
    };

    ~Promise(){}

    void set_value(T value)
    {
        if (!_state) throw "No State!\n";
        std::lock_guard<std::mutex> lck(_state->_mtx);
        if (true == _state->_ready) throw "Promise already satisfied!\n";
        _state->_value = std::move(value);
        _state->_ready = true;
        _state->_cv.notify_all();
    }

    void set_exception(std::exception_ptr e)
    {
        if (!_state) throw "No State!\n";
        std::lock_guard<std::mutex> lck(_state->_mtx);
        if (true == _state->_ready) throw "Promise already satisfied!\n";
        _state->_exception = std::move(e);
        _state->_ready = true;
        _state->_cv.notify_all();
    }

    Future<T> get_future()
    {
        if (_state == nullptr) std::cout << "No State!\n";
        if (true == future_retreived) throw "Future already retrived!\n";
        future_retreived = true;
        return Future<T>(_state);
    }
private:
    std::shared_ptr<SharedState<T>> _state;
    bool future_retreived{false};
};


int foo()
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 1;
}

int main() 
{
    Promise<int> p;
    Future<int> res = p.get_future();

    p.set_value(foo());

    std::cout << "result: " << res.get() << "\n";
}
