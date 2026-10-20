#pragma once

namespace NLA
{
	void Install();

	/// Drops all per-actor state tracked by the hooks, e.g. when a different save is loaded.
	void Reset();
}
