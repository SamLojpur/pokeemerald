#include "global.h"
#include "data.h"
#include "battle.h"
#include "pokemon.h"
#include "constants/moves.h"
#include "battle_setup.h"
#include "main.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"
#include "constants/battle_frontier_mons.h"
#include "battle_tower.h"
#include "constants/abilities.h"

#define MAX_INPUT_QUEUE 50

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

struct QueuedAction {
    enum TestAction action;
    u8 parameter;  // For move index, pokemon slot, etc.
};

// Global flag that battle system sets when ready for input
EWRAM_DATA bool8 gTestRunnerReadyForInput = FALSE;

static struct QueuedAction sActionQueue[MAX_INPUT_QUEUE];
static u8 sQueueCount;
static u8 sQueueIndex;
static bool8 sWaitingForState;
static enum TestAction sCurrentAction;
static u8 sActionCooldown;  // Frames to wait after executing an action
static u8 sStateStableFrames;  // Count frames state has been stable
static bool8 sLastStateCheck;  // Previous frame's state check result
static u8 sDebugCounter;  // Debug frame counter
static bool8 sWaitMessageSawBusy;  // For ACTION_WAIT_MESSAGE: have we seen ExecFlags != 0?
static bool8 sBattleStarted;  // Has the battle actually begun?
static u16 sFramesSinceBattleStart;  // Count frames since battle started

// Queue an action to be performed
void QueueAction(enum TestAction action, u8 parameter)
{
    if (sQueueCount < MAX_INPUT_QUEUE)
    {
        sActionQueue[sQueueCount].action = action;
        sActionQueue[sQueueCount].parameter = parameter;
        sQueueCount++;
    }
}

// Helper function for battle system to signal ready for input
void TestRunner_Battle_SetReadyForInput(void)
{
    if (!gTestRunnerReadyForInput) {
    gTestRunnerReadyForInput = TRUE;
    #ifdef MGBA_LOG_ENABLE
    DebugPrintf("Battle signaled ready for input!");
    #endif
    }
}

// Check if current game state matches what we're waiting for
static bool8 IsExpectedState(enum TestAction action)
{
    // #ifdef MGBA_LOG_ENABLE
    // if (sDebugCounter++ % 60 == 0)  // Log every 60 frames
    // {
    //     DebugPrintf("State check for action %d: ExecFlags=%d, InMenuId=%d, newKeys=%d, heldKeys=%d",
    //                 action, gBattleControllerExecFlags, gBattlerInMenuId, gMain.newKeys, gMain.heldKeys);
    // }
    // #endif
    
    switch (action)
    {
    case ACTION_WAIT_MENU:
    case ACTION_SELECT_FIGHT:
    case ACTION_SELECT_POKEMON:
    case ACTION_SELECT_BAG:
    case ACTION_SELECT_RUN:
    case ACTION_SELECT_MOVE:
        // All menu/selection actions require the ready flag
        if (!gTestRunnerReadyForInput)
        {
            return FALSE;
        }
        
        // Additional safety checks
        // if (gBattleControllerExecFlags != 0)
        // {
        //     return FALSE;
        // }
        if (gBattlerInMenuId >= MAX_BATTLERS_COUNT)
        {
            return FALSE;
        }
        if (gMain.newKeys != 0 || gMain.heldKeys != 0)
        {
            return FALSE;
        }
        return TRUE;
        
    case ACTION_WAIT_MESSAGE:
        // Messages should wait for ExecFlags to go busy (!=0) then return to idle (==0)
        // This ensures animations/messages have started and completed
        
        if (gBattleControllerExecFlags != 0)
        {
            sWaitMessageSawBusy = TRUE;  // We've seen it become busy
            return FALSE;  // Still waiting
        }
        
        // ExecFlags is 0 now
        if (sWaitMessageSawBusy)
        {
            // We saw it busy and now it's idle - animation complete!
            // Also make sure no keys are being processed
            return gMain.newKeys == 0 && gMain.heldKeys == 0;
        }
        
        // ExecFlags is 0 but we haven't seen it busy yet - keep waiting
        return FALSE;
        
    case ACTION_CONFIRM:
        // Generic confirmation - wait for stable state
        return gMain.newKeys == 0 && gMain.heldKeys == 0;
        
    default:
        return FALSE;
    }
}

