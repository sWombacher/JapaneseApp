#include "sharedlogic.h"
#include <iostream>
#include <unordered_map>

static constexpr std::wstring_view StartLoop = L"start ";
static constexpr std::wstring_view FinishLoop = L"end";

struct SimpleIOHandler {
    std::wstring readLine() {
        std::wstring result;
        std::getline(std::wcin, result);
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    void writeLine(std::wstring_view line = L"",
                   bool appendFinishLoopInfo = false) {

        if (appendFinishLoopInfo)
            std::wcout << line << L" (use \"" << FinishLoop << "\" to finish)";
        else
            std::wcout << line;

        std::wcout << std::endl;
    }
};

static void listDecks(shared::LogicHandler& lh, bool) {
    SimpleIOHandler sioh;
    sioh.writeLine(L"available decks:");
    for (const auto& e : lh.listDecks())
        sioh.writeLine(e);
}

static void questioning(shared::LogicHandler& lh, bool forever) {
    SimpleIOHandler sioh;
    auto qh = lh.createQuestionHandler();
    const auto questions = qh.getQuestionSet(
        shared::QuestionHandler::FlagEnum_QuestionType::Vocabulary,
        shared::QuestionHandler::FlagEnum_TranslationType::KanaToEnglish);

    for (const auto& e : questions) {
        sioh.writeLine();
        sioh.writeLine(L"translate to english", true);
        sioh.writeLine(e.getQuestionVocabulary());

        const auto res = e.checkAnswer(sioh.readLine());
        e.acceptAnswer(res);

        sioh.writeLine(res ? L"correct!" : L"false!");
        sioh.writeLine();
        if (!forever)
            break;
    }
}

static void translate(shared::LogicHandler& lh, bool forever) {
    const auto& translator = lh.getVocabularyTranslator();
    SimpleIOHandler sioh;
    do {
        sioh.writeLine();
        sioh.writeLine(L"translate word", true);
        const auto line = sioh.readLine();
        if (line == FinishLoop)
            break;

        sioh.writeLine(translator.translateEnglish(line));
    } while (forever);
}

static void listVocabularies(shared::LogicHandler& lh, bool) {
    auto deck = lh.getCurrentDeck();
    SimpleIOHandler sioh;
    for (const auto& e : deck->getAllVocabularies())
        sioh.writeLine(e.kana + L"\t\t" + e.english.front());
}

static void createDeck(shared::LogicHandler& lh, bool) {
    auto newDeck = lh.createVocabularyDeck();
    SimpleIOHandler sioh;
    sioh.writeLine(L"Deck name: ");
    const auto name = sioh.readLine();
    while (true) {
        sioh.writeLine();
        sioh.writeLine(L"Add a english word to deck", true);
        auto english = sioh.readLine();
        if (english == FinishLoop)
            break;

        const auto vocabularies = lh.getAllVocabulary().findAllIf(
            [english](const detail::Vocabulary& voc) {
                for (const auto& e : voc.english) {
                    auto cpy = e;
                    std::for_each(cpy.begin(), cpy.end(), ::tolower);
                    if (cpy == english)
                        return true;
                }
                return false;
            });

        if (!vocabularies.empty()) {
            sioh.writeLine(L"found vocabulary: " + vocabularies.front()->kana);
            newDeck.addVocabularyUnique(*vocabularies.front());
        } else
            sioh.writeLine(L"vocabulary not found, nothing has been added!");
    }
    newDeck.saveAs(name);
    lh.loadDeck(name);
}

static void loadDeck(shared::LogicHandler& lh, bool) {
    SimpleIOHandler sioh;
    listDecks(lh, false);
    sioh.writeLine(L"filename:");
    lh.loadDeck(sioh.readLine());
}

static void removeDeck(shared::LogicHandler& lh, bool) {
    SimpleIOHandler sioh;
    listDecks(lh, false);
    sioh.writeLine(L"deck name to beremoved", true);
    const auto path = sioh.readLine();
    if (path != FinishLoop)
        lh.removeDeck(path);
}

static const std::unordered_map<std::wstring,
                                void (*)(shared::LogicHandler&, bool)>
    cmdToFunction = {
        {L"list decks", listDecks},   {L"questions", questioning},
        {L"translate", translate},    {L"list vocs", listVocabularies},
        {L"create deck", createDeck}, {L"load deck", loadDeck},
        {L"remove deck", removeDeck},
};

static void printUsage() {
    SimpleIOHandler sioh;
    sioh.writeLine();
    sioh.writeLine(L"Available commands", true);
    for (const auto& e : cmdToFunction)
        sioh.writeLine(e.first);
}

static void startLoop(shared::LogicHandler& lh) {
    for (SimpleIOHandler sioh; true;) {
        sioh.writeLine();
        auto line = sioh.readLine();
        const auto startLoop = line.find(StartLoop) == 0;
        if (startLoop)
            line = line.substr(StartLoop.size());

        const auto iter = cmdToFunction.find(line);
        if (iter == cmdToFunction.end()) {
            sioh.writeLine(L"Unknodwn command, try again");
            printUsage();
        } else
            iter->second(lh, startLoop);
    }
}

int main()
{
    setlocale(LC_ALL, "en_US.utf8");
    shared::LogicHandler lh("../databases", "./");
    printUsage();
    startLoop(lh);
    return 0;
}
