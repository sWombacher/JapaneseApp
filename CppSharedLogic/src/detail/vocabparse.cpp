#include "detail/vocabparse.h"

#include <algorithm>
#include <assert.h>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <locale>
#include <string_view>
#include <sstream>

#include <sqlite3.h>
#include <tinyxml2.h>

inline static constexpr const wchar_t katakanaMin = L'ァ';
inline static constexpr const wchar_t katakanaMax = L'ヶ';

inline static constexpr const wchar_t hiraganaMin = L'ぁ';
inline static constexpr const wchar_t hiraganaMax = L'ゖ';

namespace detail {

    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>
        stringWstringConverter;

    std::wstring convertUtf8Wstring(const std::string& str) {
        return stringWstringConverter.from_bytes(str);
    }

    std::string convertWstringUtf8(const std::wstring& str) {
        return stringWstringConverter.to_bytes(str);
    }

    constexpr int simpleHash(std::string_view view) {
        int res = 0;
        for (const auto& e : view)
            res += e;
        return res;
    }

    template <typename _Function>
    void for_each_node(const tinyxml2::XMLNode& node, _Function&& function) {
        for (auto child = node.FirstChild(); child;
             child = child->NextSibling())
            function(*child);
    }

    std::wstring parse__r_ele(const tinyxml2::XMLNode& node) {
        std::wstring kana;
        for_each_node(node, [&](const tinyxml2::XMLNode& node) {
            switch (simpleHash(node.ToElement()->Name())) {
            case simpleHash("reb"):
                assert(kana.empty());
                kana = convertUtf8Wstring(node.ToElement()->GetText());
                break;
            case simpleHash("re_nokanji"):
                // always empty?
                break;
            case simpleHash("re_restr"):
                // stuff for kanji and romanji with hiragana?
                break;
            case simpleHash("re_pri"):
                // indicator for how common the vocabluary is
                break;
            case simpleHash("re_inf"):
                // Typically it will be used to indicate some unusual aspect of
                // the reading
                break;
            default:
                assert(false);
            }
        });
        assert(!kana.empty());
        return kana;
    }

    std::vector<std::wstring> parse__sense(const tinyxml2::XMLNode& node) {
        std::vector<std::wstring> english;
        for_each_node(node, [&](const tinyxml2::XMLNode& node) {
            switch (simpleHash(node.ToElement()->Name())) {
            case simpleHash("pos"):
                // porbably no needed?
                break;
            case simpleHash("gloss"): {
                std::wstring&& eng =
                    convertUtf8Wstring(node.ToElement()->GetText());
                english.push_back(std::move(eng));
            } break;
            case simpleHash("xref"):
                // cross reference, not used
                break;
            case simpleHash("misc"):
                // other info, not used
                break;
            case simpleHash("dial"):
                // dialects, not used
                break;
            case simpleHash("s_inf"):
                // additional info, not used
                break;
            case simpleHash("stagr"):
                // not used
                break;
            case simpleHash("field"):
                // field of the word like 'sport' or 'food'
                // not used
                break;
            case simpleHash("lsource"):
                // not used
                break;
            case simpleHash("stagk"):
                // some kind of restriction
                // not used
                break;
            case simpleHash("ant"):
                // not used
                break;
            default:
                assert(false);
            }
        });
        assert(!english.empty());
        return english;
    }

    Vocabulary parseEntry(const tinyxml2::XMLNode& node) {
        Vocabulary voc;
        for_each_node(node, [&](const tinyxml2::XMLNode& child) {
            const std::string name = child.ToElement()->Name();
            switch (simpleHash(name)) {
            case simpleHash("ent_seq"):
                // only id?
                break;
            case simpleHash("r_ele"):
                voc.kana = parse__r_ele(child);
                break;
            case simpleHash("sense"): {
                const auto english = parse__sense(child);
                voc.english.insert(voc.english.end(), english.cbegin(),
                                   english.cend());
            } break;
            case simpleHash("k_ele"):
                // containing kaji, not used
                break;
            default:
                assert(false);
            }
        });
        assert(!voc.kana.empty());
        assert(!voc.english.empty());
        return voc;
    }

