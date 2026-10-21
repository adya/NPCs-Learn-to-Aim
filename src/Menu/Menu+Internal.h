#pragma once

#include "Options.h"
#include <functional>

namespace NLA::Menu
{
	using Config = Options::Config;
	using HeadshotSettings = Options::NPCConfig::HeadshotSettings;

	// ---- Pages ----

	struct PageContext
	{
		Config&       cfg;
		const Config& defaults;
		bool          isNPC;
	};

	using PageBody = void (*)(const PageContext&);

	/// With `masterSwitch` the page starts with the actor's Enable checkbox; otherwise it shows a notice while that is off.
	void RenderActorPage(bool isNPC, bool masterSwitch, PageBody body);

	template <bool IsNPC, PageBody Body, bool MasterSwitch = false>
	void ActorPage() {
		RenderActorPage(IsNPC, MasterSwitch, Body);
	}

	void RenderGeneralPage(const PageContext& ctx);
	void RenderBowsPage(const PageContext& ctx);
	void RenderCrossbowsPage(const PageContext& ctx);
	void RenderSpellsPage(const PageContext& ctx);
	void RenderConcentrationSpellsPage(const PageContext& ctx);
	void RenderStavesPage(const PageContext& ctx);
	void RenderConcentrationStavesPage(const PageContext& ctx);
	void RenderScrollsPage(const PageContext& ctx);
	void RenderActionsPage();

	// ---- Widgets ----

	class DisabledIf
	{
	public:
		explicit DisabledIf(bool disabled);
		~DisabledIf();
		DisabledIf(const DisabledIf&) = delete;
		DisabledIf& operator=(const DisabledIf&) = delete;

	private:
		bool disabled;
	};

	void ShowTooltip(const char* text);

	/// Begins a 3-column table with a fixed-width label column, optionally followed by the Accuracy/Precision header.
	/// Call EndTable() if it returns true.
	bool BeginPageTable(const char* id, bool hasAccuracy, bool withHeader);

	/// Begins a table with the (empty)/Accuracy/Precision header, renders `rows` and ends it.
	void RenderTable(const char* id, bool hasAccuracy, const std::function<void()>& rows);

	/// Lays an attack kind out as consecutive tables, each titled by a separator. The label column has a fixed width,
	/// so sliders line up across tables, and only the first table has the Accuracy/Precision header.
	class TableSections
	{
	public:
		explicit TableSections(bool hasAccuracy);
		~TableSections();
		TableSections(const TableSections&) = delete;
		TableSections& operator=(const TableSections&) = delete;

		/// Starts a new table below `separator`, or continues the current one when it is null (except for the first call).
		/// Returns false if the table isn't visible, in which case the rows must be skipped.
		bool Section(const char* separator);

	private:
		bool hasAccuracy;
		bool open = false;
		int  count = 0;
	};

	void HeaderRow(const char* first, const char* second);
	void EmptyRow();

	/// Starts a new row with the button in the first column.
	bool ResetButton(const char* label);

