#include "sharedlogic.h"

#include <iostream>

#include <array>
#include <cassert>
#include <random>
#include <sqlite3.h>
#include <sstream>

#include "detail/util.hpp"

namespace parameter {

    static constexpr const std::wstring_view VocabularyDeck_Exstension = L".vd";

    static constexpr const auto FileName_Jmdict = "JMdict_e";

    static constexpr const auto PostFix_Anki_KanjiEnglish =
        "-vocab-kanji-eng.anki";
    static constexpr const auto PostFix_Anki_KanjiHiragana =
        "-vocab-kanji-hiragana.anki";

    static constexpr std::array VocabularyType_Prefix = {
        std::make_pair(detail::Vocabulary::Type::N1, "n1"),
        std::make_pair(detail::Vocabulary::Type::N2, "n2"),
        std::make_pair(detail::Vocabulary::Type::N3, "n3"),
        std::make_pair(detail::Vocabulary::Type::N4, "n4"),
        std::make_pair(detail::Vocabulary::Type::N5, "n5")};
} // namespace parameter

namespace shared {

    class QuestionSetHelper {
        static decltype(auto) CharactersHelper(
            const decltype(
                detail::Vocabulary::HiraganaSingleCharacters)& single,
            const decltype(
                detail::Vocabulary::HiraganaMultiCharacters)& multi) {
            using Voc = detail::Vocabulary;
            std::vector<Voc> result;
            result.insert(result.end(), multi.cbegin(), multi.cend());
            result.insert(result.end(), single.cbegin(), single.cend());
            return result;
        }

        static Question ConvertVocabularyToQuestion_EnglishToKana(
            std::function<void(bool)> acceptCallback,
            const detail::Vocabulary& voc) {
            auto rng_iter = detail::util::getRandomIterator(
                voc.english.cbegin(), voc.english.cend());

            return Question(Question::KeyboardType::English, *rng_iter,
                            {voc.kana}, std::move(acceptCallback));
        }
        static Question ConvertVocabularyToQuestion_KanaToEnglish(
            std::function<void(bool)> acceptCallback,
            const detail::Vocabulary& voc) {
            auto rng_iter = detail::util::getRandomIterator(
                voc.english.cbegin(), voc.english.cend());

            return Question(Question::KeyboardType::English, *rng_iter,
                            {voc.kana}, std::move(acceptCallback));
        }
        static Question ConvertVocabularyToQuestion_Mixed(
            std::function<void(bool)> acceptCallback,
            const detail::Vocabulary& voc) {
            if ((detail::util::GetRandomGenerator()()) & 1) {
                return ConvertVocabularyToQuestion_EnglishToKana(
                    std::move(acceptCallback), voc);
            }
            return ConvertVocabularyToQuestion_KanaToEnglish(
                std::move(acceptCallback), voc);
        }

      public:
        static decltype(auto) CharactersHiragana() {
            using Voc = detail::Vocabulary;
            return CharactersHelper(Voc::HiraganaSingleCharacters,
                                    Voc::HiraganaMultiCharacters);
        }
        static decltype(auto) CharactersKatakana() {
            using Voc = detail::Vocabulary;
            return CharactersHelper(Voc::KatakanaSingleCharacters,
                                    Voc::KatakanaMultiCharacters);
        }

        static std::vector<detail::Vocabulary> NumbersInteger() {
            /// TODO
            return {};
        }
        static std::vector<detail::Vocabulary> NumbersFloatingPoint() {
            /// TODO
            return {};
        }

        static std::vector<detail::Vocabulary> WeekDay() {
            /// TODO
            return {};
        }
        static std::vector<detail::Vocabulary> Month() {
            /// TODO
            return {};
        }

        static std::vector<detail::Vocabulary>
            Vocabulary(const detail::VocabularyVector& vm) {
            /// TODO
            return {};
        }

