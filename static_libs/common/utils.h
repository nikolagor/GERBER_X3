#pragma once

#include <QString>
#include <chrono>
#include <map>
#include <string_view>

using nanoseconds__ = std::nano;
using microseconds__ = std::micro;
using milliseconds__ = std::milli;
using seconds__ = std::ratio<1>;
using minutes__ = std::ratio<60>;
using hours__ = std::ratio<3600>;
using std::is_same_v;
template <class T = seconds__>
requires                          //
    is_same_v<T, nanoseconds__> ||  //
    is_same_v<T, microseconds__> || //
    is_same_v<T, milliseconds__> || //
    is_same_v<T, seconds__> ||      //
    is_same_v<T, minutes__> ||      //
    is_same_v<T, hours__>           //

struct Timer {
#if defined(__gnu_linux__) || defined(__GNUC__)
    const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> t1;
#else
    const std::chrono::time_point<std::chrono::steady_clock> t1;
#endif
    const std::string_view stringView;
    static inline std::map<std::string_view, std::pair<size_t, double>> avgMap;

    constexpr Timer(std::string_view name, T = {})
        : t1 { std::chrono::high_resolution_clock::now() }
        , stringView { name } {
    }

    ~Timer() {
        using std::chrono::duration;
        using std::chrono::high_resolution_clock;

        duration<double, T> timeout { high_resolution_clock::now() - t1 };

        auto& [ctr, avg] = avgMap[stringView];
        avg += timeout.count();
        ++ctr;

        /**/ if constexpr (std::is_same_v<T, nanoseconds__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) nanoseconds", stringView.data(), timeout.count(), avg / ctr);
        else if constexpr (std::is_same_v<T, microseconds__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) microseconds", stringView.data(), timeout.count(), avg / ctr);
        else if constexpr (std::is_same_v<T, milliseconds__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) milliseconds", stringView.data(), timeout.count(), avg / ctr);
        else if constexpr (std::is_same_v<T, seconds__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) seconds", stringView.data(), timeout.count(), avg / ctr);
        else if constexpr (std::is_same_v<T, minutes__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) minutes", stringView.data(), timeout.count(), avg / ctr);
        else if constexpr (std::is_same_v<T, hours__>)
            qDebug("%s\n-> %1.3f (avg %1.3f) hours", stringView.data(), timeout.count(), avg / ctr);
    }
};
template <class T>
Timer(std::string_view, T) -> Timer<T>;

inline auto toU16StrView(const QString& str) {
    return std::u16string_view(reinterpret_cast<const char16_t*>(str.utf16()), str.size());
}

template <class T>
struct CtreCapTo {
    T& cap;
    CtreCapTo(T& cap) /*requires class ctre::captured_content<0,void>::storage<class std::_String_view_iterator<struct std::char_traits<char16_t>>>*/
        : cap { cap } {
    }

    auto toDouble() const { return QString(*this).toDouble(); }
    auto toInt() const { return QString(*this).toInt(); }

    operator QString() const {
        //qDebug("QString  D%d S%d", cap.data(), cap.size());
        return QString(reinterpret_cast<const QChar*>(cap.data()), static_cast<size_t>(cap.size()));
    }

    operator double() const { return toDouble(); }
    operator int() const { return toInt(); }
};
template <class T>
CtreCapTo(T) -> CtreCapTo<T>;
