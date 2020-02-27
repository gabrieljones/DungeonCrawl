#include "State.h"

#define PATH_COLOR dim(BLUE, 64)
#define AVATAR_COLOR GREEN
#define WALL_COLOR RED
#define FOG_COLOR WHITE
#define RESET_COLOR MAGENTA

enum commandStates {NONE, AVATAR, MOVE, PATH, WALL, WALL_REVEALED, RESET}; //add a treasure tile?
bool revealed = false;
byte heading = 0;
Timer timer;

extern const State avatarS;
extern const State avatarMovingS;
extern const State pathS;
extern const State wallS;
extern const State resetBroadcastS;
extern const State resetIgnoreS;
extern const State resetS;

const State setupS {
  NoOp,
  []() {
    revealed = false;
    randomize();
    if(startState() == START_STATE_WE_ARE_ROOT) {
      changeState(avatarS);
    } else {
      changeState(random(2) == 0 ? wallS : pathS);
    }
  }
};

const State avatarS {
  []() { //entry
    setValueSentOnAllFaces(AVATAR);
    setColor(AVATAR_COLOR);
    revealed = true; //will become revealed path after adventurer leaves
  },
  []() { //loop
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case MOVE: //avatar is being pulled to neighbor revert to path
            changeState(pathS);
            break;
        }
      }
    }
  }
};

const State avatarMovingS {
  []() { //entry
    setValueSentOnFace(MOVE, heading); //tell neighbor avatar is moving here
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  },
  []() { //loop
    bool doneMoving = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case AVATAR: //wait for all neighbors to be not AVATARs
            doneMoving = false;
            break;
          case RESET:
            changeState(resetBroadcastS);
            break;
        }
      }
    }
    if (doneMoving) { //after avatar is confirmed to be here then transition to actual Avatar state
      changeState(avatarS);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS);
    }
  }
};

const State pathS {
  []() { //entry
    setValueSentOnAllFaces(PATH);
    setColor(revealed ? PATH_COLOR : FOG_COLOR);
  },
  []() { //loop
    if (buttonSingleClicked()) {      
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case AVATAR: //reveal and pull avatar
              revealed = true;
              heading = f;
              changeState(avatarMovingS);
              break;
          }
        }
      }
    }
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case RESET:
            changeState(resetBroadcastS);
            break;
        }
      }
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS);
    }
  }
};

const State wallS {
  []() { //entry
    setValueSentOnAllFaces(revealed ? WALL_REVEALED : WALL);
    setColor(revealed ? WALL_COLOR : FOG_COLOR);
  },
  []() { //loop
    if (buttonSingleClicked()) {      
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case AVATAR: //reveal and don't pull avatar
              revealed = true;
              heading = f;
              changeState(wallS);
              break;
          }
        }
      }
    }
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case RESET:
            changeState(resetBroadcastS);
            break;
        }
      }
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS);
    }
  }
};

//broadcast reset for a bit
const State resetBroadcastS {
  []() { //entry
    setValueSentOnAllFaces(RESET);
    setColor(RESET_COLOR);
    timer.set(512);
  },
  []() { //loop
    if (timer.isExpired()) {
      changeState(resetIgnoreS);
    }
  }
};


//stop broadcasting reset after a bit, then ignore the reset broadcast
const State resetIgnoreS {
  []() { //entry
    setValueSentOnAllFaces(NONE);
    setColor(dim(RESET_COLOR, 128));
    timer.set(512);
  },
  []() { //loop
    if (timer.isExpired()) {
      changeState(resetS);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS);
    }
  }
};

//ignore reset wave for a bit more, then reinitialize
const State resetS {
  []() { //entry
    timer.set(512);
  },
  []() { //loop
    if (timer.isExpired()) {
      changeState(setupS);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS);
    }
  }
};

void setup() {
  changeState(setupS);
}

void loop() {
  stateFn();
}
