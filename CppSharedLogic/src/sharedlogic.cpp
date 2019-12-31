#include "sharedlogic.h"

#include <cassert>
#include <random>
#include <sqlite3.h>

#include "detail/util.hpp"

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
            std::function<void(unsigned vocIdx, bool accpet)> acceptCallback,
            const std::vector<detail::Vocabulary>& vocs,
            QuestionHandler::FlagEnum_TranslationType conversion) {

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

        return QuestionSetHelper::ConvertVocsToQuestion(
            [this](unsigned vocIdx, bool accept) {
                this->acceptAnswerCallback(vocIdx, accept);
            },
            vocs, conversion);
    }

    void QuestionHandler::acceptAnswerCallback(unsigned vocIdx, bool accept) {}

    QuestionHandler::QuestionHandler(
        std::shared_ptr<const VocabularyDeck> manager)
        : m_VocabularyMaanger(std::move(manager)) {}

    Question::Question(Question::KeyboardType kbt,
                       std::wstring_view questionVoc,
                       std::vector<std::wstring> answers,
                       std::function<void(bool)> answerCallback)
        : m_KbType(kbt), m_Question(questionVoc), m_Answers(std::move(answers)),
          m_AnswerCallback(std::move(answerCallback)) {}

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
        std::transform(
            voc.cbegin(), voc.cend(), tmpContainer.begin(),
            [](detail::VocabularyVector::const_iterator iter) {
                return detail::util::combineWStringContainerToWstring(
                    iter->english);
            });
        return detail::util::combineWStringContainerToWstring(tmpContainer);
    }

    VocabularyTranslator::VocabularyTranslator(
        const detail::VocabularyVector& manager)
        : m_VocabularyMaanger(manager) {}

    std::wstring
        VocabularyTranslator::translateKana(std::wstring_view kana) const {
        const auto voc = this->m_VocabularyMaanger.findAllKana(kana);
        std::vector<std::wstring> tmpContainer(voc.size());
        std::transform(voc.cbegin(), voc.cend(), tmpContainer.begin(),
                       [](detail::VocabularyVector::const_iterator iter) {
                           return iter->kana;
                       });
        return detail::util::combineWStringContainerToWstring(tmpContainer);
    }

    static detail::VocabularyVector
        readAnkiFromBasepathAndPrefix(const std::filesystem::path& basepath,
                                      const std::string& prefix) {
        const auto kanji_english_filepath =
            basepath / (prefix + LogicHandler::PostFix_Anki_KanjiEnglish);

        const auto kanji_hiragana_filepath =
            basepath / (prefix + LogicHandler::PostFix_Anki_KanjiHiragana);

        if (!std::filesystem::exists(kanji_english_filepath) ||
            !std::filesystem::exists(kanji_hiragana_filepath)) {
            return {};
        }
        const auto result = detail::VocParser::parseAnkiDataBase(
            kanji_english_filepath, kanji_hiragana_filepath);

        return result ? *result : detail::VocabularyVector();
    }

    static detail::VocabularyVector
        parseAnkiData(const std::filesystem::path& basepath) {
        detail::VocabularyVector result;
        for (const auto& e : LogicHandler::VocabularyType_Prefix) {
            auto anki = readAnkiFromBasepathAndPrefix(basepath, e.second);
            for (auto& voc : anki)
                voc.type = e.first;

            result.insert(result.end(), anki.cbegin(), anki.cend());
        }
        return result;
    }

    static detail::VocabularyVector
        parseJmdictData(const std::filesystem::path& basepath) {
        const auto parsed = detail::VocParser::parseJMDictFile(
            basepath / LogicHandler::FileName_Jmdict);

        return parsed ? *parsed : detail::VocabularyVector();
    }

    detail::VocabularyVector combineVocs(const detail::VocabularyVector& v0,
                                         const detail::VocabularyVector& v1) {
        detail::VocabularyVector result = v0;
        result.insert(result.end(), v1.cbegin(), v1.cend());
        return result;
    }

    LogicHandler::LogicHandler(const std::filesystem::path& databasesDirectory,
                               const std::filesystem::path& userFilePath)
        : m_Translator(this->m_AllVocabulary), m_UserFilePath(userFilePath),
          m_AnkiVocabulary(parseAnkiData(databasesDirectory)),
          m_JmdictVocabulary(parseJmdictData(databasesDirectory)),
          m_AllVocabulary(
              combineVocs(this->m_AnkiVocabulary, this->m_JmdictVocabulary)) {}

    QuestionHandler LogicHandler::createQuestionHandler() const {
        return QuestionHandler(this->m_CurrentDeck);
    }

    const VocabularyTranslator& LogicHandler::getVocabularyTranslator() const {
        return this->m_Translator;
    }

    VocabularyDeck LogicHandler::createVocabularyDeck() const {
        return VocabularyDeck(this->m_UserFilePath);
    }

    GenericTranslator LogicHandler::createGenericTranslator() const {
        return GenericTranslator();
    }

    void LogicHandler::loadDeck(std::wstring_view filename) {
        // always use new deck since the current one might be in use!
        this->m_CurrentDeck =
            std::make_shared<VocabularyDeck>(this->m_UserFilePath, filename);
    }

    std::vector<std::wstring> LogicHandler::listDecks() const {
        std::vector<std::wstring> decks;
        for (const auto& e :
             std::filesystem::directory_iterator(this->m_UserFilePath)) {
            auto name = e.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.rfind(LogicHandler::VocabularyDeck_Exstension) == 0)
                decks.push_back(e.path().wstring());
        }
        return decks;
    }

    std::shared_ptr<const VocabularyDeck> LogicHandler::getCurrentDeck() const {
        return this->m_CurrentDeck;
    }

    VocabularyDeck::VocabularyDeck(const std::filesystem::path& userFilePath,
                                   std::wstring_view filename)
        : m_UserFilePath(userFilePath) {
        load(filename);
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

        this->m_SortedFlashcards.insert(std::next(iter), fc);
        return true;
    }

    shared::VocabularyDeck::operator const detail::VocabularyVector&() const {
        return this->getAllVocabularies();
    }

    void VocabularyDeck::load(std::wstring_view filename) {
        if (filename.empty())
            return;
    }

    void VocabularyDeck::save() {
        const auto path = this->m_UserFilePath / this->m_DeckName;
        if (!std::filesystem::exists(path))
            return;

        detail::util::Sqlite3OpenCloseHelper db(path.string());
        if (!db)
            throw std::runtime_error("");
    }

    void VocabularyDeck::saveAs(std::wstring_view filename) {}

    void VocabularyDeck::addVocabularyUnique(detail::Vocabulary voc) {
        auto citer = std::find(this->m_Vocabulary.cbegin(),
                               this->m_Vocabulary.cend(), voc);

        if (citer != this->m_Vocabulary.cend())
            this->m_Vocabulary.push_back(std::move(voc));
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

    bool VocabularyDeck::Flashcard::operator<(
        const VocabularyDeck::Flashcard& rhs) const {
        return this->vocIter < rhs.vocIter;
    }

    bool VocabularyDeck::Flashcard::operator==(
        const VocabularyDeck::Flashcard& rhs) const {
        return this->vocIter == rhs.vocIter;
    }

} // namespace shared