    std::optional<VocabularyVector>
        VocParser::parseJMDictFile(const boost::filesystem::path& fileName) {
        std::fstream file(fileName, std::fstream::in);
        std::string data((std::istreambuf_iterator<char>(file)),
                         (std::istreambuf_iterator<char>()));
        return parseJMDictData(data);
    }

    std::optional<VocabularyVector>
        VocParser::parseJMDictData(std::string_view data) {
        using namespace tinyxml2;

        XMLDocument document;
        if (document.Parse(&data[0], data.size()) != XMLError::XML_SUCCESS ||
            !document.RootElement())
            return {};

        auto voc = std::make_optional<VocabularyVector>();
        for_each_node(*document.RootElement(), [&](const XMLNode& node) {
            voc->push_back(parseEntry(node));
        });
        return voc;
    }

    struct AnkiParseResult {
        std::string utf8question;
        std::string utf8answer;
        bool operator<(const AnkiParseResult& rhs) const {
            return this->utf8question < rhs.utf8question;
        }
    };
    [[nodiscard]] static std::vector<AnkiParseResult>
        _parseAnkiDataBase(std::string_view filename) {

        sqlite3* db = nullptr;
        if (const int res = sqlite3_open(filename.data(), &db);
            res != SQLITE_OK) {
            throw std::runtime_error("Unable to open database, error: " +
                                     std::to_string(res));
        }
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "", -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error("Unable to prepare statement");

        constexpr const char* sqlCommand = "SELECT question, answer FROM cards";
        using ResultType = decltype(_parseAnkiDataBase(""));

        auto callBack = [](void* result, int argc, char** argv,
                           char** /* azColName */) {
            assert(argc == 2);
            auto& res = *static_cast<ResultType*>(result);
            auto& element = res.emplace_back();
            element.utf8question = argv[0];
            element.utf8answer = argv[1];
            return 0;
        };
        char* errMsg = nullptr;
        ResultType result;
        if (sqlite3_exec(db, sqlCommand, callBack, &result, &errMsg) !=
            SQLITE_OK)
            throw std::runtime_error("Unable to call exec on database");

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        auto extractVoc = [](std::string& str) {
            const auto p0 = str.find('>');
            const auto p1 = str.rfind('<');
            assert(p0 != std::string::npos);
            assert(p1 != std::string::npos);
            assert(p1 > p0);
            str = str.substr(p0 + 1, p1 - p0 - 1);
        };
        for (auto& e : result) {
            if (e.utf8question.empty() || e.utf8answer.empty())
                continue;

            extractVoc(e.utf8answer);
            extractVoc(e.utf8question);
        }
        return result;
    }

    [[nodiscard]] std::vector<std::wstring> splitAnki(const std::wstring& str) {
        std::vector<std::wstring> split;
        std::wstringstream wsstr(str);
        for (std::wstring value; std::getline(wsstr, value, L',');)
            split.push_back(value);

        return split;
    }

    [[nodiscard]] std::optional<VocabularyVector> _merge_KanjiEng_KanjiHiragana(
        const decltype(_parseAnkiDataBase(""))& sortedKanjiEng,
        const decltype(_parseAnkiDataBase(""))& sortedKanjiHiragna) {

        const auto& eng = sortedKanjiEng;
        const auto& hira = sortedKanjiHiragna;
        auto vm = std::make_optional<VocabularyVector>();

        for (auto iterEng = eng.cbegin(), iterHira = hira.cbegin();
             iterEng != eng.cend() && iterHira != hira.cend();
             iterEng->utf8question < iterHira->utf8question ? ++iterEng
                                                            : ++iterHira) {
            if (iterEng->utf8question == iterHira->utf8question) {
                auto& element = vm->emplace_back();
                element.kanji = convertUtf8Wstring(iterEng->utf8question);
                element.kana = convertUtf8Wstring(iterHira->utf8answer);
                element.english =
                    splitAnki(convertUtf8Wstring(iterEng->utf8answer));
            }
        }
        return vm;
    }

