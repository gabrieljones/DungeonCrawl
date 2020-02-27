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

STATE_DEC(avatarS);
STATE_DEC(avatarMovingS);
STATE_DEC(pathS);
STATE_DEC(wallS);
STATE_DEC(resetBroadcastS);
STATE_DEC(resetIgnoreS);
STATE_DEC(resetS);

STATE_DEF(setupS,
  { //entry
    revealed = false;
    randomize();
  },
  { //loop
    if(startState() == START_STATE_WE_ARE_ROOT) {
      changeState(avatarS::state);
    } else {
      changeState(random(2) == 0 ? wallS::state : pathS::state);
    }
  }
)

STATE_DEF(avatarS, 
  { //entry
    setValueSentOnAllFaces(AVATAR);
    setColor(AVATAR_COLOR);
    revealed = true; //will become revealed path after adventurer leaves
  },
  { //loop
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case MOVE: //avatar is being pulled to neighbor revert to path
            changeState(pathS::state);
            break;
        }
      }
    }
  }
)

STATE_DEF(avatarMovingS, 
  { //entry
    setValueSentOnFace(MOVE, heading); //tell neighbor avatar is moving here
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
  },
  { //loop
    bool doneMoving = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case AVATAR: //wait for all neighbors to be not AVATARs
            doneMoving = false;
            break;
          case RESET:
            changeState(resetBroadcastS::state);
            break;
        }
      }
    }
    if (doneMoving) { //after avatar is confirmed to be here then transition to actual Avatar state
      changeState(avatarS::state);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS::state);
    }
  }
)

STATE_DEF(pathS, 
  { //entry
    setValueSentOnAllFaces(PATH);
    setColor(revealed ? PATH_COLOR : FOG_COLOR);
  },
  { //loop
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case AVATAR: //reveal and pull avatar
              revealed = true;
              heading = f;
              changeState(avatarMovingS::state);
              break;
          }
        }
      }
    }
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case RESET:
            changeState(resetBroadcastS::state);
            break;
        }
      }
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS::state);
    }
  }
)

STATE_DEF(wallS, 
  { //entry
    setValueSentOnAllFaces(revealed ? WALL_REVEALED : WALL);
    setColor(revealed ? WALL_COLOR : FOG_COLOR);
  },
  { //loop
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case AVATAR: //reveal and don't pull avatar
              revealed = true;
              heading = f;
              changeState(wallS::state);
              break;
          }
        }
      }
    }
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case RESET:
            changeState(resetBroadcastS::state);
            break;
        }
      }
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS::state);
    }
  }
)

//broadcast reset for a bit
STATE_DEF(resetBroadcastS, 
  { //entry
    setValueSentOnAllFaces(RESET);
    setColor(RESET_COLOR);
    timer.set(512);
  },
  { //loop
    if (timer.isExpired()) {
      changeState(resetIgnoreS::state);
    }
  }
)

//stop broadcasting reset after a bit, then ignore the reset broadcast
STATE_DEF(resetIgnoreS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    setColor(dim(RESET_COLOR, 128));
    timer.set(512);
  },
  { //loop
    if (timer.isExpired()) {
      changeState(resetS::state);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS::state);
    }
  }
)

//ignore reset wave for a bit more, then reinitialize
STATE_DEF(resetS, 
  { //entry
    timer.set(512);
  },
  { //loop
    if (timer.isExpired()) {
      changeState(setupS::state);
    }
    if (buttonLongPressed()) {
      changeState(resetBroadcastS::state);
    }
  }
)

void setup() {
  changeState(setupS::state);
}

void loop() {
  stateFn();
}

/*
 * 3422, 700
 * /
 */
