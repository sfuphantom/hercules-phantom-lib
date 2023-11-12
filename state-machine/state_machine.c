#include "state_machine.h"

int update_state(StateMachine_t* sm, uint32_t event){

	// get state lookup table
	StateTransition_t* stt = sm->table;

	for (int i = 0; i < sm->tbsize; i++){

		StateTransition_t row = stt[i];

		// look up state definition
		if (sm->state & row.state_mask) {
			
			// check if event conditions match
			if (event & row.event_mask)
			{
				row.action(row.next_state);
				sm->state = row.next_state;

				// don't break because a state may apply
				// to multiple rows of the stt
			}
		}
	}
	return 1;
}

