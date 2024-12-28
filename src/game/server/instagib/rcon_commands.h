// This file can be included several times.

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

CONSOLE_COMMAND("hammer", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConHammer, this, "Gives a hammer to you")
CONSOLE_COMMAND("gun", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConGun, this, "Gives a gun to you")
CONSOLE_COMMAND("unhammer", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConUnHammer, this, "Removes a hammer from you")
CONSOLE_COMMAND("ungun", "", CFGFLAG_SERVER | CMDFLAG_TEST, ConUnGun, this, "Removes a gun from you")
CONSOLE_COMMAND("godmode", "?v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConGodmode, this, "Removes damage")

CONSOLE_COMMAND("force_ready", "?v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConForceReady, this, "Sets a player to ready (when the game is paused)")

CONSOLE_COMMAND("shuffle_teams", "", CFGFLAG_SERVER, ConShuffleTeams, this, "Shuffle the current teams")
CONSOLE_COMMAND("swap_teams", "", CFGFLAG_SERVER, ConSwapTeams, this, "Swap the current teams")
CONSOLE_COMMAND("swap_teams_random", "", CFGFLAG_SERVER, ConSwapTeamsRandom, this, "Swap the current teams or not (random chance)")

CONSOLE_COMMAND("add_map_to_pool", "s[map name]", CFGFLAG_SERVER, ConAddMapToPool, this, "Can be picked by random_map_from_pool command (entries can be duplicated to increase chance)")
CONSOLE_COMMAND("clear_map_pool", "", CFGFLAG_SERVER, ConClearMapPool, this, "Clears pool used by random_map_from_pool command")
CONSOLE_COMMAND("random_map_from_pool", "", CFGFLAG_SERVER, ConRandomMapFromPool, this, "Changes to random map from pool (see add_map_to_pool)")
CONSOLE_COMMAND("create_sound", "i[id] i[sound]", CFGFLAG_SERVER, ConCreateSound, this, "Create sound in user")
CONSOLE_COMMAND("laser_text", "r[message]", CFGFLAG_SERVER, ConLaserText, this, "Write text on the laser");
#undef CONSOLE_COMMAND
