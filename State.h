typedef void (*Function0) ();

typedef struct {
  Function0 entryFn;
  Function0 loopFn;
} State;

State* state;

Function0 stateFn;

void enterState() {
  state -> entryFn();
  stateFn = state -> loopFn;
}

void changeState(const State* nextState) {
  state = nextState;
  stateFn = enterState;
}


#define STATE_DEC(stateName) namespace stateName { extern const State* state; }

#define STATE_DEF(stateName, entryBody, loopBody) \
namespace stateName { \
  void entry() entryBody \
  void loop() loopBody \
  const State _state { entry, loop }; \
  extern const State* state = &_state; \
}

/* Usage

STATE_DEC(stateName1)
STATE_DEC(stateName2)

STATE_DEF(stateName1, 
  {
    //entry function body, called once before state starts looping
  }, 
  {
    //loop function body, called repeatedly while this is the current state
    //some condition
    changeState(stateName2::state);
  }
)

STATE_DEF(name2, 
  {
    //entry function body
  }, 
  {
    //loop function body
    //some condition
    changeState(stateName1::state);
  }
)
*/