    void _appendAnki_KanjiActualHiragana(
        VocabularyVector& vm,
        const decltype(_parseAnkiDataBase(""))& kanjiEng) {

        auto isHiragana = [](std::wstring_view text) {
            for (const auto& e : text) {
                if (e < hiraganaMin || e > hiraganaMax)
                    return false;
            }
            return true;
        };
        for (const auto& e : kanjiEng) {
            std::wstring quest = convertUtf8Wstring(e.utf8question);
            if (isHiragana(quest)) {
                auto& element = vm.emplace_back();
                element.kana = std::move(quest);
                element.english = splitAnki(convertUtf8Wstring(e.utf8answer));
            }
        }
    }

    std::optional<VocabularyVector> VocParser::parseAnkiDataBase(
        const boost::filesystem::path& fileNameKanjiEng,
        const boost::filesystem::path& fileNameKanjiHiragana) {
        (void)sqlite3_initialize();
        auto eng = _parseAnkiDataBase(fileNameKanjiEng.string());
        auto hira = _parseAnkiDataBase(fileNameKanjiHiragana.string());
        std::sort(eng.begin(), eng.end());
        std::sort(hira.begin(), hira.end());

        auto vm = _merge_KanjiEng_KanjiHiragana(eng, hira);
        assert(vm);

        // kanji may only be hiragana symbos...
        // we have to add those...
        _appendAnki_KanjiActualHiragana(*vm, eng);

        return vm;
    }

    std::wostream& operator<<(std::wostream& os, const Vocabulary& voc) {
        os << L"Kana: " << voc.kana << L"\nEnglish:\n";
        for (const auto& e : voc.english)
            os << e << L"; ";
        os << L'\n';
        return os;
    }

    namespace impl {

        template <typename _IteratorType, typename _VocManager,
                  typename _SearchFunction, typename _PartitonFunction>
        std::vector<_IteratorType> _findAll(_VocManager& voc,
                                            _SearchFunction&& searchFunc,
                                            _PartitonFunction&& partitionFunc) {
            std::vector<_IteratorType> result;
            for (auto iter = std::find_if(voc.begin(), voc.end(), searchFunc);
                 iter != voc.end();
                 ++iter, iter = std::find_if(iter, voc.end(), searchFunc))
                result.push_back(iter);

            auto iter =
                std::partition(result.begin(), result.end(),
                               std::forward<_PartitonFunction>(partitionFunc));

            std::sort(result.begin(), iter,
                      [](_IteratorType lhs, _IteratorType rhs) {
                          return int(lhs->type) < int(rhs->type);
                      });

            return result;
        }

        template <typename _IteratorType, typename _VocManager>
        std::vector<_IteratorType> _findAllEnglish(_VocManager& voc,
                                                   std::wstring_view english) {
            auto searchFunc = [&](const Vocabulary& voc) {
                return std::find_if(voc.english.begin(), voc.english.end(),
                                    [&](const std::wstring& str) {
                                        const auto start = str.find(english);
                                        if (start == std::wstring::npos)
                                            return false;

                                        if (start > 0 && str[start - 1] != L' ')
                                            return false;

                                        const auto end = start + english.size();
                                        if (end < str.size() &&
                                            str[end] != L' ')
                                            return false;

                                        return true;
                                    }) != voc.english.end();
            };
            auto partitionFunc = [&](const _IteratorType i) {
                return std::find(i->english.cbegin(), i->english.cend(),
                                 english) != i->english.cend();
            };
            return _findAll<_IteratorType>(voc, searchFunc, partitionFunc);
        }

        template <typename _IteratorType, typename _VocManager>
        std::vector<_IteratorType> _findAllKana(_VocManager& voc,
                                                std::wstring_view kana) {
            auto searchFunction = [&](const Vocabulary& str) {
                return str.kana.find(kana) != std::wstring::npos;
            };
            auto partitionFunc = [&](const _IteratorType i) {
                return i->kana == kana;
            };
            return _findAll<_IteratorType>(voc, searchFunction, partitionFunc);
        }

    } // namespace impl