        static decltype(auto)
            GetVocs(QuestionHandler::FlagEnum_QuestionType type,
                    const detail::VocabularyVector& vm) {
            std::vector<detail::Vocabulary> vocs;
            auto append =
                [&vocs](const std::vector<detail::Vocabulary>& toInsert) {
                    vocs.insert(vocs.end(), toInsert.cbegin(), toInsert.cend());
                };
            using FT = QuestionHandler::FlagEnum_QuestionType;
            if (type | FT::Characters_Hiragana)
                append(CharactersHiragana());
            if (type | FT::Characters_Katakana)
                append(CharactersKatakana());
            if (type | FT::Numbers_Integers)
                append(NumbersInteger());
            if (type | FT::Numbers_FloatingPoint)
                append(NumbersFloatingPoint());
            if (type | FT::Vocabulary)
                append(Vocabulary(vm));
            if (type | FT::WeekDay)
                append(WeekDay());
            if (type | FT::Month)
                append(Month());
            return vocs;
        }

        static decltype(auto) GetConvertVocsToQuestionFunction(
            QuestionHandler::FlagEnum_TranslationType conversion) {
            using Conv = QuestionHandler::FlagEnum_TranslationType;
            auto convFunc = ConvertVocabularyToQuestion_Mixed;
            if (conversion == Conv::EnglishToKana)
                convFunc = ConvertVocabularyToQuestion_EnglishToKana;
            else if (conversion == Conv::KanaToEnglish)
                convFunc = ConvertVocabularyToQuestion_KanaToEnglish;

            return convFunc;
        }

        static decltype(auto) ConvertVocsToQuestion(
            const std::vector<detail::Vocabulary>& vocs,
            QuestionHandler::FlagEnum_TranslationType conversion,
            std::function<void(unsigned vocIdx, bool accpet)> acceptCallback) {

            const auto convFunc = GetConvertVocsToQuestionFunction(conversion);
            std::vector<Question> result;
            result.reserve(vocs.size());

            // maybe find a way to use std::transform or similar...
            for (unsigned vocIdx = 0; vocIdx < vocs.size(); ++vocIdx) {
                auto callbackFunc = [vocIdx, acceptCallback](bool accept) {
                    acceptCallback(vocIdx, accept);
                };
                result.push_back(convFunc(callbackFunc, vocs[vocIdx]));
            }
            return result;
        }
    };

    std::vector<Question> QuestionHandler::getQuestionSet(
        QuestionHandler::FlagEnum_QuestionType type,
        QuestionHandler::FlagEnum_TranslationType conversion) {

        const auto vocs =
            QuestionSetHelper::GetVocs(type, *this->m_VocabularyMaanger);

        using FcIndexType = VocabularyDeck::Flashcard::index_type;
        return QuestionSetHelper::ConvertVocsToQuestion(
            vocs, conversion, [this](FcIndexType vocIdx, bool accept) {
                this->acceptAnswerCallback(vocIdx, accept);
            });
    }

    void QuestionHandler::acceptAnswerCallback(
        VocabularyDeck::Flashcard::index_type vocIdx, bool accept) {
        const auto fc = this->m_VocabularyMaanger->getFlashcard(vocIdx);
        assert(fc);

        using Fc = VocabularyDeck::Flashcard;
        auto update = [this, iter = fc->vocIter](Fc::index_type newIdx) {
            this->m_VocabularyMaanger->setFlashcardIndex(iter, newIdx);
        };
        if (accept && fc->cardIndex > Fc::MIN_CARD_INDEX)
            update(fc->cardIndex - 1);
        else if (!accept && fc->cardIndex < Fc::MAX_CARD_INDEX)
            update(fc->cardIndex + 1);
    }

    QuestionHandler::QuestionHandler(std::shared_ptr<VocabularyDeck> manager)
        : m_VocabularyMaanger(std::move(manager)) {}

    Question::Question(Question::KeyboardType kbt,
                       std::wstring_view questionVoc,
                       std::vector<std::wstring> answers,
                       std::function<void(bool)> answerCallback)
        : m_KbType(kbt), m_Question(questionVoc), m_Answers(std::move(answers)),
          m_AnswerCallback(std::move(answerCallback)) {}

    Question::KeyboardType Question::getKeyboardType() const {
        return this->m_KbType;
    }

    const std::vector<std::wstring>& Question::getAnswers() const {
        return this->m_Answers;
    }

    const std::wstring& Question::getQuestionVocabulary() const {
        return this->m_Question;
    }

    bool Question::checkAnswer(std::wstring_view answer) const {
        /// TODO: maybe add more complex checking
        return std::find(this->m_Answers.cbegin(), this->m_Answers.cend(),
                         answer) != this->m_Answers.cend();
    }

