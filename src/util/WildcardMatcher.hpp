#pragma once

#include <string>
#include <cctype>
#include <cwctype>

namespace devkit::WildcardMatcher {

    // Трейт для сравнения символов без учета регистра
    template<typename CharT>
    struct CaseInsensitiveCharTraits {
        static bool equal(CharT c1, CharT c2) {
            return c1 == c2; // По умолчанию простое сравнение
        }
    };

    // Специализация для char
    template<>
    struct CaseInsensitiveCharTraits<char> {
        static bool equal(char c1, char c2) {
            return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
        }
    };

    // Специализация для wchar_t
    template<>
    struct CaseInsensitiveCharTraits<wchar_t> {
        static bool equal(wchar_t c1, wchar_t c2) {
            return std::towlower(c1) == std::towlower(c2);
        }
    };

    template<typename CharT, typename Traits = std::char_traits<CharT>>
    static bool MatchWildcard(
        const std::basic_string<CharT, Traits>& text,
        const std::basic_string<CharT, Traits>& pattern) {

        using string_type = std::basic_string<CharT, Traits>;

        size_t textLen = text.length();
        size_t patternLen = pattern.length();
        size_t textPos = 0, patternPos = 0;
        size_t starPos = string_type::npos;
        size_t matchPos = 0;

        while (textPos < textLen) {
            if (patternPos < patternLen &&
                (pattern[patternPos] == static_cast<CharT>('?') ||
                    CaseInsensitiveCharTraits<CharT>::equal(pattern[patternPos], text[textPos]))) {
                // Символы совпадают или '?'
                ++textPos;
                ++patternPos;
            } else if (patternPos < patternLen && pattern[patternPos] == static_cast<CharT>('*')) {
                // Запоминаем позицию звездочки и текущую позицию в тексте
                starPos = patternPos;
                matchPos = textPos;
                ++patternPos; // Пропускаем звездочку в паттерне
            } else if (starPos != string_type::npos) {
                // Не совпало, но у нас есть активная звездочка
                patternPos = starPos + 1;
                ++matchPos;
                textPos = matchPos;
            } else {
                // Не совпало и нет активных звездочек
                return false;
            }
        }

        // Дошли до конца текста, проверяем остаток паттерна
        while (patternPos < patternLen && pattern[patternPos] == static_cast<CharT>('*')) {
            ++patternPos;
        }

        return patternPos == patternLen;
    }

    inline bool MatchWildcard(std::string_view text, std::string_view pattern) {
        return MatchWildcard(std::string(text), std::string(pattern));
    }

    inline bool MatchWildcard(std::wstring_view text, std::wstring_view pattern) {
        return MatchWildcard(std::wstring(text), std::wstring(pattern));
    }
}