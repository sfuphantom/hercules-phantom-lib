#pragma once
#include "stdint.h"

typedef void (*enter_action_t)(uint32_t);

typedef struct StateTransition_t{
	uint32_t state_mask;
	uint32_t event_mask;
	enter_action_t action;
	uint32_t next_state;
}StateTransition_t;

typedef struct StateMachine_t{
	const char* name;
	StateTransition_t* table;
	uint8_t tbsize;
	uint32_t state;
}StateMachine_t;

int update_state(StateMachine_t* sm, uint32_t event);


