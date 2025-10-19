#include <iostream>
#include <coroutine>
#include <exception>

template<typename T>
class generator {
public:
    struct promise_type {
        T current_value;
        
        auto get_return_object() {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        
        auto yield_value(T value) {
            current_value = value;
            return std::suspend_always{};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    struct iterator {
        std::coroutine_handle<promise_type> handle;
        bool done;
        
        iterator(std::coroutine_handle<promise_type> h, bool d) : handle(h), done(d) {
            if (!done) {
                handle.resume();
                done = handle.done();
            }
        }
        
        iterator& operator++() {
            handle.resume();
            done = handle.done();
            return *this;
        }
        
        bool operator!=(const iterator& other) const {
            return done != other.done;
        }
        
        const T& operator*() const {
            return handle.promise().current_value;
        }
    };
    
    iterator begin() {
        return iterator{handle, false};
    }
    
    iterator end() {
        return iterator{handle, true};
    }
    
    generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    
    ~generator() {
        if (handle) handle.destroy();
    }
    
    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;
    
    generator(generator&& other) : handle(other.handle) {
        other.handle = nullptr;
    }
    
    generator& operator=(generator&& other) {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    
private:
    std::coroutine_handle<promise_type> handle;
};

generator<unsigned int> iota(unsigned int n = 0) {
    while (true)
        co_yield n += 1234;
}

int main() {
    auto number_generator = iota(0);

    for (const auto& num : number_generator) {
        std::cout << num << std::endl;
        if (num > 5000) {
            break;
        }
    }
    return 0;
}