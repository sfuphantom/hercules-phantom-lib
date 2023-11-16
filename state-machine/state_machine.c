#include "state_machine.h"

int update_state(StateMachine_t* sm, uint32_t event){

	// get state lookup table
	StateTransition_t* stt = sm->table;
	for (int i = 0; i < sm->tbsize; i++){

		StateTransition_t row = stt[i];
		// look up state definition
		if (sm->state & row.state_mask) {
			
			// check if event conditions match
			if (event & row.event_mask) {
				row.action(row.next_state);
				sm->state = row.next_state;

				return 1; // should only transition states once for a given event
			}
		}
	}
	return 0; // no state change
}

