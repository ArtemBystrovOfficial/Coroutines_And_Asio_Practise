#include <coroutine>
#include <iostream>

template  <class T>
struct generator {
	struct promise_type {
		T current_value;

		using coro_handle = std::coroutine_handle<promise_type>;
		auto get_return_object() {
			return coro_handle::from_promise(*this);
		}
		auto yield_value(T value) {
			current_value = value;
			return std::suspend_always{};
		}
		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() { exit(0); }
	};
	using coro_handle = promise_type::coro_handle;

	generator(coro_handle handle) : handle_(handle) {}
	generator(generator&) = delete;
	generator(generator&& gen) noexcept : handle_(gen.handle_) { gen.handle_ = nullptr; }

	T get_next() {
		if (!handle_.done()) 
			handle_.resume();
		return handle_.promise().current_value;
	}

	~generator() { handle_.destroy(); }
private:
	coro_handle handle_;
};

generator<int> coroInit() {
	int num = 0;
	for (;;) {
		co_yield num;
		num += 1;
	}
}

int main() {
	auto gen = coroInit();
	
	auto y =  gen.get_next();
	auto z = gen.get_next();

	std::cout << y << " " << z << std::endl;
}