    std::vector<std::vector<Vocabulary>::const_iterator>
        VocabularyVector::findAllEnglish(std::wstring_view english) const {
        return impl::_findAllEnglish<const_iterator>(*this, english);
    }

    std::vector<std::vector<Vocabulary>::const_iterator>
        VocabularyVector::findAllKana(std::wstring_view kana) const {
        return impl::_findAllKana<const_iterator>(*this, kana);
    }

    std::vector<std::vector<Vocabulary>::const_iterator>
        VocabularyVector::findAllByType(Vocabulary::Type type) const {
        auto searchFunc = [type](const Vocabulary& voc) {
            return voc.type == type;
        };
        // partition function not required -> use default one
        auto partitionFunc = [](const auto&) { return true; };
        return impl::_findAll<const_iterator>(*this, searchFunc, partitionFunc);
    }

    std::vector<std::vector<Vocabulary>::const_iterator>
        VocabularyVector::findAllIf(
            std::function<bool(const Vocabulary&)> predicate) const {

        auto partitionFunc = [](const auto&) { return true; };
        return impl::_findAll<const_iterator>(*this, predicate, partitionFunc);
    }

    std::wstring Vocabulary::ConvertKanaToHiraganaOnly(std::wstring_view kana) {
        std::wstring res(kana);
        std::transform(res.cbegin(), res.cend(), res.begin(), [](wchar_t c) {
            if (c >= katakanaMin && c <= katakanaMax)
                c = c - katakanaMin + hiraganaMin;
            return c;
        });
        auto secondBegin = res.begin();
        ++secondBegin;
        std::transform(secondBegin, res.end(), res.begin(), secondBegin,
                       [](wchar_t second, wchar_t first) {
                           return second == L'ー' ? first : second;
                       });
        return res;
    }

    std::wstring Vocabulary::ConvertKanaToKatakanaOnly(std::wstring_view kana) {
        std::wstring res(kana);
        std::transform(res.cbegin(), res.cend(), res.begin(), [](wchar_t c) {
            if (c >= hiraganaMin && c <= hiraganaMax)
                c = c - hiraganaMin + katakanaMin;
            return c;
        });
        auto secondBegin = res.begin();
        ++secondBegin;
        std::transform(secondBegin, res.end(), res.begin(), secondBegin,
                       [](wchar_t second, wchar_t first) {
                           return first == second ? L'ー' : second;
                       });
        return res;
    }

    Vocabulary::Vocabulary(std::wstring_view _kana,
                           const std::vector<std::wstring>& _english,
                           Vocabulary::Type _type, std::wstring_view _kanji)
        : type(_type), kana(_kana), kanji(_kanji), english(_english) {}

    bool Vocabulary::operator==(const Vocabulary& rhs) const {
        return this->kana == rhs.kana && this->type == rhs.type &&
               this->kanji == rhs.kanji && this->english == rhs.english;
    }

    bool Vocabulary::operator!=(const Vocabulary& rhs) const {
        return !(*this == rhs);
    }

