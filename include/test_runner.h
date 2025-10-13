// include/test_runner.h
#ifndef GUARD_TEST_RUNNER_H
#define GUARD_TEST_RUNNER_H

#ifdef TEST_MODE

// Test action types
enum TestAction {
    ACTION_WAIT_MENU,      // Wait for main battle menu
    ACTION_SELECT_FIGHT,   // Select "Fight" option
    ACTION_SELECT_MOVE,    // Select a move (with index)
    ACTION_SELECT_POKEMON, // Select Pokemon switch
    ACTION_SELECT_BAG,     // Select Bag
    ACTION_SELECT_RUN,     // Select Run
    ACTION_CONFIRM,        // Generic A button confirm
    ACTION_WAIT_MESSAGE,   // Wait for message to complete
    ACTION_NONE
};

// Global flag set by battle system when ready for input
extern bool8 gTestRunnerReadyForInput;

// Main test runner callback
void CB2_TestRunner(void);

// Battle setup
void SetupTestBattle(void);

// Action queue management
void QueueAction(enum TestAction action, u8 parameter);
void ResetActionQueue(void);

// Input system (called from ReadKeys)
bool8 HasScheduledInput(void);
u16 GetScheduledInput(void);

// Helper to signal test system that battle is ready for input
void TestRunner_Battle_SetReadyForInput(void);

#endif // TEST_MODE
#endif // GUARD_TEST_RUNNER_H