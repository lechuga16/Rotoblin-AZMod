/*
 * ============================================================================
 *
 *  Rotoblin
 *
 *  File:			clientindexes.inc
 *  Type:			Helper
 *  Description:	Keep tracks of teammembers on each team.
 * 	Credits:		SRSMod team for the original source for L4D2
 *					(http://code.google.com/p/srsmod/).
 *
 *  Copyright (C) 2012-2015  raziEiL <war4291@mail.ru>
 *  Copyright (C) 2010  Mr. Zero <mrzerodk@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 */

// Don't let the script be included more than once.
#if defined _helper_clientindexes
  #endinput
#endif
#define _helper_clientindexes

// --------------------
//       Public
// --------------------

new		SurvivorIndex[MAXPLAYERS+1], InfectedIndex[MAXPLAYERS+1], SpectateIndex[MAXPLAYERS+1], DeadSurvivorIndex[MAXPLAYERS+1],
		SpectateCount, SurvivorCount, InfectedCount, DeadSurvivorCount,
		bool:g_bBlackSpot = true;

// --------------------
//       Private
// --------------------

static	const	Float:	REBUILD_DELAY				= 0.3;

static					g_iDebugChannel				= 0;
static	const	String:	DEBUG_CHANNEL_NAME[]		= "ClientIndexes";

// **********************************************
//                   Forwards
// **********************************************

/**
 * Plugin is starting.
 *
 * @noreturn
 */
_H_ClientIndexes_OnPluginStart()
{
	g_iDebugChannel = DebugAddChannel(DEBUG_CHANNEL_NAME);
	DebugPrintToAllEx("Module is now setup");
}

/**
 * Plugin is now enabled.
 *
 * @noreturn
 */
_CI_OnPluginEnabled()
{
	RebuildIndex();
	HookEvent("round_start",				_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	HookEvent("round_end",				_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	HookEvent("player_team",				_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	HookEvent("player_spawn",				_CI_Event, 						EventHookMode_PostNoCopy);
	HookEvent("player_disconnect",		_CI_Event, 						EventHookMode_PostNoCopy);
	HookEvent("entity_killed",			_CI_Event, 						EventHookMode_PostNoCopy);
	HookEvent("player_bot_replace",		_CI_Event, 						EventHookMode_PostNoCopy);
	HookEvent("bot_player_replace",		_CI_Event, 						EventHookMode_PostNoCopy);
	DebugPrintToAllEx("Module is now loaded");
}

/**
 * Plugin is now disabled.
 *
 * @noreturn
 */
_CI_OnPluginDisabled()
{
	UnhookEvent("round_start", 			_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	UnhookEvent("round_end", 			_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	UnhookEvent("player_team", 			_CI_Event_TempStop_Event, 	EventHookMode_PostNoCopy);
	UnhookEvent("player_spawn", 			_CI_Event, 						EventHookMode_PostNoCopy);
	UnhookEvent("player_disconnect", 	_CI_Event, 						EventHookMode_PostNoCopy);
	UnhookEvent("entity_killed", 		_CI_Event, 						EventHookMode_PostNoCopy);
	UnhookEvent("player_bot_replace",	_CI_Event, 						EventHookMode_PostNoCopy);
	UnhookEvent("bot_player_replace",	_CI_Event, 						EventHookMode_PostNoCopy);
	ResetCounts();
	DebugPrintToAllEx("Module is now unloaded");
}

/**
 * On map start.
 *
 * @noreturn
 */
_CI_OnMapStart()
{
	ResetCounts();
	DebugPrintToAllEx("Map start, resetting indexes");
}

/**
 * On map end.
 *
 * @noreturn
 */
_CI_OnMapEnd()
{
	ResetCounts();
	DebugPrintToAllEx("Map end, resetting indexes");
}

/**
 * Called when a event that invalidates the index, is fired.
 *
 * @param event			INVALID_HANDLE due to EventHookMode_PostNoCopy.
 * @param name			String containing the name of the event.
 * @param dontBroadcast	True if event was not broadcast to clients, false otherwise.
 * @noreturn
 */
public _CI_Event_TempStop_Event(Handle:event, const String:name[], bool:dontBroadcast)
{
	DebugPrintToAllEx("Event \"%s\" was fired, rebuild indexes in %f secs", name, REBUILD_DELAY);
	ResetCounts();
	CreateTimer(REBUILD_DELAY, _CI_RebuildIndex_Timer);
}

/**
 * Called when a event that invalidates the index, is fired.
 *
 * @param event			INVALID_HANDLE due to EventHookMode_PostNoCopy.
 * @param name			String containing the name of the event.
 * @param dontBroadcast	True if event was not broadcast to clients, false otherwise.
 * @noreturn
 */
public _CI_Event(Handle:event, const String:name[], bool:dontBroadcast)
{
	DebugPrintToAllEx("Event \"%s\" was fired, rebuild indexes", name);
	RebuildIndex();
}

/**
 * Called when rebuild index interval has elapsed.
 *
 * @param timer			Handle to the timer object.
 * @noreturn
 */
public Action:_CI_RebuildIndex_Timer(Handle:timer)
{
	DebugPrintToAllEx("Rebuild index timer executed, rebuilding indexes");
	RebuildIndex();
}

// **********************************************
//                 Private API
// **********************************************

/**
 * Rebuilds client indexes.
 *
 * @noreturn
 */
static RebuildIndex()
{
	ResetCounts();

	if (!IsServerProcessing()) return;

	for (new i = FIRST_CLIENT; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i)) continue;

		switch (GetClientTeam(i))
		{
			case TEAM_SURVIVOR:
			{
				if (!IsPlayerAlive(i))
					DeadSurvivorIndex[DeadSurvivorCount++] = i;
				else
					SurvivorIndex[SurvivorCount++] = i;
			}
			case TEAM_INFECTED:
			{
				InfectedIndex[InfectedCount++] = i;
			}
			case TEAM_SPECTATOR:
			{
				SpectateIndex[SpectateCount++] = i;
			}
		}
	}
	g_bBlackSpot = false;
	DebugPrintToAllEx("Indexes is now rebuild. Survivor count: %i, Infected count: %i, Spectate count: %i", SurvivorCount, InfectedCount, SpectateCount);
}

/**
 * Reset all player counters.
 *
 * @noreturn
 */
static ResetCounts()
{
	g_bBlackSpot = true;

	SurvivorCount = 0;
	InfectedCount = 0;
	SpectateCount = 0;
	DeadSurvivorCount = 0;
}

/**
 * Wrapper for printing a debug message without having to define channel index
 * everytime.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
static DebugPrintToAllEx(const String:format[], any:...)
{
	decl String:buffer[DEBUG_MESSAGE_LENGTH];
	VFormat(buffer, sizeof(buffer), format, 2);
	DebugPrintToAll(g_iDebugChannel, buffer);
}