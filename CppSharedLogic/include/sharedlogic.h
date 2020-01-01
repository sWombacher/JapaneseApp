#pragma once

#include <cinttypes>
#include <detail/vocabparse.h>
#include <filesystem>
#include <optional>

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
        VocabularyDeck(const std::filesystem::path& userFilePath,
                       std::wstring_view filename = L"");

        VocabularyDeck(VocabularyDeck&) = delete;
        void operator=(VocabularyDeck&) = delete;

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

        bool load(std::wstring_view filename);

        bool save();
        bool saveAs(std::wstring filename);

        void addVocabularyUnique(const detail::Vocabulary& voc);

        void clear();
        bool removeVocabulary(const detail::Vocabulary& voc);

      private:
        bool _saveToFile(const std::filesystem::path& path) const;
        bool _loadFromFile(const std::filesystem::path& path);

        std::string m_DeckName;
        detail::VocabularyVector m_Vocabulary;
        std::vector<Flashcard> m_SortedFlashcards;
        const std::filesystem::path m_UserFilePath;
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
        void operator=(QuestionHandler&) = delete;
        QuestionHandler(QuestionHandler&) = delete;

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
        std::wstring translateKana(std::wstring_view kana) const;
        std::wstring translateEnglish(std::wstring_view english) const;

      protected:
        friend LogicHandler;
        VocabularyTranslator(const detail::VocabularyVector& manager);

      private:
        const detail::VocabularyVector& m_VocabularyMaanger;
    };

    struct LogicHandler {
        // jmdict is not recommended due to to many vocabulary and other reasons
        LogicHandler(const std::filesystem::path& databasesDirectory,
                     const std::filesystem::path& userFilePath,
                     bool enableJmdict = false);

        const detail::VocabularyVector& getAllVocabulary() const;
        const VocabularyTranslator& getVocabularyTranslator() const;

        VocabularyDeck createVocabularyDeck() const;
        QuestionHandler createQuestionHandler() const;
        GenericTranslator createGenericTranslator() const;

        void loadDeck(std::wstring_view filename);
        std::vector<std::wstring> listDecks() const;
        bool removeDeck(std::wstring_view filename) const;
        std::shared_ptr<const VocabularyDeck> getCurrentDeck() const;

        static constexpr const std::wstring_view VocabularyDeck_Exstension =
            L".vd";

        static constexpr const auto FileName_Jmdict = "JMdict_e";

        static constexpr const auto PostFix_Anki_KanjiEnglish =
            "-vocab-kanji-eng.anki";
        static constexpr const auto PostFix_Anki_KanjiHiragana =
            "-vocab-kanji-hiragana.anki";

        static constexpr std::array VocabularyType_Prefix = {
            std::pair<detail::Vocabulary::Type, const char*>(
                detail::Vocabulary::Type::N1, "n1"),
            std::pair<detail::Vocabulary::Type, const char*>(
                detail::Vocabulary::Type::N2, "n2"),
            std::pair<detail::Vocabulary::Type, const char*>(
                detail::Vocabulary::Type::N3, "n3"),
            std::pair<detail::Vocabulary::Type, const char*>(
                detail::Vocabulary::Type::N4, "n4"),
            std::pair<detail::Vocabulary::Type, const char*>(
                detail::Vocabulary::Type::N5, "n5")};

      private:
        std::shared_ptr<VocabularyDeck> m_CurrentDeck;

        const VocabularyTranslator m_Translator;
        const std::filesystem::path m_UserFilePath;
        const detail::VocabularyVector m_AnkiVocabulary;
        const detail::VocabularyVector m_JmdictVocabulary;

        // has to be the last VocVector due to initializatin order
        const detail::VocabularyVector m_AllVocabulary;
    };

} // namespace shared
