#pragma once

#include <cinttypes>
#include <optional>

#include <boost/filesystem.hpp>

#include "detail/vocabparse.h"

namespace shared {

    struct LogicHandler;

    struct VocabularyDeck {
        struct Flashcard {
            using index_type = unsigned;
            static constexpr const unsigned MIN_CARD_INDEX =
                std::numeric_limits<index_type>::min();
            static constexpr const unsigned MAX_CARD_INDEX =
                std::numeric_limits<index_type>::max();

            index_type cardIndex = Flashcard::MAX_CARD_INDEX;
            detail::VocabularyVector::const_iterator vocIter;

            bool operator<(const Flashcard& rhs) const;
            bool operator==(const Flashcard& rhs) const;
        };
        VocabularyDeck(const std::string &userFilePath, const std::wstring &filename = L"");

        const std::string& getDeckname() const;
        operator const detail::VocabularyVector&() const;
        const detail::VocabularyVector& getAllVocabularies() const;

        std::optional<Flashcard> getFlashcard(unsigned vocIdx) const;
        std::optional<Flashcard>
            getFlashcard(const detail::Vocabulary& voc) const;
        std::optional<Flashcard>
            getFlashcard(detail::VocabularyVector::const_iterator citer) const;

        bool setFlashcardIndex(unsigned vocIdx, unsigned newCardIdx);
        bool setFlashcardIndex(const detail::Vocabulary& voc,
                               unsigned newCardIdx);
        bool setFlashcardIndex(detail::VocabularyVector::const_iterator citer,
                               unsigned newCardIdx);

        bool addFlashcard(const Flashcard& fc);

        bool load(const std::wstring& filename);

        bool save();
        bool saveAs(std::wstring filename);

        void addVocabularyUnique(const detail::Vocabulary& voc);

        void clear();
        bool removeVocabulary(const detail::Vocabulary &voc);

    private:
        bool _saveToFile(const std::string &path) const;
        bool _loadFromFile(const std::string &path);

        std::string m_DeckName;
        detail::VocabularyVector m_Vocabulary;
        std::vector<Flashcard> m_SortedFlashcards;
        const boost::filesystem::path m_UserFilePath;
    };

    struct Question {
        enum class KeyboardType { English, Hiragana };

        Question(KeyboardType kbt, std::wstring_view questionVoc,
                 std::vector<std::wstring> answers,
                 std::function<void(bool)> answerCallback = nullptr);

        KeyboardType getKeyboardType() const;
        const std::wstring& getQuestionVocabulary() const;
        const std::vector<std::wstring>& getAnswers() const;

        void acceptAnswer(bool correct) const;
        bool checkAnswer(std::wstring_view answer) const;

        bool isEnglishToKanaQuesiton() const;
        bool isKanaToEnglishQuestion() const;

      private:
        const KeyboardType m_KbType;
        const std::wstring m_Question;
        const std::vector<std::wstring> m_Answers;
        const std::function<void(bool)> m_AnswerCallback;
    };

    struct QuestionHandler {
        enum FlagEnum_QuestionType {
            Characters_Hiragana = 1 << 0,
            Characters_Katakana = 1 << 1,
            Characters = Characters_Hiragana | Characters_Katakana,
            Numbers_FloatingPoint = 1 << 2,
            Numbers_Integers = 1 << 3,
            Numbers = Numbers_Integers | Numbers_FloatingPoint,
            Vocabulary = 1 << 4,
            WeekDay = 1 << 5,
            Month = 1 << 6
        };
        enum class FlagEnum_TranslationType {
            EnglishToKana = 1 << 0,
            KanaToEnglish = 1 << 1,
            Mixed = EnglishToKana | KanaToEnglish
        };

        QuestionHandler(std::shared_ptr<VocabularyDeck> manager);

        /// TODO: add feedback -> algo for 'correct/incorrect answers'
        std::vector<Question>
            getQuestionSet(FlagEnum_QuestionType type,
                           FlagEnum_TranslationType conversion =
                               FlagEnum_TranslationType::Mixed);

      private:
        void acceptAnswerCallback(VocabularyDeck::Flashcard::index_type vocIdx,
                                  bool accept);

        std::shared_ptr<VocabularyDeck> m_VocabularyMaanger;
    };

    struct GenericTranslator {
        enum class WeekDay {
            Monday,
            Tuesday,
            Wednesday,
            Thurstday,
            Friday,
            Saturday,
            Sunday
        };
        struct Time {
            uint8_t hours;   // 1-23
            uint8_t minutes; // 0-60
            uint8_t seconds; // 0-60
        };
        struct Date {
            uint16_t day;  // 1-366
            uint8_t month; // 1-12
            uint8_t year;  // 1900-2200
        };

        std::wstring translateTime(Time time);
        std::wstring translateDate(Date date);
        std::wstring translateDateTime(time_t dateTime);
        std::wstring translateDateTime(Date date, Time time);

        std::wstring translateNumber(double value);

        std::wstring translateWeekDay(WeekDay day);

      protected:
        friend LogicHandler;
        GenericTranslator() = default;
    };

    struct VocabularyTranslator {
        std::wstring translateKana(const std::wstring &kana) const;
        std::wstring translateEnglish(const std::wstring &english) const;

    protected:
        friend LogicHandler;
        VocabularyTranslator(const detail::VocabularyVector& manager);

      private:
        const detail::VocabularyVector& m_VocabularyMaanger;
    };

    struct LogicHandler {
        // jmdict is not recommended due to to many vocabulary and other reasons
        LogicHandler(const std::string &databasesDirectory,
                     const std::string &userFilePath,
                     bool enableJmdict = false);

        const detail::VocabularyVector& getAllVocabulary() const;
        const VocabularyTranslator& getVocabularyTranslator() const;

        VocabularyDeck createVocabularyDeck() const;
        QuestionHandler createQuestionHandler() const;
        GenericTranslator createGenericTranslator() const;

        void loadDeck(const std::wstring& filename);
        std::vector<std::wstring> listDecks() const;
        bool removeDeck(const std::wstring& filename) const;
        std::shared_ptr<const VocabularyDeck> getCurrentDeck() const;

      private:
        std::shared_ptr<VocabularyDeck> m_CurrentDeck;

        const VocabularyTranslator m_Translator;
        const boost::filesystem::path m_UserFilePath;
        const detail::VocabularyVector m_AnkiVocabulary;
        const detail::VocabularyVector m_JmdictVocabulary;

        // has to be the last VocVector due to initializatin order
        const detail::VocabularyVector m_AllVocabulary;
    };

} // namespace shared