    std::wstring Vocabulary::ConvertKanaToRomanji(std::wstring_view kana) {
        std::wstring res;
        const auto hiragana = Vocabulary::ConvertKanaToHiraganaOnly(kana);
        res.reserve(hiragana.size() *
                    2); // on average about 2 symbols per hiaragana

        std::for_each(hiragana.cbegin(), hiragana.cend(), [&res](wchar_t h) {
            switch (h) {
            case L'ぁ':
            case L'あ':
                res += L'a';
                break;
            case L'ぃ':
            case L'い':
                res += L'i';
                break;
            case L'ぅ':
            case L'う':
                res += L'u';
                break;
            case L'ぇ':
            case L'え':
                res += L'e';
                break;
            case L'ぉ':
            case L'お':
                res += L'o';
                break;
            case L'ゕ':
            case L'か':
                res += L"ka";
                break;
            case L'が':
                res += L"ga";
                break;
            case L'き':
                res += L"ki";
                break;
            case L'ぎ':
                res += L"gi";
                break;
            case L'く':
                res += L"ku";
                break;
            case L'ぐ':
                res += L"gu";
                break;
            case L'ゖ':
            case L'け':
                res += L"ke";
                break;
            case L'げ':
                res += L"ge";
                break;
            case L'こ':
                res += L"ko";
                break;
            case L'ご':
                res += L"go";
                break;
            case L'さ':
                res += L"sa";
                break;
            case L'ざ':
                res += L"za";
                break;
            case L'し':
                res += L"shi";
                break;
            case L'じ':
                res += L"ji";
                break;
            case L'す':
                res += L"su";
                break;
            case L'ず':
                res += L"zu";
                break;
            case L'せ':
                res += L"se";
                break;
            case L'ぜ':
                res += L"ze";
                break;
            case L'そ':
                res += L"so";
                break;
            case L'ぞ':
                res += L"zo";
                break;
            case L'た':
                res += L"ta";
                break;
            case L'だ':
                res += L"da";
                break;
            case L'ち':
                res += L"chi";
                break;
            case L'ぢ':
                res += L"ji";
                break;
            case L'っ':
            case L'つ':
                res += L"tsu";
                break;
            case L'づ':
                res += L"zu";
                break;
            case L'て':
                res += L"te";
                break;
            case L'で':
                res += L"de";
                break;
            case L'と':
                res += L"to";
                break;
            case L'ど':
                res += L"do";
                break;
            case L'な':
                res += L"na";
                break;
            case L'に':
                res += L"ni";
                break;
            case L'ぬ':
                res += L"nu";
                break;
            case L'ね':
                res += L"ne";
                break;
            case L'の':
                res += L"no";
                break;
            case L'は':
                res += L"ha";
                break;
            case L'ば':
                res += L"ba";
                break;
            case L'ぱ':
                res += L"pa";
                break;
            case L'ひ':
                res += L"hi";
                break;
            case L'び':
                res += L"bi";
                break;
            case L'ぴ':
                res += L"pi";
                break;
            case L'ふ':
                res += L"fu";
                break;
            case L'ぶ':
                res += L"bu";
                break;
            case L'ぷ':
                res += L"pu";
                break;
            case L'へ':
                res += L"he";
                break;
            case L'べ':
                res += L"be";
                break;
            case L'ぺ':
                res += L"pe";
                break;
            case L'ほ':
                res += L"ho";
                break;
            case L'ぼ':
                res += L"bo";
                break;
            case L'ぽ':
                res += L"po";
                break;
            case L'ま':
                res += L"ma";
                break;
            case L'み':
                res += L"mi";
                break;
            case L'む':
                res += L"mu";
                break;
            case L'め':
                res += L"me";
                break;
            case L'も':
                res += L"mo";
                break;
            case L'ゃ':
            case L'や':
                res += L"ya";
                break;
            case L'ゅ':
            case L'ゆ':
                res += L"yu";
                break;
            case L'ょ':
            case L'よ':
                res += L"yo";
                break;
            case L'ら':
                res += L"ra";
                break;
            case L'り':
                res += L"ri";
                break;
            case L'る':
                res += L"ru";
                break;
            case L'れ':
                res += L"re";
                break;
            case L'ろ':
                res += L"ro";
                break;
            case L'ゎ':
            case L'わ':
                res += L"wa";
                break;
            case L'を':
                res += L"wo";
                break;
            case L'ん':
                res += L'n';
                break;
            default:
                break;
            }
        });
        // maybe add support for 'ju -> jiyu' and so on
        return res;
    }

