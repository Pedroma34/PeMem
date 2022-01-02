#pragma once
#include <chrono>

namespace pemem {
	class Time {
		std::chrono::system_clock::time_point m_time1;
		std::chrono::system_clock::time_point m_time2;
		float m_elapsed;
	public:
		Time() : m_time1(std::chrono::system_clock::now()), m_time2(std::chrono::system_clock::now()),
			m_elapsed(0) {}
		~Time() {}
		void Update() {
			m_time2 = std::chrono::system_clock::now();
			std::chrono::duration<float> elapsed = m_time2 - m_time1;
			m_time1 = m_time2;
			m_elapsed = elapsed.count();
		}

		const float& GetElapsed() {
			return m_elapsed;
		}
	};
}