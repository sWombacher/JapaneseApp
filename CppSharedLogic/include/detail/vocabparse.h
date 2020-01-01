#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace detail {

    std::wstring convertUtf8Wstring(const std::string& str);
    std::string convertWstringUtf8(const std::wstring& str);

    struct Vocabulary {
        enum class Type {
            // have to be ascending ordered
            // to be sorted correctly
            N5,
            N4,
            N3,
            N2,
            N1,
            UNKNOWN
        };

        Type type = Type::UNKNOWN;

        std::wstring kana;
        std::wstring kanji;
        std::vector<std::wstring> english;

        Vocabulary() = default;
        Vocabulary(std::wstring_view _kana,
                   const std::vector<std::wstring>& _english,
                   Type _type = Type::UNKNOWN, std::wstring_view _kanji = L"");

        bool operator==(const Vocabulary& rhs) const;
        bool operator!=(const Vocabulary& rhs) const;

        // not yet working
        [[nodiscard]] static std::wstring
            ConvertKanaToRomanji(std::wstring_view kana);

        [[nodiscard]] static std::wstring
            ConvertKanaToHiraganaOnly(std::wstring_view kana);
        [[nodiscard]] static std::wstring
            ConvertKanaToKatakanaOnly(std::wstring_view kana);

        friend std::wostream& operator<<(std::wostream& os,
                                         const Vocabulary& voc);

        static const std::vector<Vocabulary> HiraganaMultiCharacters;
        static const std::vector<Vocabulary> HiraganaSingleCharacters;

        static const std::vector<Vocabulary> KatakanaMultiCharacters;
        static const std::vector<Vocabulary> KatakanaSingleCharacters;
    };

    struct VocabularyVector : public std::vector<Vocabulary> {
        [[nodiscard]] std::vector<const_iterator>
            findAllEnglish(std::wstring_view english) const;

        [[nodiscard]] std::vector<const_iterator>
            findAllKana(std::wstring_view kana) const;

        [[nodiscard]] std::vector<const_iterator>
            findAllByType(Vocabulary::Type type) const;

        [[nodiscard]] std::vector<const_iterator>
            findAllIf(std::function<bool(const Vocabulary&)> predicate) const;
    };

    struct VocParser {
        VocParser() = delete;

        // parse jmdict file format,
        // link: https://www.edrdg.org/jmdict/j_jmdict.html
        [[nodiscard]] static std::optional<VocabularyVector>
            parseJMDictFile(const std::filesystem::path& fileName);
        [[nodiscard]] static std::optional<VocabularyVector>
            parseJMDictData(std::string_view data);

        // parse anki files from:
        // http://www.tanos.co.uk/jlpt/
        [[nodiscard]] static std::optional<VocabularyVector> parseAnkiDataBase(
            const std::filesystem::path& fileNameKanjiEng,
            const std::filesystem::path& fileNameKanjiHiragana);
    };

} // namespace detail
