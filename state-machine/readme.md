# State machine 

## Purpose
This module implements a declarative API for creating simple event driven Mealy state machines. 

## Usage

Event and state constants must be a power of two so that they can be properly 
merged and bitmasked together

Below is an example state machine

```
//   v----------r, bar()------------| 
// START -------------e1,foo()--> FAULT-------------|
//   ^--e2,foo()--|                 ^---e1, foo()---|

// this defines any state transitions
StateTransition_t tbtest[] = {
	// on these states, during this event, execute this function, and move to this state
	{START | FAULT, e1, foo, FAULT },
	{FAULT, r, bar, START },
	{START , e2, foo, START },
};

main(){
	StateMachine_t test_sm = {
		.name = "test_sm", // for debugging
		.state = START, // initial state
		.table = tbtest, // state transition table to reference
		.tbsize = sizeof(tbtest), // size of stt
	};

	update_state(&test_sm, e1); // update the state by passing in any events that occur
}

```