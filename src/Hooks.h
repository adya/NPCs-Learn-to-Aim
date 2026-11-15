#pragma once

namespace NLA
{
	void Install();

	/// Subscribes to the game events the hooks need. Call once the game data is loaded.
	void ListenForEvents();

	/// Drops all per-actor state tracked by the hooks, e.g. when a different save is loaded.
	void Reset();
}
