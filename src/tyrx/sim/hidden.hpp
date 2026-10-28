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

// The C globals whose state sim::State holds, and the C functions that use
// them.  These names hide them from code in sim/ and port/ (whose headers have
// `using namespace hidden;`), so that using one by mistake doesn't compile.

#pragma once

namespace tyrx::hidden {

struct InSimState {};
inline constexpr InSimState twoPlayerMode, onePlayerAction, galagaMode, superArcadeMode, superArcadePowerUp,
    difficultyLevel, initialDifficulty, oldDifficultyLevel, superTyrian, processorType, pentiumMode,
    frameCountMax, fastPlay, gameSpeed, playDemo, recordDemo, xmas, displayScore, richMode;
inline constexpr InSimState youAreCheating, superWild, wild, doIced, infiniteShot, constantPlay, constantDie,
    constantLastX, superPause, globalFlags;
inline constexpr InSimState episodeNum, initial_episode_num, episodeAvail, lvlFileNum, levelName,
    lastLevelName, loadLevelOk, cubeList, cubeMax, lastCubeMax, itemAvail, itemAvailMax, editorLevel,
    editorItemAvail, secretHint, useLastBank, trentWin, jumpBackToEpisode1, extraGame, gameLoaded,
    gameJustLoaded, gameHasRepeated, doNotSaveBackup;
inline constexpr InSimState mt_global;
inline constexpr InSimState explosions, rep_explosions, explodeMove, explosionFollowAmountX,
    explosionFollowAmountY;
inline constexpr InSimState superpixels, last_superpixel;
inline constexpr InSimState starfield_stars, starfield_speed;
inline constexpr InSimState soundQueue, musicFade, tempVolume, levelSong;
inline constexpr InSimState levelFilter, levelFilterNew, levelBrightness, levelBrightnessChg, filtrationAvail,
    filterActive, filterFade, filterFadeStart, explosionTransparent, smoothies, smoothie_data, anySmoothies,
    flash, flashChange, warningCol, warningColChange, warningRed, warningSoundDelay, levelWarningDisplay,
    levelWarningLines, levelWarningText, textErase, displayTime, neat, starShowVGASpecialCode, starActive,
    boss_bar, portConfigChange, portConfigDone;
inline constexpr InSimState backPos, backPos2, backPos3, backMove, backMove2, backMove3, mapX, mapY, mapX2,
    mapX3, mapY2, mapY3, mapYPos, mapY2Pos, mapY3Pos, mapXPos, oldMapXOfs, mapXOfs, mapX2Ofs, mapX2Pos,
    mapX3Pos, oldMapX3Ofs, mapX3Ofs, tempMapXOfs, mapXbpPos, mapX2bpPos, mapX3bpPos, map1YDelay,
    map1YDelayMax, map2YDelay, map2YDelayMax, tempBackMove, BKwrap1, BKwrap2, BKwrap3, BKwrap1to, BKwrap2to,
    BKwrap3to, stopBackgrounds, stopBackgroundNum, background2, background2notTransparent, background3x1,
    background3x1b, background2over, background3over, skyEnemyOverAll, topEnemyOver;
inline constexpr InSimState eventRec, maxEvent, eventLoc, curLoc, returnLoc, returnActive, forceEvents,
    jumpSection, enemiesActive, levelEnemy, levelEnemyMax, levelEnemyFrequency, enemyContinualDamage,
    superEnemy254Jump, randomExplosions, smallEnemyAdjust, galagaShotFreq, galagaLife, levelTimer,
    levelTimerCountdown, levelTimerJumpTo, mapOrigin, mapPNum, mapPlanet, mapSection, newPL;
inline constexpr InSimState endLevel, reallyEndLevel, waitToEndLevel, playerEndLevel, readyToEndLevel,
    levelEnd, levelEndFxWait, levelEndWarp, firstGameOver, allPlayersGone, enemyStillExploding, stoppedDemo,
    bonusLevel, bonusLevelCurrent, normalBonusLevelCurrent, mainLevel, nextLevel;
inline constexpr InSimState button, fireButtonHeld;
inline constexpr InSimState player, shotRepeat, shotMultiPos, power, lastPower, powerAdd, shieldWait, shieldT,
    chargeWait, chargeLevel, chargeMax, chargeGr, chargeGrWait, damageRate, armorShipDelay, twoPlayerLinked,
    linkGunDirec, optionSatelliteRotate, optionAttachmentMove, optionAttachmentLinked, optionAttachmentReturn,
    shipGr, shipGr2, shipGrPtr, shipGr2ptr;
inline constexpr InSimState specialWait, nextSpecialWait, SFCurrentCode, SFExecuted, spraySpecial,
    specialWeaponWpn, specialWeaponFilter, specialWeaponFreq, linkToPlayer, flareDuration, flareStart,
    flareColChg, zinglonDuration, astralDuration;
inline constexpr InSimState enemy, enemyAvail, enemyOnScreen, totalEnemy, enemyKilled, enemyOffset;
inline constexpr InSimState playerShotData, shotAvail;
inline constexpr InSimState enemyShot, enemyShotAvail;
inline constexpr InSimState temp2;

// The C structs that sim/state.hpp's C++ structs stand for
// (../gold/mirror.hpp).
inline constexpr InSimState JE_EventRecType, Explosion, rep_explosion_type, superpixel_type, StarfieldStar,
    boss_bar_t, EnemyShotType, PlayerShotDataType, JE_SingleEnemyType, JE_MultiEnemyType, Player, PlayerItems;

// mtrand.h's functions, which draw from mt_global.
inline constexpr InSimState mt_rand, mt_rand_1, mt_rand_lt1;

}  // namespace tyrx::hidden