    void Question::acceptAnswer(bool correct) const {
        if (this->m_AnswerCallback)
            this->m_AnswerCallback(correct);
    }

    bool Question::isEnglishToKanaQuesiton() const {
        return this->m_KbType == KeyboardType::Hiragana;
    }

    bool Question::isKanaToEnglishQuestion() const {
        return this->m_KbType == KeyboardType::English;
    }

    std::wstring GenericTranslator::translateTime(Time time) {
        throw std::runtime_error("Not implemented yet!");
    }
    std::wstring GenericTranslator::translateDate(Date date) {
        throw std::runtime_error("Not implemented yet!");
    }
    std::wstring GenericTranslator::translateDateTime(time_t dateTime) {
        throw std::runtime_error("Not implemented yet!");
    }
    std::wstring GenericTranslator::translateDateTime(Date date, Time time) {
        throw std::runtime_error("Not implemented yet!");
    }

    std::wstring GenericTranslator::translateNumber(double value) {
        throw std::runtime_error("Not implemented yet!");
    }

    std::wstring GenericTranslator::translateWeekDay(WeekDay day) {
        throw std::runtime_error("Not implemented yet!");
    }

    std::wstring VocabularyTranslator::translateEnglish(
        std::wstring_view english) const {
        const auto voc = this->m_VocabularyMaanger.findAllEnglish(english);
        std::vector<std::wstring> tmpContainer(voc.size());
        std::transform(voc.cbegin(), voc.cend(), tmpContainer.begin(),
                       [](detail::VocabularyVector::const_iterator iter) {
                           return iter->kana;
                       });
        return detail::util::combineWStringContainerToWstring(tmpContainer,
                                                              L"\n");
    }

    VocabularyTranslator::VocabularyTranslator(
        const detail::VocabularyVector& manager)
        : m_VocabularyMaanger(manager) {}

    std::wstring
        VocabularyTranslator::translateKana(std::wstring_view kana) const {
        const auto voc = this->m_VocabularyMaanger.findAllKana(kana);
        std::vector<std::wstring> tmpContainer(voc.size());
        std::transform(
            voc.cbegin(), voc.cend(), tmpContainer.begin(),
            [](detail::VocabularyVector::const_iterator iter) {
                return detail::util::combineWStringContainerToWstring(
                    iter->english);
            });
        return detail::util::combineWStringContainerToWstring(tmpContainer,
                                                              L"\n");
    }

    static detail::VocabularyVector
        readAnkiFromBasepathAndPrefix(const boost::filesystem::path& basepath,
                                      const std::string& prefix) {
        const auto kanji_english_filepath =
            basepath / (prefix + parameter::PostFix_Anki_KanjiEnglish);

        const auto kanji_hiragana_filepath =
            basepath / (prefix + parameter::PostFix_Anki_KanjiHiragana);

        if (!boost::filesystem::exists(kanji_english_filepath) ||
            !boost::filesystem::exists(kanji_hiragana_filepath)) {
            return {};
        }
        const auto result = detail::VocParser::parseAnkiDataBase(
            kanji_english_filepath, kanji_hiragana_filepath);

        return result ? *result : detail::VocabularyVector();
    }

    static detail::VocabularyVector
        parseAnkiData(const boost::filesystem::path& basepath) {
        detail::VocabularyVector result;
        for (const auto& e : parameter::VocabularyType_Prefix) {
            auto anki = readAnkiFromBasepathAndPrefix(basepath, e.second);
            for (auto& voc : anki)
                voc.type = e.first;

            result.insert(result.end(), anki.cbegin(), anki.cend());
        }
        return result;
    }

    static detail::VocabularyVector
        parseJmdictData(const boost::filesystem::path& basepath, bool enable) {
        if (!enable)
            return {};

        const auto parsed = detail::VocParser::parseJMDictFile(
            basepath / parameter::FileName_Jmdict);

        return parsed ? *parsed : detail::VocabularyVector();
    }

    detail::VocabularyVector combineVocs(const detail::VocabularyVector& v0,
                                         const detail::VocabularyVector& v1) {
        detail::VocabularyVector result = v0;
        result.insert(result.end(), v1.cbegin(), v1.cend());
        return result;
    }

