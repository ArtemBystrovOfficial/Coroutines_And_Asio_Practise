#include <coroutine>
#include <iostream>

struct generator {
	struct promise_type {
		using coro_handle = std::coroutine_handle<promise_type>;
		auto get_return_object() {
			return coro_handle::from_promise(*this);
		}
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() { exit(0); }
	};
	using coro_handle = promise_type::coro_handle;

	generator(coro_handle handle) : handle_(handle) {}
	generator(generator&) = delete;
	generator(generator&& gen) noexcept : handle_(gen.handle_) { gen.handle_ = nullptr; }

	bool resume() {
		if (!handle_.done()) 
			handle_.resume();
		return !handle_.done();
	}

	~generator() { handle_.destroy(); }
private:
	coro_handle handle_;
};

generator coroInit() {
	std::cout << "Coro initial\n";
	co_await std::suspend_always();
	std::cout << "Coro finish\n";
}

int main() {
	auto get = coroInit();
	std::cout << "coro middle\n";
	get.resume();
}