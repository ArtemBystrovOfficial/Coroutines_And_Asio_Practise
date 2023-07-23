#include <coroutine>
#include <iostream>

#pragma once
#pragma warning ( push )
#pragma warning( disable : 4996)

template  <class T>
struct generator {
	class iterator : public std::iterator<std::input_iterator_tag,
		int, int, const int*, int> {
	public:
		explicit iterator(generator & gen,int num) : gen_ref(gen), _num(num) {}
		iterator& operator++() {
			if (!gen_ref.handle_.done()) {
				gen_ref.handle_.resume();
				if (gen_ref.handle_.done())
					_num = -1;
				else
					_num = gen_ref.handle_.promise().current_value;
			}
			else
				_num = -1;
			return *this;
		}
		bool operator ==(iterator other) const { return _num == other._num; }
		bool operator !=(iterator other) const { return _num != other._num; }
		reference operator*() const { return _num; }
	private:
		generator& gen_ref;
		int _num;
	};
	friend class iterator;

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

	iterator begin() {
		int begin_value;
		if (!handle_.done()) {
			handle_.resume();
			begin_value = handle_.promise().current_value;
		}
		else
			begin_value = -1;
		return iterator(*this, begin_value);
	}

	iterator end() {
		return iterator(*this, -1);
	}

	T get_next() {
		if (!handle_.done()) 
			handle_.resume();
		return handle_.promise().current_value;
	}

	~generator() { handle_.destroy(); }
private:
	coro_handle handle_;
};

generator<int> range(int start, int fin, int step) {
	int num;
	for (auto num = start; num < fin; num += step)
		co_yield num;
}

int main() {
	for (auto num : range(5, 20, 3)) 
		std::cout << num << std::endl;

}
