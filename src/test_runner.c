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

// #ifdef TEST_MODE

#define MAX_INPUT_SCHEDULE 50

struct InputFrame {
    u16 frame;
    u16 buttons;
};

static struct InputFrame sInputSchedule[MAX_INPUT_SCHEDULE];
static u8 sScheduleCount;
static u16 sCurrentFrame;
static u8 sScheduleIndex;

void QueueInputAtFrame(u16 frame, u16 buttons)
{
    if (sScheduleCount < MAX_INPUT_SCHEDULE)
    {
        sInputSchedule[sScheduleCount].frame = frame;
        sInputSchedule[sScheduleCount].buttons = buttons;
        sScheduleCount++;
    }
}

u16 GetScheduledInput(void)
{
    // Check if we should inject input this frame
    if (sScheduleIndex < sScheduleCount && 
        sInputSchedule[sScheduleIndex].frame == sCurrentFrame)
    {
        u16 buttons = sInputSchedule[sScheduleIndex].buttons;
        sScheduleIndex++;
        return buttons;
    }
    return 0;  // No input this frame
}

void AdvanceInputFrame(void)
{
    sCurrentFrame++;
}

bool8 HasScheduledInput(void)
{
    return sScheduleIndex < sScheduleCount && 
           sInputSchedule[sScheduleIndex].frame == sCurrentFrame;
}

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
        // Battle is running, we'll return here after battle ends
        // For now, just let it run manually
        break;
        
    case STATE_CHECK_RESULT:
        #ifdef MGBA_LOG_ENABLE
        DebugPrintf("Test complete!");
        #endif
        sTestState = STATE_DONE;
        break;
        
    case STATE_DONE:
        // For now, just idle
        break;
    }
}

void SetupTestBattle(void)
{
    u16 thunderbolt = MOVE_THUNDERBOLT;
    u16 tackle = MOVE_TACKLE;
    u16 surf = MOVE_SURF;
    u16 flamethrower = MOVE_FLAMETHROWER;
    u16 splash = MOVE_SPLASH;

    u16 thick_fat = ABILITY_THICK_FAT;

    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;

    // Clear parties
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    
    // Create player Pokemon - Pikachu with Thunderbolt
    CreateMon(&gPlayerParty[0], 
              SPECIES_CHARIZARD,
              50,                    // level
              31,                    // maxIV
              TRUE,                  // hasFixedPersonality
              0,                     // fixedPersonality
              OT_ID_PLAYER_ID,       // otIdType
              0);                    // fixedOtId
    
    SetMonData(&gPlayerParty[0], MON_DATA_MOVE1, &flamethrower);

    CreateMon(&gPlayerParty[1], 
            SPECIES_PIKACHU,
            50,                    // level
            31,                    // maxIV
            TRUE,                  // hasFixedPersonality
            0,                     // fixedPersonality
            OT_ID_PLAYER_ID,       // otIdType
            0);     
    
    // Create opponent Pokemon - Geodude with Tackle
    CreateMon(&gEnemyParty[0],
              SPECIES_SNORLAX,
              50,
              31,
              TRUE,
              0,
              OT_ID_PLAYER_ID,
              0);
    
    SetMonData(&gEnemyParty[0], MON_DATA_ABILITY_NUM, &thick_fat);
    SetMonData(&gEnemyParty[0], MON_DATA_MOVE1, &splash);

    CreateMon(&gEnemyParty[1],
            SPECIES_RATTATA,
            50,
            31,
            TRUE,
            0,
            OT_ID_PLAYER_ID,
            0);

    // gBattleMons[gBattlerTarget].statStages[STAT_DEF] = 8;

    sScheduleCount = 0;
    sCurrentFrame = 0;
    sScheduleIndex = 0;
    
    // Space out inputs so newKeys logic works
    QueueInputAtFrame(60,  A_BUTTON);  // 1 second in
    QueueInputAtFrame(120, A_BUTTON);  // 2 seconds
    QueueInputAtFrame(180, A_BUTTON);  // 3 seconds - open menu
    QueueInputAtFrame(182, A_BUTTON);  // Select Fight
    QueueInputAtFrame(200, A_BUTTON);  // Select move
    
    // Next turn
    QueueInputAtFrame(400, A_BUTTON);  // Open menu
    QueueInputAtFrame(402, A_BUTTON);  // Fight
    QueueInputAtFrame(420, A_BUTTON);  // Move    
    QueueInputAtFrame(500, A_BUTTON);  // Open menu
    QueueInputAtFrame(600, A_BUTTON);  // Fight
    QueueInputAtFrame(700, A_BUTTON);  // Move    
    QueueInputAtFrame(800, A_BUTTON);  // Move    
    QueueInputAtFrame(900, A_BUTTON);  // Move    
    QueueInputAtFrame(1000, A_BUTTON);  // Move
    QueueInputAtFrame(1100, A_BUTTON);  // Open menu
    QueueInputAtFrame(1200, A_BUTTON);  // Fight
    QueueInputAtFrame(1300, A_BUTTON);  // Move    
    QueueInputAtFrame(1400, A_BUTTON);  // Move    
    QueueInputAtFrame(1450, A_BUTTON);  // Move    
    QueueInputAtFrame(1500, A_BUTTON);  // Open menu
    QueueInputAtFrame(1600, A_BUTTON);  // Fight
    QueueInputAtFrame(1700, A_BUTTON);  // Move    
    QueueInputAtFrame(1800, A_BUTTON);  // Move    
    QueueInputAtFrame(1900, A_BUTTON);  // Move    
    QueueInputAtFrame(2000, A_BUTTON);  // Move    
    
    // Set battle type
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_FACTORY;

    gTrainerBattleOpponent_A = TRAINER_BRENDAN_ROUTE_103_MUDKIP;

    #ifdef MGBA_LOG_ENABLE
    DebugPrintf("Created Pikachu vs Geodude battle");
    #endif
}

// #endif // TEST_MODE