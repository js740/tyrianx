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

// The C globals that hold the state of sim::State's members in the C game,
// and copying between the two.  The C++ game reads its State from them when a
// level starts, and after C code that works on them has run (the pause
// screen, the in-game menu and help), and writes it back after each pass, for
// the C code that runs between ticks and after the level.

#pragma once

#include "../sim/state.hpp"
#include "api.hpp"

namespace tyrx::c {

// Each member of each of State's subsystems, and the C global that holds its
// state in the C game.
template <typename State, typename F>
void for_each_binding(State& s, F f) {
  f(s.game.two_player_mode, ::twoPlayerMode);
  f(s.game.one_player_action, ::onePlayerAction);
  f(s.game.galaga_mode, ::galagaMode);
  f(s.game.super_arcade_mode, ::superArcadeMode);
  f(s.game.super_arcade_power_up, ::superArcadePowerUp);
  f(s.game.difficulty_level, ::difficultyLevel);
  f(s.game.initial_difficulty, ::initialDifficulty);
  f(s.game.old_difficulty_level, ::oldDifficultyLevel);
  f(s.game.super_tyrian, ::superTyrian);
  f(s.game.processor_type, ::processorType);
  f(s.game.pentium_mode, ::pentiumMode);
  f(s.game.frame_count_max, ::frameCountMax);
  f(s.game.fast_play, ::fastPlay);
  f(s.game.game_speed, ::gameSpeed);
  f(s.game.play_demo, ::playDemo);
  f(s.game.record_demo, ::recordDemo);
  f(s.game.xmas, ::xmas);
  f(s.game.display_score, ::displayScore);
  f(s.game.rich_mode, ::richMode);
  f(s.cheats.you_are_cheating, ::youAreCheating);
  f(s.cheats.super_wild, ::superWild);
  f(s.cheats.wild, ::wild);
  f(s.player_shots.do_iced, ::doIced);
  f(s.player_shots.infinite_shot, ::infiniteShot);
  f(s.cheats.constant_play, ::constantPlay);
  f(s.cheats.constant_die, ::constantDie);
  f(s.cheats.constant_last_x, ::constantLastX);
  f(s.cheats.super_pause, ::superPause);
  f(s.level.global_flags, ::globalFlags);
  f(s.progress.episode_num, ::episodeNum);
  f(s.progress.initial_episode_num, ::initial_episode_num);
  f(s.progress.episode_avail, ::episodeAvail);
  f(s.progress.lvl_file_num, ::lvlFileNum);
  f(s.progress.level_name, ::levelName);
  f(s.progress.last_level_name, ::lastLevelName);
  f(s.progress.load_level_ok, ::loadLevelOk);
  f(s.progress.cube_list, ::cubeList);
  f(s.progress.cube_max, ::cubeMax);
  f(s.progress.last_cube_max, ::lastCubeMax);
  f(s.progress.item_avail, ::itemAvail);
  f(s.progress.item_avail_max, ::itemAvailMax);
  f(s.progress.editor_level, ::editorLevel);
  f(s.progress.editor_item_avail, ::editorItemAvail);
  f(s.progress.secret_hint, ::secretHint);
  f(s.display.use_last_bank, ::useLastBank);
  f(s.progress.trent_win, ::trentWin);
  f(s.progress.jump_back_to_episode1, ::jumpBackToEpisode1);
  f(s.progress.extra_game, ::extraGame);
  f(s.progress.game_loaded, ::gameLoaded);
  f(s.progress.game_just_loaded, ::gameJustLoaded);
  f(s.progress.game_has_repeated, ::gameHasRepeated);
  f(s.progress.do_not_save_backup, ::doNotSaveBackup);
  f(s.random.mt, ::mt_global);
  f(s.explosions.slots, ::explosions);
  f(s.explosions.repeating, ::rep_explosions);
  f(s.explosions.follow_x, ::explosionFollowAmountX);
  f(s.explosions.follow_y, ::explosionFollowAmountY);
  f(s.explosions.move, ::explodeMove);
  f(s.superpixels.slots, ::superpixels);
  f(s.superpixels.last, ::last_superpixel);
  f(s.starfield.stars, ::starfield_stars);
  f(s.starfield.speed, ::starfield_speed);
  f(s.sound.queue, ::soundQueue);
  f(s.sound.music_fade, ::musicFade);
  f(s.sound.temp_volume, ::tempVolume);
  f(s.sound.level_song, ::levelSong);
  f(s.display.filter, ::levelFilter);
  f(s.display.filter_new, ::levelFilterNew);
  f(s.display.brightness, ::levelBrightness);
  f(s.display.brightness_chg, ::levelBrightnessChg);
  f(s.display.filtration_avail, ::filtrationAvail);
  f(s.display.filter_active, ::filterActive);
  f(s.display.filter_fade, ::filterFade);
  f(s.display.filter_fade_start, ::filterFadeStart);
  f(s.display.explosion_transparent, ::explosionTransparent);
  f(s.display.smoothies, ::smoothies);
  f(s.display.smoothie_data, ::smoothie_data);
  f(s.display.any_smoothies, ::anySmoothies);
  f(s.display.flash, ::flash);
  f(s.display.flash_change, ::flashChange);
  f(s.display.warning_col, ::warningCol);
  f(s.display.warning_col_change, ::warningColChange);
  f(s.display.warning_red, ::warningRed);
  f(s.display.warning_sound_delay, ::warningSoundDelay);
  f(s.display.level_warning_display, ::levelWarningDisplay);
  f(s.display.level_warning_lines, ::levelWarningLines);
  f(s.display.level_warning_text, ::levelWarningText);
  f(s.display.text_erase, ::textErase);
  f(s.display.display_time, ::displayTime);
  f(s.display.neat, ::neat);
  f(s.display.star_show_vga_special_code, ::starShowVGASpecialCode);
  f(s.display.star_active, ::starActive);
  f(s.display.boss_bars, ::boss_bar);
  f(s.display.port_config_change, ::portConfigChange);
  f(s.display.port_config_done, ::portConfigDone);
  f(s.map.back_pos, ::backPos);
  f(s.map.back_pos2, ::backPos2);
  f(s.map.back_pos3, ::backPos3);
  f(s.map.back_move, ::backMove);
  f(s.map.back_move2, ::backMove2);
  f(s.map.back_move3, ::backMove3);
  f(s.map.map_x, ::mapX);
  f(s.map.map_y, ::mapY);
  f(s.map.map_x2, ::mapX2);
  f(s.map.map_x3, ::mapX3);
  f(s.map.map_y2, ::mapY2);
  f(s.map.map_y3, ::mapY3);
  f(s.map.map_y_pos, ::mapYPos);
  f(s.map.map_y2_pos, ::mapY2Pos);
  f(s.map.map_y3_pos, ::mapY3Pos);
  f(s.map.map_x_pos, ::mapXPos);
  f(s.map.old_map_x_ofs, ::oldMapXOfs);
  f(s.map.map_x_ofs, ::mapXOfs);
  f(s.map.map_x2_ofs, ::mapX2Ofs);
  f(s.map.map_x2_pos, ::mapX2Pos);
  f(s.map.map_x3_pos, ::mapX3Pos);
  f(s.map.old_map_x3_ofs, ::oldMapX3Ofs);
  f(s.map.map_x3_ofs, ::mapX3Ofs);
  f(s.map.temp_map_x_ofs, ::tempMapXOfs);
  f(s.map.map_x_bp_pos, ::mapXbpPos);
  f(s.map.map_x2_bp_pos, ::mapX2bpPos);
  f(s.map.map_x3_bp_pos, ::mapX3bpPos);
  f(s.map.map1_y_delay, ::map1YDelay);
  f(s.map.map1_y_delay_max, ::map1YDelayMax);
  f(s.map.map2_y_delay, ::map2YDelay);
  f(s.map.map2_y_delay_max, ::map2YDelayMax);
  f(s.map.temp_back_move, ::tempBackMove);
  f(s.map.bk_wrap1, ::BKwrap1);
  f(s.map.bk_wrap2, ::BKwrap2);
  f(s.map.bk_wrap3, ::BKwrap3);
  f(s.map.bk_wrap1_to, ::BKwrap1to);
  f(s.map.bk_wrap2_to, ::BKwrap2to);
  f(s.map.bk_wrap3_to, ::BKwrap3to);
  f(s.map.stop_backgrounds, ::stopBackgrounds);
  f(s.map.stop_background_num, ::stopBackgroundNum);
  f(s.map.background2, ::background2);
  f(s.map.background2_not_transparent, ::background2notTransparent);
  f(s.map.background3x1, ::background3x1);
  f(s.map.background3x1b, ::background3x1b);
  f(s.map.background2_over, ::background2over);
  f(s.map.background3_over, ::background3over);
  f(s.map.sky_enemy_over_all, ::skyEnemyOverAll);
  f(s.map.top_enemy_over, ::topEnemyOver);
  f(s.level.events, ::eventRec);
  f(s.level.max_event, ::maxEvent);
  f(s.level.event_loc, ::eventLoc);
  f(s.level.cur_loc, ::curLoc);
  f(s.level.return_loc, ::returnLoc);
  f(s.level.return_active, ::returnActive);
  f(s.level.force_events, ::forceEvents);
  f(s.level.jump_section, ::jumpSection);
  f(s.level.enemies_active, ::enemiesActive);
  f(s.level.enemy_types, ::levelEnemy);
  f(s.level.enemy_types_max, ::levelEnemyMax);
  f(s.level.enemy_frequency, ::levelEnemyFrequency);
  f(s.level.enemy_continual_damage, ::enemyContinualDamage);
  f(s.level.super_enemy_254_jump, ::superEnemy254Jump);
  f(s.level.random_explosions, ::randomExplosions);
  f(s.level.small_enemy_adjust, ::smallEnemyAdjust);
  f(s.level.galaga_shot_freq, ::galagaShotFreq);
  f(s.level.galaga_life, ::galagaLife);
  f(s.level.timer, ::levelTimer);
  f(s.level.timer_countdown, ::levelTimerCountdown);
  f(s.level.timer_jump_to, ::levelTimerJumpTo);
  f(s.level.map_origin, ::mapOrigin);
  f(s.level.map_pnum, ::mapPNum);
  f(s.level.map_planet, ::mapPlanet);
  f(s.level.map_section, ::mapSection);
  f(s.level.new_pl, ::newPL);
  f(s.ending.end_level, ::endLevel);
  f(s.ending.really_end_level, ::reallyEndLevel);
  f(s.ending.wait_to_end_level, ::waitToEndLevel);
  f(s.ending.player_end_level, ::playerEndLevel);
  f(s.ending.ready_to_end_level, ::readyToEndLevel);
  f(s.ending.countdown, ::levelEnd);
  f(s.ending.fx_wait, ::levelEndFxWait);
  f(s.ending.warp, ::levelEndWarp);
  f(s.ending.first_game_over, ::firstGameOver);
  f(s.ending.all_players_gone, ::allPlayersGone);
  f(s.ending.enemy_still_exploding, ::enemyStillExploding);
  f(s.ending.stopped_demo, ::stoppedDemo);
  f(s.ending.bonus_level, ::bonusLevel);
  f(s.ending.bonus_level_current, ::bonusLevelCurrent);
  f(s.ending.normal_bonus_level_current, ::normalBonusLevelCurrent);
  f(s.ending.main_level, ::mainLevel);
  f(s.ending.next_level, ::nextLevel);
  f(s.input.button, ::button);
  f(s.input.fire_button_held, ::fireButtonHeld);
  f(s.players.slots, ::player);
  f(s.players.shot_repeat, ::shotRepeat);
  f(s.players.shot_multi_pos, ::shotMultiPos);
  f(s.players.power, ::power);
  f(s.players.last_power, ::lastPower);
  f(s.players.power_add, ::powerAdd);
  f(s.players.shield_wait, ::shieldWait);
  f(s.players.shield_cost, ::shieldT);
  f(s.players.charge_wait, ::chargeWait);
  f(s.players.charge_level, ::chargeLevel);
  f(s.players.charge_max, ::chargeMax);
  f(s.players.charge_gr, ::chargeGr);
  f(s.players.charge_gr_wait, ::chargeGrWait);
  f(s.players.damage_rate, ::damageRate);
  f(s.players.armor_ship_delay, ::armorShipDelay);
  f(s.players.two_player_linked, ::twoPlayerLinked);
  f(s.players.link_gun_direction, ::linkGunDirec);
  f(s.players.option_satellite_rotate, ::optionSatelliteRotate);
  f(s.players.option_attachment_move, ::optionAttachmentMove);
  f(s.players.option_attachment_linked, ::optionAttachmentLinked);
  f(s.players.option_attachment_return, ::optionAttachmentReturn);
  f(s.players.ship_gr, ::shipGr);
  f(s.players.ship_gr2, ::shipGr2);
  f(s.players.ship_gr_ptr, ::shipGrPtr);
  f(s.players.ship_gr2_ptr, ::shipGr2ptr);
  f(s.specials.wait, ::specialWait);
  f(s.specials.next_wait, ::nextSpecialWait);
  f(s.specials.sf_current_code, ::SFCurrentCode);
  f(s.specials.sf_executed, ::SFExecuted);
  f(s.specials.spray, ::spraySpecial);
  f(s.specials.weapon_wpn, ::specialWeaponWpn);
  f(s.specials.weapon_filter, ::specialWeaponFilter);
  f(s.specials.weapon_freq, ::specialWeaponFreq);
  f(s.specials.link_to_player, ::linkToPlayer);
  f(s.specials.flare_duration, ::flareDuration);
  f(s.specials.flare_start, ::flareStart);
  f(s.specials.flare_col_chg, ::flareColChg);
  f(s.specials.zinglon_duration, ::zinglonDuration);
  f(s.specials.astral_duration, ::astralDuration);
  f(s.enemies.slots, ::enemy);
  f(s.enemies.status, ::enemyAvail);
  f(s.enemies.on_screen, ::enemyOnScreen);
  f(s.enemies.total, ::totalEnemy);
  f(s.enemies.killed, ::enemyKilled);
  f(s.enemies.offset, ::enemyOffset);
  f(s.player_shots.slots, ::playerShotData);
  f(s.player_shots.ttl, ::shotAvail);
  f(s.enemy_shots.slots, ::enemyShot);
  f(s.enemy_shots.available, ::enemyShotAvail);
  f(s.scratch.temp2, ::temp2);
}

void read_state(sim::State& state);
void write_state(sim::State const& state);

}  // namespace tyrx::c
