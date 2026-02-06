#pragma once

#include <string>
#include <regex>
#include <algorithm>
#include <locale>

template <class T>
void TrimRight(T& s, const T& trimSet)
{
    typename T::size_type const n = s.find_last_not_of(trimSet);

    if (n != std::string::npos)
    {
        s = s.substr(0, n + 1);
    }
    else
    {
        s.erase();
    }
}

template <class T>
void TrimLeft(T& s, const T& trimSet)
{
    typename T::size_type const n = s.find_first_not_of(trimSet);

    if (n != std::string::npos)
    {
        s = s.substr(n);
    }
    else
    {
        s.erase();
    }
}

template <class T>
void Trim(T& s, const T& trimSet)
{
    TrimRight(s, trimSet);
    TrimLeft(s, trimSet);
}

template <class T>
T CopyToLower(const T& s)
{
    T t(s);
    std::transform(t.begin(), t.end(), t.begin(), [](typename T::value_type c) -> typename T::value_type
        {
            return static_cast<typename T::value_type>(std::tolower(static_cast<unsigned char>(c)));
        });
    return t;
}

template <class T>
T CopyToUpper(const T& s)
{
    T t(s);
    std::transform(t.begin(), t.end(), t.begin(), [](typename T::value_type c) -> typename T::value_type
        {
            return static_cast<typename T::value_type>(std::toupper(static_cast<unsigned char>(c)));
        });
    return t;
}

template <class T>
T ToLower(T&& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](typename T::value_type c) -> typename T::value_type
        {
            return static_cast<typename T::value_type>(std::tolower(static_cast<unsigned char>(c)));
        });
    return s;
}

template <class T>
T ToUpper(T&& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](typename T::value_type c) -> typename T::value_type
        {
            return static_cast<typename T::value_type>(std::toupper(static_cast<unsigned char>(c)));
        });
    return s;
}

template<class Pr>
bool ParseRegEx(const std::string& strToParse, const std::string& strRegExp, Pr pred)
{
    const std::regex e(strRegExp);

    std::sregex_token_iterator i(strToParse.cbegin(), strToParse.cend(), e);
    const std::sregex_token_iterator end;

    if (i != end)
    {
        do
        {
            if (!pred(*i))
            {
                break;
            }
        } while (++i != end);

        return true;
    }
    return false;
}