#include "State.h"

#define PATH_COLOR dim(BLUE, 64)
#define AVATAR_COLOR GREEN
#define WALL_COLOR RED
#define FOG_COLOR WHITE
#define RESET_COLOR MAGENTA
#define STAIRS_COLOR YELLOW

enum tileTypes {NONE, AVATAR, FOG, PATH, WALL, STAIRS, MOVE}; //add a treasure tile?
byte heading = 0;
Timer timer;

STATE_DEC(avatarS);
STATE_DEC(avatarMovingS);
STATE_DEC(fogS);
STATE_DEC(pathS);
STATE_DEC(stairsS);
STATE_DEC(wallS);

bool noPathToAvatar() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f) && getLastValueReceivedOnFace(f) != NONE) {
      return false;
    }
  }
  return true;
}

STATE_DEF(setupS,
  { //entry
    randomize();
  },
  { //loop
    changeState(startState() == START_STATE_WE_ARE_ROOT ? avatarS::state : fogS::state);
  }
)

STATE_DEF(avatarS, 
  { //entry
    //TODO set faces telling neighbors type of tile they are
    FOREACH_FACE(f) {
      setValueSentOnFace(f, f); //debug 
    }
    setColor(AVATAR_COLOR);
  },
  { //loop
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case MOVE: //avatar is being pulled to neighbor revert to path
            setValueSentOnFace(NONE, f); //inform requesting tile that the avatar has left this tile to live on the requesting tile
            changeState(pathS::state);
            break;
        }
      }
    }
  }
)

STATE_DEF(avatarMovingS, 
  { //entry
    setValueSentOnFace(MOVE, heading); //request the avatar move here
    setColorOnFace(AVATAR_COLOR, heading);
    setColorOnFace(AVATAR_COLOR, (heading + 1) % 6);
    setColorOnFace(AVATAR_COLOR, (heading + 5) % 6);
    timer.set(512); //if the avatar does not move here, then it must not be on the neighbor
  },
  { //loop
    bool doneMoving = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case PATH: //wait for all neighbors to be not PATHs, this means the AVATAR has moved to us
            doneMoving = false;
            break;
        }
      }
    }
    if (timer.isExpired()) { //avatar did not move here revert back to empty path
      setValueSentOnFace(NONE, heading); //stop requesting avatar moves here
      changeState(pathS::state);
    }
    if (doneMoving) { //after avatar is confirmed to be here then transition to actual Avatar state
      changeState(avatarS::state);
    }
  }
)

STATE_DEF(fogS, 
  { //entry
    setValueSentOnAllFaces(NONE);
    setColor(FOG_COLOR);
  },
  { //loop
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        switch(getLastValueReceivedOnFace(f)) {
          case WALL:
            changeState(wallS::state);
            break;
          case PATH:
            changeState(pathS::state);
            break;
          case STAIRS:
            changeState(stairsS::state);
            break;
        }
      }
    }
  }
)

STATE_DEF(pathS, 
  { //entry
//leave faces as they are    setValueSentOnAllFaces(PATH); 
    setColor(PATH_COLOR);
  },
  { //loop
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case PATH: //a neighbor is telling us we are a path, it might be the avatar, attempt to pull avatar
              heading = f;
              changeState(avatarMovingS::state);
              break;
          }
        }
      }
    }
    if(noPathToAvatar()) { changeState(fogS::state); }
  }
)


STATE_DEF(stairsS, 
  { //entry
//leave faces as they are    setValueSentOnAllFaces(PATH); 
    FOREACH_FACE(f) { 
      setColorOnFace(dim(STAIRS_COLOR, f * (255 / 6)), f);
    }
  },
  { //loop
    if (buttonSingleClicked()) {
      FOREACH_FACE(f) { //check if avatar is on neighbor
        if (!isValueReceivedOnFaceExpired(f)) {
          switch(getLastValueReceivedOnFace(f)) {
            case STAIRS: //a neighbor is telling us we are stairs, it might be the avatar, attempt to pull avatar
              heading = f;
              changeState(avatarMovingS::state);
              break;
          }
        }
      }
    }
    if(noPathToAvatar()) { changeState(fogS::state); }   
  }
)

STATE_DEF(wallS, 
  { //entry
//leave faces as they are    setValueSentOnAllFaces(NONE);
    setColor(WALL_COLOR);
  },
  { //loop
    if(noPathToAvatar()) { changeState(fogS::state); }
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
