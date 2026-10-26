//------------------------------------------------------------------------------
// Tyrian X: A modern C++ cross-platform port of Tyrian
// Copyright (C) The Tyrian X Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//------------------------------------------------------------------------------

// The C globals that Tyrian X registers, by state class.  Each list is an X
// macro: it takes a macro X and expands to X(symbol) for every symbol in the
// list, in alphabetical order.  Registered symbols need external linkage and a
// header declaration.
//
// Every other writable symbol of the C objects is listed by object in
// untracked.txt; tools/check_c_state_registry.py checks at link time
// that the two together classify every symbol.

// Gameplay state that changes during play.
#define TYRX_SIM(X) \
  X(allPlayersGone) X(anySmoothies) X(armorShipDelay) X(astralDuration) X(background2) \
  X(background2notTransparent) X(background2over) X(background3over) X(background3x1) X(background3x1b) \
  X(backMove) X(backMove2) X(backMove3) X(backPos) X(backPos2) X(backPos3) X(BKwrap1) X(BKwrap1to) \
  X(BKwrap2) X(BKwrap2to) X(BKwrap3) X(BKwrap3to) X(bonusLevel) X(bonusLevelCurrent) X(boss_bar) X(button) \
  X(chargeGr) X(chargeGrWait) X(chargeLevel) X(chargeMax) X(chargeWait) X(constantDie) X(constantLastX) \
  X(constantPlay) X(cubeList) X(cubeMax) X(curLoc) X(damageRate) X(difficultyLevel) X(displayScore) \
  X(doIced) X(doNotSaveBackup) X(editorItemAvail) X(editorLevel) X(endLevel) X(enemiesActive) X(enemy) \
  X(enemyAvail) X(enemyContinualDamage) X(enemyKilled) X(enemyOffset) X(enemyOnScreen) X(enemyShot) \
  X(enemyShotAvail) X(enemyStillExploding) X(episodeAvail) X(episodeNum) X(eventLoc) X(eventRec) \
  X(explosionTransparent) X(extraGame) X(fastPlay) X(filtrationAvail) X(fireButtonHeld) X(firstGameOver) \
  X(flareDuration) X(flareStart) X(forceEvents) X(frameCountMax) X(galagaLife) X(galagaMode) \
  X(galagaShotFreq) X(gameHasRepeated) X(gameJustLoaded) X(gameLoaded) X(gameSpeed) X(globalFlags) \
  X(infiniteShot) X(initial_episode_num) X(initialDifficulty) X(itemAvail) X(itemAvailMax) \
  X(jumpBackToEpisode1) X(jumpSection) X(lastCubeMax) X(lastLevelName) X(lastPower) X(levelEnd) \
  X(levelEndFxWait) X(levelEndWarp) X(levelName) X(levelSong) X(levelTimer) X(levelTimerCountdown) \
  X(levelTimerJumpTo) X(levelWarningDisplay) X(levelWarningLines) X(levelWarningText) X(linkGunDirec) \
  X(linkToPlayer) X(loadLevelOk) X(lvlFileNum) X(mainLevel) X(map1YDelay) X(map1YDelayMax) X(map2YDelay) \
  X(map2YDelayMax) X(mapOrigin) X(mapPlanet) X(mapPNum) X(mapSection) X(mapX) X(mapX2) X(mapX2bpPos) \
  X(mapX2Ofs) X(mapX2Pos) X(mapX3) X(mapX3bpPos) X(mapX3Ofs) X(mapX3Pos) X(mapXbpPos) X(mapXOfs) X(mapXPos) \
  X(mapY) X(mapY2) X(mapY2Pos) X(mapY3) X(mapY3Pos) X(mapYPos) X(maxEvent) X(mt_global) X(neat) X(newPL) \
  X(nextLevel) X(nextSpecialWait) X(normalBonusLevelCurrent) X(oldDifficultyLevel) X(oldMapX3Ofs) \
  X(oldMapXOfs) X(onePlayerAction) X(optionAttachmentLinked) X(optionAttachmentMove) \
  X(optionAttachmentReturn) X(optionSatelliteRotate) X(pentiumMode) X(playDemo) X(player) X(playerEndLevel) \
  X(playerShotData) X(portConfigChange) X(portConfigDone) X(power) X(powerAdd) X(processorType) \
  X(randomExplosions) X(readyToEndLevel) X(reallyEndLevel) X(recordDemo) X(rep_explosions) X(returnActive) \
  X(returnLoc) X(richMode) X(secretHint) X(SFCurrentCode) X(SFExecuted) X(shieldT) X(shieldWait) X(shipGr) \
  X(shipGr2) X(shipGr2ptr) X(shipGrPtr) X(shotAvail) X(shotMultiPos) X(shotRepeat) X(skyEnemyOverAll) \
  X(smallEnemyAdjust) X(smoothie_data) X(smoothies) X(specialWait) X(specialWeaponFilter) \
  X(specialWeaponFreq) X(specialWeaponWpn) X(spraySpecial) X(starShowVGASpecialCode) X(stopBackgroundNum) \
  X(stopBackgrounds) X(stoppedDemo) X(superArcadeMode) X(superArcadePowerUp) X(superEnemy254Jump) \
  X(superPause) X(superTyrian) X(superWild) X(tempBackMove) X(tempMapXOfs) X(topEnemyOver) X(totalEnemy) \
  X(trentWin) X(twoPlayerLinked) X(twoPlayerMode) X(useLastBank) X(waitToEndLevel) X(wild) X(xmas) \
  X(youAreCheating) X(zinglonDuration)

