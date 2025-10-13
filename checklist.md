## Battle Test Framework MVP - Checklist
## 📋 TODO - Next Steps
### Phase 1: Multiple Test Scenarios

 Create helper functions for common setups

SetupTestBattle_SuperEffective()
SetupTestBattle_StatusMove()
SetupTestBattle_BothLowHP()


 Add ability to run multiple tests in sequence

### Phase 2: Automated Move Input

 Queue moves instead of manual input
 Hook battle input handler to read from test queue
 Auto-advance turns without button presses

### Phase 3: Result Validation

 Detect when battle ends properly
 Capture AI decisions (move/switch choices)
 Log results to mGBA console
 Compare against expected behavior

### Phase 4: Showdown Comparison

 Document AI behavior from Emerald tests
 Create parallel test scenarios in Showdown
 Compare AI decision outputs
 Iterate on Showdown AI port based on differences