    const std::vector<Vocabulary> Vocabulary::HiraganaSingleCharacters = {
        {L"あ", {L"a"}},   {L"い", {L"i"}},  {L"う", {L"u"}},
        {L"え", {L"e"}},   {L"お", {L"o"}},  {L"か", {L"ka"}},
        {L"が", {L"ga"}},  {L"き", {L"ki"}}, {L"ぎ", {L"gi"}},
        {L"く", {L"ku"}},  {L"ぐ", {L"gu"}}, {L"け", {L"ke"}},
        {L"げ", {L"ge"}},  {L"こ", {L"ko"}}, {L"ご", {L"go"}},
        {L"さ", {L"sa"}},  {L"ざ", {L"za"}}, {L"し", {L"shi"}},
        {L"じ", {L"ji"}},  {L"す", {L"su"}}, {L"ず", {L"zu"}},
        {L"せ", {L"se"}},  {L"ぜ", {L"ze"}}, {L"そ", {L"so"}},
        {L"ぞ", {L"zo"}},  {L"た", {L"ta"}}, {L"だ", {L"da"}},
        {L"ち", {L"chi"}}, {L"ぢ", {L"ji"}}, {L"つ", {L"tsu"}},
        {L"づ", {L"zu"}},  {L"て", {L"te"}}, {L"で", {L"de"}},
        {L"と", {L"to"}},  {L"ど", {L"do"}}, {L"な", {L"na"}},
        {L"に", {L"ni"}},  {L"ぬ", {L"nu"}}, {L"ね", {L"ne"}},
        {L"の", {L"no"}},  {L"は", {L"ha"}}, {L"ば", {L"ba"}},
        {L"ぱ", {L"pa"}},  {L"ひ", {L"hi"}}, {L"び", {L"bi"}},
        {L"ぴ", {L"pi"}},  {L"ふ", {L"fu"}}, {L"ぶ", {L"bu"}},
        {L"ぷ", {L"pu"}},  {L"へ", {L"he"}}, {L"べ", {L"be"}},
        {L"ぺ", {L"pe"}},  {L"ほ", {L"ho"}}, {L"ぼ", {L"bo"}},
        {L"ぽ", {L"po"}},  {L"ま", {L"ma"}}, {L"み", {L"mi"}},
        {L"む", {L"mu"}},  {L"め", {L"me"}}, {L"も", {L"mo"}},
        {L"や", {L"ya"}},  {L"ゆ", {L"yu"}}, {L"よ", {L"yo"}},
        {L"ら", {L"ra"}},  {L"り", {L"ri"}}, {L"る", {L"ru"}},
        {L"れ", {L"re"}},  {L"ろ", {L"ro"}}, {L"わ", {L"wa"}},
        {L"を", {L"wo"}},  {L"ん", {L"n"}},
    };
    const std::vector<Vocabulary> Vocabulary::HiraganaMultiCharacters = {
        {L"りゃ", {L"rya"}}, {L"りゅ", {L"ryu"}}, {L"りょ", {L"ryo"}},
        {L"みゃ", {L"mya"}}, {L"みゅ", {L"myu"}}, {L"みょ", {L"myo"}},
        {L"ぴゃ", {L"pya"}}, {L"ぴゅ", {L"pyu"}}, {L"ぴょ", {L"pyo"}},
        {L"びゃ", {L"bya"}}, {L"びゅ", {L"byu"}}, {L"びょ", {L"byo"}},
        {L"ひゃ", {L"hya"}}, {L"ひゅ", {L"hyu"}}, {L"ひょ", {L"hyo"}},
        {L"にゃ", {L"nya"}}, {L"にゅ", {L"nyu"}}, {L"にょ", {L"nyo"}},
        {L"ちゃ", {L"cha"}}, {L"ちゅ", {L"chu"}}, {L"ちょ", {L"cho"}},
        {L"じゃ", {L"ja"}},  {L"じゅ", {L"ju"}},  {L"じょ", {L"jo"}},
        {L"しゃ", {L"sha"}}, {L"しゅ", {L"shu"}}, {L"しょ", {L"sho"}},
        {L"ぎゃ", {L"gya"}}, {L"ぎゅ", {L"gyu"}}, {L"ぎょ", {L"gyo"}},
        {L"きゃ", {L"kya"}}, {L"きゅ", {L"kyu"}}, {L"きょ", {L"kyo"}},
    };

