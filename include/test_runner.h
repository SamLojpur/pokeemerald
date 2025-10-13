// include/test_runner.h (NEW FILE)
#ifndef GUARD_TEST_RUNNER_H
#define GUARD_TEST_RUNNER_H

#ifdef TEST_MODE
void CB2_TestRunner(void);
void SetupTestBattle(void);
void AdvanceInputFrame(void);
bool8 HasScheduledInput(void);
u16 GetScheduledInput(void);
void QueueInputAtFrame(u16 frame, u16 buttons);
#endif

#endif // GUARD_TEST_RUNNER_H
