#include <iostream>
#include "BoardManager.h"

int main() {

	AllSettings allSettings;

	RobotSettings robotSettings;
	robotSettings.isWhiteRobot = true;
	robotSettings.isBlackRobot = true;
	robotSettings.whiteBotType = SearcherType::IMPROVED_SEARCHER;
	robotSettings.blackBotType = SearcherType::OLD_SEARCHER;
	robotSettings.whiteRobotDifficulty = Difficulty::IMPOSSIBLE;
	robotSettings.blackRobotDifficulty = Difficulty::IMPOSSIBLE;
	robotSettings.numThreads = 1;

	TimeSettings timeSettings;
	timeSettings.incrementSec = 0.6;
	timeSettings.tournamentTimeSec = 60;
	timeSettings.gm = GameMode::TOURNAMENT_MODE;
	timeSettings.rtum = RobotTimeUsageMode::TOURNEMENT_TIME;

	allSettings.robotSettings = robotSettings;
	allSettings.timeSettings = timeSettings;

    BoardManager bm;
	// bm.runUCIService();
	bm.setSettings(allSettings);
	bm.startMultiThreadedSimulation(1000, 10);

	return 0;
}