    const std::vector<Vocabulary> Vocabulary::KatakanaMultiCharacters = {
        {L"ア", {L"a"}},   {L"イ", {L"i"}},  {L"ウ", {L"u"}},
        {L"エ", {L"e"}},   {L"オ", {L"o"}},  {L"カ", {L"ka"}},
        {L"ガ", {L"ga"}},  {L"キ", {L"ki"}}, {L"ギ", {L"gi"}},
        {L"ク", {L"ku"}},  {L"グ", {L"gu"}}, {L"ケ", {L"ke"}},
        {L"ゲ", {L"ge"}},  {L"コ", {L"ko"}}, {L"ゴ", {L"go"}},
        {L"サ", {L"sa"}},  {L"ザ", {L"za"}}, {L"シ", {L"shi"}},
        {L"ジ", {L"ji"}},  {L"ス", {L"su"}}, {L"ズ", {L"zu"}},
        {L"セ", {L"se"}},  {L"ゼ", {L"ze"}}, {L"ソ", {L"so"}},
        {L"ゾ", {L"zo"}},  {L"タ", {L"ta"}}, {L"ダ", {L"da"}},
        {L"チ", {L"chi"}}, {L"ヂ", {L"ji"}}, {L"ツ", {L"tsu"}},
        {L"ヅ", {L"zu"}},  {L"テ", {L"te"}}, {L"デ", {L"de"}},
        {L"ト", {L"to"}},  {L"ド", {L"do"}}, {L"ナ", {L"na"}},
        {L"ニ", {L"ni"}},  {L"ヌ", {L"nu"}}, {L"ネ", {L"ne"}},
        {L"ノ", {L"no"}},  {L"ハ", {L"ha"}}, {L"バ", {L"ba"}},
        {L"パ", {L"pa"}},  {L"ヒ", {L"hi"}}, {L"ビ", {L"bi"}},
        {L"ピ", {L"pi"}},  {L"フ", {L"fu"}}, {L"ブ", {L"bu"}},
        {L"プ", {L"pu"}},  {L"ヘ", {L"he"}}, {L"ベ", {L"be"}},
        {L"ペ", {L"pe"}},  {L"ホ", {L"ho"}}, {L"ボ", {L"bo"}},
        {L"ポ", {L"po"}},  {L"マ", {L"ma"}}, {L"ミ", {L"mi"}},
        {L"ム", {L"mu"}},  {L"メ", {L"me"}}, {L"モ", {L"mo"}},
        {L"ヤ", {L"ya"}},  {L"ユ", {L"yu"}}, {L"ヨ", {L"yo"}},
        {L"ラ", {L"ra"}},  {L"リ", {L"ri"}}, {L"ル", {L"ru"}},
        {L"レ", {L"re"}},  {L"ロ", {L"ro"}}, {L"ワ", {L"wa"}},
        {L"ヲ", {L"wo"}},  {L"ン", {L"n"}},  {L"ヴ", {L"b/v"}},
    };
    const std::vector<Vocabulary> Vocabulary::KatakanaSingleCharacters = {
        {L"イェ", {L"ie"}},    {L"ウェ", {L"we"}},    {L"ウィ", {L"wi"}},
        {L"ウォ", {L"wo"}},    {L"ヴァ", {L"ba/va"}}, {L"ヴェ", {L"be/ve"}},
        {L"ヴィ", {L"bi/vi"}}, {L"ヴォ", {L"bo/vo"}}, {L"シェ", {L"she"}},
        {L"ジェ", {L"je"}},    {L"チェ", {L"che"}},   {L"ティ", {L"ti"}},
        {L"ディ", {L"di"}},    {L"トゥ", {L"tu"}},    {L"ドゥ", {L"du"}},
        {L"フェ", {L"fe"}},    {L"フィ", {L"fi"}},    {L"フォ", {L"fo"}},
        {L"ファ", {L"fa"}},
    };

} // namespace detail