// State that changes during play but only affects what is shown or heard.
#define TYRX_PRESENT(X) \
  X(displayTime) X(explodeMove) X(explosionFollowAmountX) X(explosionFollowAmountY) X(explosions) \
  X(filterActive) X(filterFade) X(filterFadeStart) X(flareColChg) X(flash) X(flashChange) X(last_superpixel) \
  X(levelBrightness) X(levelBrightnessChg) X(levelFilter) X(levelFilterNew) X(musicFade) X(soundQueue) \
  X(starActive) X(starfield_speed) X(starfield_stars) X(superpixels) X(tempVolume) X(textErase) \
  X(warningCol) X(warningColChange) X(warningRed) X(warningSoundDelay)

// Level and episode data loaded from files (a few events modify it).
#define TYRX_DATA(X) \
  X(cubeFilename) X(enemyDat) X(episodeFilename) X(extraAvail) X(extraShips) X(levelEnemy) \
  X(levelEnemyFrequency) X(levelEnemyMax) X(levelFilename) X(lvlNum) X(lvlPos) X(megaData1) X(megaData2) \
  X(megaData3) X(options) X(powerSys) X(shields) X(ships) X(special) X(weaponPort) X(weapons)

// Input state at a tick boundary: not hashed, but restored by round trips.
#define TYRX_INPUT(X) \
  X(ESCPressed) X(ingamemenu_pressed) X(keyboardInputs) X(keyboardInputsBack) X(keyboardInputsCount) \
  X(keyboardInputsFront) X(keysactive) X(lordKeySymsDown) X(mouseButtonsDown) X(mouseHasMotionInput) \
  X(mouseInputs) X(mouseInputsBack) X(mouseInputsCount) X(mouseInputsFront) X(mouseRelativeEnabled) \
  X(mouseWindowXRelative) X(mouseWindowYRelative) X(mouseX) X(mouseY) X(pause_pressed) X(windowHasFocus)

// Not hashed; registered so that pointers into them can be described.
#define TYRX_POINTEES(X) \
  X(destructSpriteSheet) X(enemySpriteSheets) X(explosionSpriteSheet) X(shopSpriteSheet) X(spriteSheet8) \
  X(spriteSheet9) X(spriteSheet10) X(spriteSheet11) X(spriteSheet12)