// Get the button input for the current action
static u16 GetActionInput(enum TestAction action, u8 parameter)
{
    switch (action)
    {
    case ACTION_WAIT_MENU:
    case ACTION_WAIT_MESSAGE:
        return A_BUTTON;  // Advance through waits with A
        
    case ACTION_SELECT_FIGHT:
        return A_BUTTON;  // Fight is default option
        
    case ACTION_SELECT_MOVE:
        // Navigate to move slot then press A
        // For now, just press A (assumes cursor is correct)
        // You could add DPAD logic here for specific moves
        if (parameter > 0)
        {
            // Would need to add DPAD navigation based on parameter
            // For now, simplified
            return A_BUTTON;
        }
        return A_BUTTON;
        
    case ACTION_SELECT_POKEMON:
        return DPAD_RIGHT | A_BUTTON;  // Navigate right then select
        
    case ACTION_SELECT_BAG:
        return DPAD_DOWN | A_BUTTON;
        
    case ACTION_SELECT_RUN:
        return DPAD_DOWN | DPAD_RIGHT | A_BUTTON;
        
    case ACTION_CONFIRM:
        return A_BUTTON;
        
    default:
        return 0;
    }
}

// Main function called from ReadKeys
u16 GetScheduledInput(void)
{
    struct QueuedAction *currentAction;
    u16 buttons;
    bool8 stateReady;
    
    // Detect when battle has actually started
    if (!sBattleStarted)
    {
        // Battle is considered started when we have valid battle data
        if (gBattleControllerExecFlags != 0 || gBattlerInMenuId < MAX_BATTLERS_COUNT)
        {
            sBattleStarted = TRUE;
            #ifdef MGBA_LOG_ENABLE
            DebugPrintf("Battle detected as started!");
            #endif
            // gSideStatuses[0] = SIDE_STATUS_REFLECT;
            // gSideStatuses[1] = SIDE_STATUS_REFLECT;

            // gSideTimers[0].reflectTimer = 5;
            // gSideTimers[0].reflectBattlerId = gBattlerAttacker;

            // gSideTimers[1].reflectTimer = 5;
            // gSideTimers[1].reflectBattlerId = gBattlerTarget;
        }
        else
        {
            return 0;  // Don't process any inputs until battle starts
        }
    }
    
    // Count frames since battle started
    if (sBattleStarted)
    {
        sFramesSinceBattleStart++;
        
        // Wait at least 60 frames (1 second) after battle start before first action
        if (sFramesSinceBattleStart < 60)
        {
            #ifdef MGBA_LOG_ENABLE
            if (sFramesSinceBattleStart % 30 == 0)
            {
                DebugPrintf("Waiting for battle to settle... frame %d/60", sFramesSinceBattleStart);
            }
            #endif
            return 0;
        }
    }
    
    // If we have no more actions, return no input
    if (sQueueIndex >= sQueueCount)
        return 0;
    
    // If we're in cooldown, decrement and return no input
    if (sActionCooldown > 0)
    {
        sActionCooldown--;
        return 0;
    }
    
    currentAction = &sActionQueue[sQueueIndex];
    stateReady = IsExpectedState(currentAction->action);
    
    // Require state to be stable for at least 3 frames before acting
    if (stateReady)
    {
        if (sLastStateCheck)
        {
            sStateStableFrames++;
        }
        else
        {
            sStateStableFrames = 1;
            sLastStateCheck = TRUE;
            #ifdef MGBA_LOG_ENABLE
            DebugPrintf("State became ready for action %d", currentAction->action);
            #endif
        }
        
        // Need 3 stable frames before we consider state truly ready
        if (sStateStableFrames < 3)
            return 0;
            
        #ifdef MGBA_LOG_ENABLE
        if (sStateStableFrames == 3)
        {
            DebugPrintf("State stable for 3 frames, executing action %d", currentAction->action);
        }
        #endif
    }
    else
    {
        sStateStableFrames = 0;
        sLastStateCheck = FALSE;
        return 0;
    }
    
    // State is stable and ready, execute action
    buttons = GetActionInput(currentAction->action, currentAction->parameter);
    sQueueIndex++;
    sActionCooldown = 20;  // Increased to 20 frames to give game more time to process
    sStateStableFrames = 0;
    sLastStateCheck = FALSE;
    
    // Clear the ready flag after consuming input
    gTestRunnerReadyForInput = FALSE;
    
    // Reset wait message tracking when we move to next action
    if (currentAction->action == ACTION_WAIT_MESSAGE)
    {
        sWaitMessageSawBusy = FALSE;
    }
    
    #ifdef MGBA_LOG_ENABLE
    DebugPrintf("Executing action %d with param %d, buttons=%d", currentAction->action, currentAction->parameter, buttons);
    #endif
    
    return buttons;
}

bool8 HasScheduledInput(void)
{
    if (sQueueIndex >= sQueueCount)
        return FALSE;
        
    return IsExpectedState(sActionQueue[sQueueIndex].action);
}

void ResetActionQueue(void)
{
    sQueueCount = 0;
    sQueueIndex = 0;
    sWaitingForState = FALSE;
    sActionCooldown = 0;
    sStateStableFrames = 0;
    sLastStateCheck = FALSE;
    sDebugCounter = 0;
    sWaitMessageSawBusy = FALSE;
    sBattleStarted = FALSE;
    sFramesSinceBattleStart = 0;
    gTestRunnerReadyForInput = FALSE;
}

