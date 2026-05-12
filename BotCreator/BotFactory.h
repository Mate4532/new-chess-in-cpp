#pragma once
#include <memory>
#include "ISearcher.h"
#include "Settings.h"

class BotFactory {
public:
    static std::unique_ptr<ISearcher> createBot(SearcherType st, RobotSettings rt);
};