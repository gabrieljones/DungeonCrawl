typedef void (*Function0) ();

void NoOp() {}

typedef struct {
  Function0 entryFn;
  Function0 loopFn;
} State;

State * state;

Function0 stateFn;

void enterState() {
  state -> entryFn();
  stateFn = state -> loopFn;
}

void changeState(State _nextState) {
  state = & _nextState;
  stateFn = enterState;
}

/* Usage
const State nameS {
  []() { //entry
  },
  []() { //loop
  }
};
*/
