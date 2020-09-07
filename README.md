# Labyrinth
###### by Gabriel Jones
  
| Number of Blinks | Number of Players | Duration of Gameplay | Recommended Ages |
|:----------------:|:-----------------:|:--------------------:|:----------------:|
| 12+              | 1 maybe more?     | 6 minutes            | 8 & Up           |

## Objective

You have been imprisoned within the labyrinth. You must escape before you die of starvation (6 minutes).
You are on the sixth sublevel of the labyrinth you must search for the stairs to the levels above.
Reach the stairs to the outside world and you shall gain your freedom.
Beware, areas of the labyrinth that are out of sight tend to change.  

## Setup

All tiles start as `Fog` tiles.
Long press a `Fog` tile to change it to the avatar.

## Gameplay

- Place `Fog` tiles next to the `Avatar` to reveal the adjacent space around the `Avatar`.
- Click on `Path` tiles adjacent to the `Avatar` to move the avatar to that tile.
- Click on `Stairs` tiles adjacent to the `Avatar` to ascend to the level above.
- Remove tiles from the board to create new `Fog` tiles that can be placed next the `Avatar` to reveal as yet unexplored areas of the labyrinth.
- If you escape the labyrinth within the allotted time you escape (WIN).
- If you fail to find the sixth and final `Stairs` to the surface before time is up you starve and are imprisoned within the labyrinth forever (LOSE).

## Tiles

### Fog
 - Color: White
 - Indicates time remaining.
 - **Long-press**: Become `Avatar`.
 - **Place next to `Avatar`**: Reveal that tile.
 - **Place next to other tile**: Stay `Fog`.

### Avatar
 - Color: Green
 - Indicates the current sublevel by the number of lit pips.

### Path
 - Color: Dark Blue
 - Adjacent to `Avatar`
   - **Single-click**: Move avatar to tile.
 - Away from `Avatar`
   - After some time reverts to `Fog`.

### Stairs
 - Color: Yellow
 - Adjacent to `Avatar`
   - **Single-click**: Move avatar to tile and ascend to next level above.
 - Away from `Avatar`
   - After some time reverts to `Fog`.

### Wall
 - Color: Red
 - Adjacent to `Avatar`
   - **Single-click**: Nothing happens.
 - Away from `Avatar`
   - After some time reverts to `Fog`.

## Reset
 - **Long-press**: Reset all connected tiles to `Fog` and reset timer, to get ready for a new game.

## Issues
 - Possible to clone Avatar, this is unintentional.
 - Other Avatars are consumed by the ASCEND broadcast.
   - If I made Avatars ignore ASCEND, then multiple players each racing their own Avatar to the surface might be a thing.
 - Need a victory animation for the Avatar tile itself.