// Test battle setup
static void SetupTestBattle(void);

enum {
    STATE_INIT,
    STATE_SETUP_BATTLE,
    STATE_START_BATTLE,
    STATE_BATTLE_RUNNING,
    STATE_CHECK_RESULT,
    STATE_NEXT_TEST,
    STATE_DONE
};

static u8 sTestState;
static u8 sCurrentTest;

void CB2_TestRunner(void)
{
    switch (sTestState)
    {
    case STATE_INIT:
        #ifdef MGBA_LOG_ENABLE
        DebugPrintf("=== TEST MODE STARTED ===");
        #endif
        sTestState = STATE_SETUP_BATTLE;
        break;
        
    case STATE_SETUP_BATTLE:
        #ifdef MGBA_LOG_ENABLE
        DebugPrintf("Setting up test battle...");
        #endif
        SetupTestBattle();
        sTestState = STATE_START_BATTLE;
        break;
        
    case STATE_START_BATTLE:
        #ifdef MGBA_LOG_ENABLE
        DebugPrintf("Starting battle...");
        #endif
        SetMainCallback2(CB2_InitBattle);
        sTestState = STATE_BATTLE_RUNNING;
        break;
        
    case STATE_BATTLE_RUNNING:
        break;
        
    case STATE_CHECK_RESULT:
        #ifdef MGBA_LOG_ENABLE
        DebugPrintf("Test complete!");
        #endif
        sTestState = STATE_DONE;
        break;
        
    case STATE_DONE:
        break;
    }
}

void SetupTestBattle(void)
{
    u16 thunderbolt = MOVE_THUNDERBOLT;
    u16 tackle = MOVE_TACKLE;
    u16 surf = MOVE_SURF;
    u16 flamethrower = MOVE_FLAMETHROWER;
    u16 earthquake = MOVE_EARTHQUAKE;
    u16 splash = MOVE_SPLASH;
    u16 thick_fat = ABILITY_THICK_FAT;
    u16 guts = ABILITY_GUTS;
    u16 burn = STATUS1_BURN;

    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;

    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    
    // Create player Pokemon
    CreateMon(&gPlayerParty[0], 
              SPECIES_MACHAMP, 50, 31, TRUE, 0, OT_ID_PLAYER_ID, 0);
    SetMonData(&gPlayerParty[0], MON_DATA_MOVE1, &earthquake);
    SetMonData(&gPlayerParty[0], MON_DATA_ABILITY_NUM, &guts);
    // SetMonData(&gPlayerParty[0], MON_DATA_STATUS, &burn);

    CreateMon(&gPlayerParty[1], 
              SPECIES_PIKACHU, 50, 31, TRUE, 0, OT_ID_PLAYER_ID, 0);
    
    // Create opponent Pokemon
    CreateMon(&gEnemyParty[0],
              SPECIES_RATTATA, 50, 31, TRUE, 0, OT_ID_PLAYER_ID, 0);
    // SetMonData(&gEnemyParty[0], MON_DATA_ABILITY_NUM, &thick_fat);
    SetMonData(&gEnemyParty[0], MON_DATA_MOVE1, &splash);

    CreateMon(&gEnemyParty[1],
              SPECIES_RATTATA, 50, 31, TRUE, 0, OT_ID_PLAYER_ID, 0);

    // Reset and queue actions
    ResetActionQueue();


    // Example action sequence - much cleaner!
    QueueAction(ACTION_WAIT_MENU, 0);     // Wait for battle menu
    QueueAction(ACTION_SELECT_FIGHT, 0);  // Select Fight
    QueueAction(ACTION_SELECT_MOVE, 0);   // Select first move
    QueueAction(ACTION_WAIT_MESSAGE, 0);  // Wait for attack to complete
    
    // Turn 2
    // QueueAction(ACTION_WAIT_MENU, 0);
    // QueueAction(ACTION_SELECT_FIGHT, 0);
    // QueueAction(ACTION_SELECT_MOVE, 0);
    // QueueAction(ACTION_WAIT_MESSAGE, 0);
    
    // // Turn 3
    // QueueAction(ACTION_WAIT_MENU, 0);
    // QueueAction(ACTION_SELECT_FIGHT, 0);
    // QueueAction(ACTION_SELECT_MOVE, 0);
    
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FACTORY;
    gTrainerBattleOpponent_A = TRAINER_BRENDAN_ROUTE_103_MUDKIP;

    #ifdef MGBA_LOG_ENABLE
    DebugPrintf("Test battle setup complete with action queue");
    #endif
}

// In ReadKeys function:
// static void ReadKeys(void)
// {
//     u16 keyInput = REG_KEYINPUT ^ KEYS_MASK;
//     #ifdef TEST_MODE
//     if (HasScheduledInput())
//     {
//         keyInput = GetScheduledInput();
//     }
//     #endif
//     // ... rest of ReadKeys implementation
// }