    LogicHandler::LogicHandler(
        const boost::filesystem::path& databasesDirectory,
        const boost::filesystem::path& userFilePath, bool enableJmdict)
        : m_Translator(this->m_AllVocabulary), m_UserFilePath(userFilePath),
          m_AnkiVocabulary(parseAnkiData(databasesDirectory)),
          m_JmdictVocabulary(parseJmdictData(databasesDirectory, enableJmdict)),
          m_AllVocabulary(
              combineVocs(this->m_AnkiVocabulary, this->m_JmdictVocabulary)) {
        this->m_CurrentDeck =
            std::make_shared<VocabularyDeck>(this->m_UserFilePath);
    }

    QuestionHandler LogicHandler::createQuestionHandler() const {
        return QuestionHandler(this->m_CurrentDeck);
    }

    const VocabularyTranslator& LogicHandler::getVocabularyTranslator() const {
        return this->m_Translator;
    }

    const detail::VocabularyVector& LogicHandler::getAllVocabulary() const {
        return this->m_AllVocabulary;
    }

    VocabularyDeck LogicHandler::createVocabularyDeck() const {
        return VocabularyDeck(this->m_UserFilePath);
    }

    GenericTranslator LogicHandler::createGenericTranslator() const {
        return GenericTranslator();
    }

    void LogicHandler::loadDeck(const std::wstring& filename) {
        // always use new deck since the current one might be in use!
        this->m_CurrentDeck =
            std::make_shared<VocabularyDeck>(this->m_UserFilePath, filename);
    }

    static bool findExtensionAtEnd(std::wstring str, std::wstring_view ext) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        const auto pos = str.rfind(ext);
        if (pos == std::wstring::npos)
            return false;