	void ToggleColumn(const char* label, bool& value, const char* tooltip);
	void SliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit = Options::Limits::kSkillMultiplier, const char* format = "x%.2f");
	void SecondsSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit);
	/// A row with `label` and a drop-down to pick the skill.
	void SkillSourceRow(const char* label, Options::SkillSource& value, const char* tooltip);
	/// Shows `value` as a percentage; `limit` is in the same fractional units as the value (1.0 is 100%).
	void PercentSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit);
	void NotApplicableColumn();

	// ---- Attack kind parts ----
	// Each part renders and resets its own rows. `aiming` is the state of the attack kind's "Learn X" toggle.

	/// Points at one of a spell type's skill sources; renders as a drop-down row inside the section it belongs to.
	struct SkillSourceRef
	{
		Config::SkillSources Config::*sources = nullptr;
		Options::SkillSource Config::SkillSources::*field = nullptr;
	};

	constexpr SkillSourceRef AimingSkill(Config::SkillSources Config::*sources) { return { sources, &Config::SkillSources::aiming }; }
	constexpr SkillSourceRef ChargeSpeedSkill(Config::SkillSources Config::*sources) { return { sources, &Config::SkillSources::chargeSpeed }; }
	constexpr SkillSourceRef RangeSkill(Config::SkillSources Config::*sources) { return { sources, &Config::SkillSources::range }; }

	struct HeadshotRows
	{
		bool HeadshotSettings::*enabled;
		float HeadshotSettings::*minChance;
		float HeadshotSettings::*maxChance;
		const char*              label;
		const char*              tooltip;
		const char*              shooter;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct RangeRows
	{
		Config::Scaling Config::*options;
		const char*                     tooltip;
		const char*                     minTooltip;
		const char*                     maxTooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		SkillSourceRef skill{};

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct SpeedRows
	{
		Config::Scaling Config::*options;
		const char*                     label;
		const char*                     tooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		SkillSourceRef skill{};

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct SpreadRows
	{
		Config::Spread Config::*options;
		const char*                    label;
		const char*                    tooltip;
		const char*                    maxReductionTooltip;
		const char*                    halfDurationTooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct FatigueRows
	{
		Config::Fatigue Config::*options;
		const char*                     label;
		const char*                     tooltip;
		const char*                     minThresholdTooltip;
		const char*                     maxThresholdTooltip;
		const char*                     maxPenaltyTooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	// ---- Attack kinds ----

	/// The "Learn X" toggle with its skill multipliers, shared by every attack kind.
	struct AttackKindBase
	{
		const char* id;
		const char* label;
		const char* tooltip;
		bool Config::*aiming;
		float Config::SkillMultiplier::*multiplier;
		const char*                     accuracyTooltip;
		const char*                     precisionTooltip;
		const char*                     resetLabel;
		SkillSourceRef                  aimSkill{};
	};

	/// Parts are rendered below the "Learn X" row in the order they are listed.
	template <class... Parts>
	struct AttackKind : AttackKindBase, Parts...
	{};

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const char* separator, const std::function<void(bool aiming, TableSections& sections)>& renderParts, const std::function<void()>& resetParts);

	/// Renders the attack kind below `separator`. Parts that have a group of their own (skill source, range, speed, steadying, fatigue) start
	/// a new section with their title; the rest stay in the section of the "Learn X" row.
	template <class... Parts>
	void RenderAttackKind(const AttackKind<Parts...>& kind, const PageContext& ctx, const char* separator = "Aiming") {
		RenderAttackKindTable(
			kind,
			ctx,
			separator,
			[&](bool aiming, TableSections& sections) { (static_cast<const Parts&>(kind).Render(ctx, aiming, sections), ...); },
			[&] { (static_cast<const Parts&>(kind).Reset(ctx), ...); });
	}

	inline constexpr const char* kScaleAimLabel = "Scale Aim With Skill";
	inline constexpr const char* kScaleDrawSpeedLabel = "Scale Draw Speed With Skill";
	inline constexpr const char* kScaleChargeSpeedLabel = "Scale Charge Speed With Skill";
	inline constexpr const char* kSteadyingLabel = "Holding Charge Steadies Aim";
	inline constexpr const char* kDrawSpeedGroup = "Draw Speed";
	inline constexpr const char* kChargeSpeedGroup = "Charge Speed";
	inline constexpr const char* kRangeGroup = "Range";

	/// Shared by spells, staves and scrolls; only the toggle's tooltip differs.
	constexpr FatigueRows CastFatigue(Config::Fatigue Config::*options, const char* tooltip) {
		return {
			options,
			"Holding Charge Fatigues Aim",
			tooltip,
			"Seconds a spell/staff/scroll can be held charged before fatigue sets in for a caster with no Magicka. A higher max Magicka extends it.",
			"Seconds a spell/staff/scroll can be held charged before fatigue sets in for a caster with 100 max Magicka. Max Magicka above that extends it further, with diminishing returns.",
			"The most spread that prolonged fatigue can add. Note, that in fatigued state, the bonus from steadying is negated before adding extra spread."
		};
	}
}
