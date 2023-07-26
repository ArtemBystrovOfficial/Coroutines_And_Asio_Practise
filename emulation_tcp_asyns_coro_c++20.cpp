#include <coroutine>
#include <iostream>
#include <list>
#include <thread>
#include <functional>

using coro_t = std::coroutine_handle<>;

struct evt_awaiter_t {
public:
	enum class Type {
		Reciver,
		Connector
	};
private:
	struct awaiter {
		evt_awaiter_t* event_;
		coro_t coro_ = nullptr;
		int counter_wait = 1;
		Type type;

		awaiter(evt_awaiter_t & event, Type tp) noexcept : event_(&event), type(tp) {}
		awaiter& operator =(const awaiter& aw) {
			event_ = aw.event_;
			coro_ = aw.coro_;
			counter_wait = aw.counter_wait;
			type = aw.type;

			return *this;
		}
		bool await_ready() const noexcept { return false; }
		void await_resume() noexcept { }
		void await_suspend(coro_t coro) noexcept {
			coro_ = coro;
			event_->push_awaiter(*this);
		}
	};
public:
	evt_awaiter_t(bool set = false) {}
	evt_awaiter_t(const  evt_awaiter_t&) = delete;
	evt_awaiter_t& operator=(const evt_awaiter_t&) = delete;
	evt_awaiter_t(evt_awaiter_t&&) = delete;
	evt_awaiter_t& operator=(evt_awaiter_t&&) = delete;

	void set_next_type(Type tp) {
		m_next_type_ = tp;
	}

	void push_awaiter(awaiter a) { 
		m_list.push_back(a); 
	}
	void set() noexcept {
		while (!m_list.empty()) {
			//Если есть условно говоря входящие подключения
			auto& elem = m_list[quene_index];

			if (elem.type == Type::Connector) {
				elem.counter_wait++;
				if (elem.counter_wait % 100 != 0) {
					quene_index++;
					if (quene_index >= m_list.size()) 
						quene_index = 0;
					continue;
				}
			}
			if (elem.type == Type::Reciver) {
				elem.counter_wait++;
				if (elem.counter_wait % 3 != 0) {
					quene_index++;
					if (quene_index >= m_list.size())
						quene_index = 0;
					continue;
				}
			}
			elem.coro_.resume();
			m_list.erase(m_list.begin()+quene_index);
			if(quene_index > 0)
				quene_index--;
		}
	}
	awaiter operator co_await() noexcept {
		return awaiter{ *this, m_next_type_ };
	}
private:
	int quene_index = 0;
	Type m_next_type_;
	std::vector<awaiter> m_list;
};

struct no_resumable_no_own {
	struct promise_type {
		using coro_handle = std::coroutine_handle<promise_type>;
		auto get_return_object() {
			return coro_handle::from_promise(*this);
		}
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() { exit(0); }
	};
	using coro_handle = promise_type::coro_handle;

	no_resumable_no_own(coro_handle handle) {}
	no_resumable_no_own(no_resumable_no_own&) {}
	no_resumable_no_own(no_resumable_no_own&& gen) noexcept {}
};

evt_awaiter_t g_evt;
int id = 0;

no_resumable_no_own asyns_read(std::function<void()> call_back) {
	g_evt.set_next_type(evt_awaiter_t::Type::Reciver);
	co_await g_evt;
	call_back();
}

no_resumable_no_own asyns_accept(std::function<void()> call_back) {
	g_evt.set_next_type(evt_awaiter_t::Type::Connector);
	co_await g_evt;
	call_back();
}

void recive(int id_client) {
	asyns_read([=] { std::cout << "readed bytes by id: " << id_client << "\n"; recive(id_client); });
}

void accepter() {
	asyns_accept([=] { std::cout << "accepted: " <<id << "\n"; recive(id); id++; accepter(); });
}

int main() {
	accepter();
	g_evt.set();
}