        return str.size() - pos - ext.size() == 0;
    }

    std::vector<std::wstring> LogicHandler::listDecks() const {
        const auto ext = parameter::VocabularyDeck_Exstension;
        std::vector<std::wstring> decks;
        for (const auto& e :
             boost::filesystem::directory_iterator(this->m_UserFilePath)) {
            auto name = e.path().filename().wstring();
            if (findExtensionAtEnd(name, ext))
                decks.push_back(std::move(name));
        }
        return decks;
    }

    bool LogicHandler::removeDeck(const std::wstring& filename) const {
        const auto path = this->m_UserFilePath / filename;
        if (boost::filesystem::is_regular_file(path)) {
            boost::filesystem::remove(path);
            return true;
        }
        return false;
    }

    std::shared_ptr<const VocabularyDeck> LogicHandler::getCurrentDeck() const {
        return this->m_CurrentDeck;
    }

    VocabularyDeck::VocabularyDeck(const boost::filesystem::path& userFilePath,
                                   const std::wstring& filename)
        : m_UserFilePath(userFilePath) {
        load(filename);
    }

    const std::string& VocabularyDeck::getDeckname() const {
        return this->m_DeckName;
    }

    template <typename FlashcardContainer>
    decltype(auto) Helper_findFlashcardsByIter(
        FlashcardContainer& sortedContainer,
        detail::VocabularyVector::const_iterator citer) {
        using Fc = VocabularyDeck::Flashcard;
        return std::equal_range(sortedContainer.begin(), sortedContainer.end(),
                                Fc{0, citer});
    }

    const detail::VocabularyVector& VocabularyDeck::getAllVocabularies() const {
        return this->m_Vocabulary;
    }

    std::optional<VocabularyDeck::Flashcard>
        VocabularyDeck::getFlashcard(unsigned vocIdx) const {
        if (vocIdx < this->m_Vocabulary.size())
            return std::nullopt;

        return this->getFlashcard(this->m_Vocabulary.cbegin() + vocIdx);
    }

    std::optional<VocabularyDeck::Flashcard>
        VocabularyDeck::getFlashcard(const detail::Vocabulary& voc) const {
        const auto citer = std::find(this->m_Vocabulary.cbegin(),
                                     this->m_Vocabulary.cend(), voc);
        return this->getFlashcard(citer);
    }

    std::optional<VocabularyDeck::Flashcard> VocabularyDeck::getFlashcard(
        detail::VocabularyVector::const_iterator citer) const {
        const auto range =
            Helper_findFlashcardsByIter(this->m_SortedFlashcards, citer);
        return range.first != this->m_SortedFlashcards.end()
                   ? std::optional{*range.first}
                   : std::nullopt;
    }

    bool VocabularyDeck::setFlashcardIndex(unsigned vocIdx,
                                           unsigned newCardIdx) {
        if (vocIdx < this->m_Vocabulary.size())
            return false;

        return this->setFlashcardIndex(this->m_Vocabulary.cbegin() + vocIdx,
                                       newCardIdx);
    }

    bool VocabularyDeck::setFlashcardIndex(const detail::Vocabulary& voc,
                                           unsigned newCardIdx) {
        const auto citer = std::find(this->m_Vocabulary.cbegin(),
                                     this->m_Vocabulary.cend(), voc);

        return this->setFlashcardIndex(citer, newCardIdx);
    }

    bool VocabularyDeck::setFlashcardIndex(
        detail::VocabularyVector::const_iterator citer, unsigned newCardIdx) {
        auto range =
            Helper_findFlashcardsByIter(this->m_SortedFlashcards, citer);

        if (range.first == this->m_SortedFlashcards.end())
            return false;

        range.first->cardIndex = newCardIdx;
        return true;
    }

    bool VocabularyDeck::addFlashcard(const VocabularyDeck::Flashcard& fc) {
        auto& sfc = this->m_SortedFlashcards;
        auto iter = std::upper_bound(sfc.begin(), sfc.end(), fc);
        if (iter != sfc.end() && iter->vocIter == fc.vocIter)
            return false;

        iter = (iter == sfc.end()) ? sfc.begin() : std::next(iter);
        this->m_SortedFlashcards.insert(iter, fc);
        return true;
    }

    shared::VocabularyDeck::operator const detail::VocabularyVector&() const {
        return this->getAllVocabularies();
    }

    bool VocabularyDeck::load(const std::wstring& filename) {
        const auto path = this->m_UserFilePath / filename;
        if (filename.empty() || !boost::filesystem::exists(path))
            return false;

        return this->_loadFromFile(path);
    }

    bool VocabularyDeck::save() {
        const auto path = this->m_UserFilePath / this->m_DeckName;
        if (!boost::filesystem::is_regular_file(path))
            return false;

        return this->_saveToFile(path);
    }

    bool VocabularyDeck::saveAs(std::wstring filename) {
        const auto& ext = parameter::VocabularyDeck_Exstension;
        if (const auto pos = filename.rfind('.'); pos != std::string::npos)
            filename.replace(pos, std::string::npos, ext);
        else
            filename += ext;

        const auto path = this->m_UserFilePath / filename;
        if (filename.empty() || boost::filesystem::exists(path))
            return false;

        return this->_saveToFile(path);
    }

    void VocabularyDeck::addVocabularyUnique(const detail::Vocabulary& voc) {
        const auto citer = std::find(this->m_Vocabulary.cbegin(),
                                     this->m_Vocabulary.cend(), voc);

        if (citer == this->m_Vocabulary.cend())
            this->m_Vocabulary.push_back(voc);
    }

    void VocabularyDeck::clear() { this->m_Vocabulary.clear(); }

    bool VocabularyDeck::removeVocabulary(const detail::Vocabulary& voc) {
        auto iter = std::find(this->m_Vocabulary.begin(),
                              this->m_Vocabulary.end(), voc);

        if (iter == this->m_Vocabulary.end())
            return false;

        this->m_Vocabulary.erase(iter);
        return true;
    }

    struct VocabularyDeck_SaveLoad_Helper {

        static constexpr const wchar_t VocSplitWChar = L';';

        static std::vector<std::wstring>
            splitVocDeckString(const std::wstring& wstr) {
            std::wstringstream wsstr(wstr);
            std::vector<std::wstring> result;
            for (std::wstring str; std::getline(wsstr, str, VocSplitWChar);)
                result.push_back(str);

            return result;
        }

        struct VocabularyTable {
            static constexpr const std::string_view TableName = "Vocabulary";

            static constexpr const std::string_view English = "English";
            static constexpr const std::string_view Kana = "Kana";
            static constexpr const std::string_view Kanji = "Kanji";
            static constexpr const std::string_view Type = "Type";

            static void createTable(sqlite3* db) {
                const auto createVocabularyTable =
                    "create table if not exists " +
                    std::string(VocabularyTable::TableName) + " (" +
                    std::string(VocabularyTable::English) + " text," +
                    std::string(VocabularyTable::Kana) + " text," +
                    std::string(VocabularyTable::Kanji) + " text," +
                    std::string(VocabularyTable::Type) + " int32)";

                if (sqlite3_exec(db, createVocabularyTable.c_str(), nullptr,
                                 nullptr, nullptr) != SQLITE_OK) {
                    throw std::runtime_error("");
                }
            }

            static detail::VocabularyVector readTable(sqlite3* db) {
                const auto stm =
                    "select * from " + std::string(VocabularyTable::TableName);
                detail::VocabularyVector result;
                if (sqlite3_exec(db, stm.c_str(), callback, &result, nullptr) !=
                    SQLITE_OK) {
                    throw std::runtime_error("");
                }
                return result;
            }

            static bool writeTable(sqlite3* db,
                                   const detail::VocabularyVector& vocs) {
                const std::string sql =
                    "insert into " + std::string(VocabularyTable::TableName) +
                    " (" + std::string(VocabularyTable::English) + ',' +
                    std::string(VocabularyTable::Kana) + ',' +
                    std::string(VocabularyTable::Kanji) + ',' +
                    std::string(VocabularyTable::Type) + ") values ";

                bool result = true;
                for (const auto& e : vocs) {
                    const auto stm = sql + vocabularyToString(e) + ';';
                    if (sqlite3_exec(db, stm.c_str(), nullptr, nullptr,
                                     nullptr) != SQLITE_OK) {
                        result = false;
                    }
                }
                return result;
            }

          private:
            static std::string
                vocabularyToString(const detail::Vocabulary& voc) {
                const auto convFunc = detail::convertWstringUtf8;
                return R"((")" +
                       convFunc(detail::util::combineWStringContainerToWstring(
                           voc.english)) +
                       R"(",")" + convFunc(voc.kana) + R"(",")" +
                       convFunc(voc.kanji) + R"(",")" +
                       std::to_string(int(voc.type)) + R"("))";
            }

            static int callback(void* vocVec, int argc, char** argv, char**) {
                assert(argc == 4);
                if (argc != 4)
                    return -1;

                auto data = static_cast<detail::VocabularyVector*>(vocVec);

                const auto english = detail::convertUtf8Wstring(argv[0]);
                const auto kana = detail::convertUtf8Wstring(argv[1]);
                const auto kanji = detail::convertUtf8Wstring(argv[2]);
                const auto type = detail::Vocabulary::Type(std::stoi(argv[3]));

                data->emplace_back(kana, splitVocDeckString(english), type,
                                   kanji);
                return 0;
            };
        };

        struct FlashcardTable {
            static constexpr const std::string_view TableName = "Flashcards";

            static constexpr const std::string_view Kana = "Kana";
            static constexpr const std::string_view FlashcardIndex =
                "FlashcardIndex";

            static void createTable(sqlite3* db) {
                const auto createVocabularyTable =
                    "create table if not exists " +
                    std::string(FlashcardTable::TableName) + " (" +
                    std::string(FlashcardTable::Kana) + " text," +
                    std::string(FlashcardTable::FlashcardIndex) + " int32)";

                if (sqlite3_exec(db, createVocabularyTable.c_str(), nullptr,
                                 nullptr, nullptr) != SQLITE_OK) {
                    throw std::runtime_error("");
                }
            }

            static std::vector<VocabularyDeck::Flashcard>
                readTable(sqlite3* db, const detail::VocabularyVector& vocs) {
                const auto stm =
                    "select * from " + std::string(FlashcardTable::TableName);
                std::vector<TableStruct> tableStructs;
                if (sqlite3_exec(db, stm.c_str(), callback, &tableStructs,
                                 nullptr)) {
                    throw std::runtime_error("");
                }
                return combineStructVocs(vocs, tableStructs);
            }

            static bool
                writeTable(sqlite3* db,
                           const std::vector<VocabularyDeck::Flashcard>& vocs) {
                if (vocs.empty())
                    return true;

                const std::string sql =
                    "insert into " + std::string(FlashcardTable::TableName) +
                    '(' + std::string(FlashcardTable::Kana) + ',' +
                    std::string(FlashcardTable::FlashcardIndex) + ") values ";

                bool result = true;
                for (const auto& e : vocs) {
                    const auto stm = sql + flashcardToString(e) + ';';
                    if (sqlite3_exec(db, stm.c_str(), nullptr, nullptr,
                                     nullptr) != SQLITE_OK) {
                        result = false;
                    }
                }
                return result;
            }

          private:
            struct TableStruct {
                std::wstring kana;
                VocabularyDeck::Flashcard::index_type flashcardIndex = 0;
            };

            static std::vector<VocabularyDeck::Flashcard> combineStructVocs(
                const detail::VocabularyVector& vocs,
                const std::vector<TableStruct>& tableStructs) {

                std::vector<VocabularyDeck::Flashcard> result;
                for (const auto& e : tableStructs) {
                    auto& card = result.emplace_back();
                    card.cardIndex = e.flashcardIndex;
                    card.vocIter =
                        std::find_if(vocs.cbegin(), vocs.cend(),
                                     [&e](const detail::Vocabulary& voc) {
                                         return voc.kana == e.kana;
                                     });
                    assert(card.vocIter != vocs.cend());
                    if (card.vocIter == vocs.cend())
                        result.pop_back();
                }
                return result;
            }

            static std::string
                flashcardToString(const VocabularyDeck::Flashcard& card) {
                return '(' + detail::convertWstringUtf8(card.vocIter->kana) +
                       ',' + std::to_string(card.cardIndex) + ')';
            }

            static int callback(void* vec, int argc, char** argv, char**) {
                assert(argc == 2);
                if (argc != 2)
                    return -1;

                auto data = static_cast<std::vector<TableStruct>*>(vec);
                auto& card = data->emplace_back();
                card.kana = detail::convertUtf8Wstring(argv[0]);
                card.flashcardIndex = std::stoi(argv[1]);
                return 0;
            };
        };
        static detail::VocabularyVector readVocabulary(sqlite3* db) {
            assert(db);
            VocabularyTable::createTable(db);
            return VocabularyTable::readTable(db);
        }

        static std::vector<VocabularyDeck::Flashcard>
            readFlashcards(const detail::VocabularyVector& vocs, sqlite3* db) {
            assert(db);
            FlashcardTable::createTable(db);
            return FlashcardTable::readTable(db, vocs);
        }

        static bool writeVocabulary(sqlite3* db,
                                    const detail::VocabularyVector& vocs) {
            assert(db);
            VocabularyTable::createTable(db);
            return VocabularyTable::writeTable(db, vocs);
        }

        static bool writeFlashcards(
            sqlite3* db,
            const std::vector<VocabularyDeck::Flashcard>& flashcards) {
            assert(db);
            FlashcardTable::createTable(db);
            return FlashcardTable::writeTable(db, flashcards);
        }
    };

    bool
        VocabularyDeck::_saveToFile(const boost::filesystem::path& path) const {
        detail::util::Sqlite3OpenCloseHelper db(path.string());
        if (!db)
            return false;

        using Vdsh = VocabularyDeck_SaveLoad_Helper;
        if (!Vdsh::writeVocabulary(db, this->m_Vocabulary) ||
            !Vdsh::writeFlashcards(db, this->m_SortedFlashcards)) {
            return false;
        }
        return true;
    }

    bool VocabularyDeck::_loadFromFile(const boost::filesystem::path& path) {
        detail::util::Sqlite3OpenCloseHelper db(path.string());
        if (!db)
            return false;

        using Vdsh = VocabularyDeck_SaveLoad_Helper;
        this->m_DeckName = path.filename().string();
        this->m_Vocabulary = Vdsh::readVocabulary(db);
        this->m_SortedFlashcards = Vdsh::readFlashcards(this->m_Vocabulary, db);
        std::sort(this->m_SortedFlashcards.begin(),
                  this->m_SortedFlashcards.end());
        return true;
    }

    bool VocabularyDeck::Flashcard::operator<(
        const VocabularyDeck::Flashcard& rhs) const {
        return this->vocIter < rhs.vocIter;
    }

    bool VocabularyDeck::Flashcard::operator==(
        const VocabularyDeck::Flashcard& rhs) const {
        return this->vocIter == rhs.vocIter;
    }

} // namespace shared
