//int entry_state(void);
//int calibration_state(void);
//int localization_state(void);
//int exit_state(void);
//
///* array and enum below must be in sync! */
//int (* state[])(void) = { entry_state, calibration_state, localization_state, exit_state};
//enum state_codes { entry, calibration, localization, end};
//
//enum ret_codes { ok, fail, repeat};
//struct transition {
//    enum state_codes src_state;
//    enum ret_codes   ret_code;
//    enum state_codes dst_state;
//};
//
///* transitions from end state aren't needed */
//struct transition state_transitions[] = {
//    {entry, ok,     calibration},
//    {entry, fail,   end},
//    {calibration,   ok,     localization},
//    {calibration,   fail,   end},
//    {localization,   ok,     end},
//    {localization,   fail,   end},
//    {localization,   repeat, localization}};
//
//#define EXIT_STATE end
//#define ENTRY_STATE entry
//
//enum state_codes cur_state = ENTRY_STATE;
//
//void init_states() {
//}
