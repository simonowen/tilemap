#pragma once

HRESULT Fail(HRESULT hr, LPCWSTR pszOperation);
std::vector<uint8_t> FileContents(const std::wstring &filename);

LONG WINAPI CrashDumpUnhandledExceptionFilter(EXCEPTION_POINTERS * ExceptionInfo);

template <typename T>
T TileToPixels(T index, T width, T gap = 0)
{
	return index * (width + gap);
}

template <typename T>
T lerp(float t, T a, T b)
{
	return a + t * (b - a);
}

uint32_t random_uint32();

template <typename T>
class EasedValue
{
public:
	EasedValue(T init_value, std::chrono::steady_clock::duration duration_ms)
		: m_current_value(init_value), m_end_value(init_value), m_duration(duration_ms)
	{
	}

	operator T() const
	{
		return m_end_value;
	}

	bool IsAnimating()
	{
		return easedValue() != m_end_value;
	}

	void setValue(T new_value, bool instant = false)
	{
		if (instant)
		{
			m_current_value = m_end_value = new_value;
		}
		else
		{
			m_start_time = std::chrono::steady_clock::now();
			m_start_value = m_end_value;
			m_end_value = new_value;
		}
	}

	void setRelativeValue(T new_value, bool instant = false)
	{
		setValue(m_end_value + new_value, instant);
	}

	T easedValue()
	{
		if (m_current_value != m_end_value)
		{
			auto elapsed = std::chrono::steady_clock::now() - m_start_time;
			if (elapsed >= m_duration)
				m_current_value = m_end_value;
			else
			{
				auto num = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
				auto den = std::chrono::duration_cast<std::chrono::milliseconds>(m_duration).count();
				auto t = static_cast<float>(num) / den;
				auto relative = sinf(1.5707963f * t);
				m_current_value = lerp(relative, m_start_value, m_end_value);
			}
		}

		return m_current_value;
	}

private:
	T m_current_value{};
	T m_start_value{};
	T m_end_value{};
	std::chrono::steady_clock::time_point m_start_time{};
	std::chrono::steady_clock::duration m_duration{};
};
