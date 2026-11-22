#pragma once

#include "Options.h"

namespace NLA::Menu
{
	using Config = Options::Config;

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
	void RenderGeneralSectionPage();

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

	/// `whileHeld` also shows it while another item is held down, for an item that is meant to be read during a hold.
	void ShowTooltip(const char* text, bool whileHeld = false);

	/// Begins a 4-column table: a fixed-width label column followed by three slider columns. Call EndTable() if it returns true.
	bool BeginPageTable(const char* id);

	/// Begins a table, renders `rows` and ends it.
	void RenderTable(const char* id, const std::function<void()>& rows);

	/// Lays an attack kind out as consecutive tables, each titled by a separator. The label column has a fixed width,
	/// so sliders line up across tables.
	class TableSections
	{
	public:
		TableSections() = default;
		~TableSections();
		TableSections(const TableSections&) = delete;
		TableSections& operator=(const TableSections&) = delete;

		/// Starts a new table below `separator`, or continues the current one when it is null (except for the first call).
		/// Returns false if the table isn't visible, in which case the rows must be skipped.
		bool Section(const char* separator);

	private:
		bool open = false;
		int  count = 0;
	};

	/// A row with headers over the slider columns. Null or empty headers leave their column blank.
	void HeaderRow(const char* first, const char* second = nullptr, const char* third = nullptr);
	void EmptyRow();

	/// The width that a table from `BeginPageTable` takes.
	float PageTableWidth();

	/// Starts a new row with the button in the first column.
	bool ResetButton(const char* label);

	void ToggleColumn(const char* label, bool& value, const char* tooltip);
	void SliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit = Options::Limits::kSkillMultiplier, const char* format = "x%.2f");
	void SecondsSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit);
	/// A row with `label` and a drop-down to pick the skill.
	void SkillSourceRow(const char* label, Options::SkillSource& value, const char* tooltip);
	/// Shows `value` as a percentage; `limit` is in the same fractional units as the value (1.0 is 100%).
	void PercentSliderColumn(const char* id, float& value, const char* tooltip, Options::Limit limit);
	/// A row with `label`, the skill multiplier slider and the multiplier bounds (at 0 and at 100 skill) in the slider columns.
	void AimRow(const char* label, const char* id, Options::Config::Aim& aim, const char* tooltip, const char* minTooltip, const char* maxTooltip, Options::Limit limit = Options::Limits::kAccuracyFactor, const char* format = "x%.2f");
	/// Leaves the next column blank.
	void EmptyColumn();

	// ---- Attack kind parts ----
	// Each part renders its own rows.

	/// What a part needs to render a kind: the page, and the kind's options.
	struct KindContext
	{
		const PageContext&         page;
		Options::Config::Attack&   cfg;
		const Options::AttackKind& info;
	};

	struct SkillShotRows
	{
		const char* label;
		const char* tooltip;
		const char* shooter;

		/// The skill that drives the chance, as worded in tooltips (e.g. "Archery skill").
		const char* skill;

		void Render(const KindContext& kind, TableSections& sections) const;
	};

	struct RangeRows
	{
		const char* tooltip;
		const char* minTooltip;
		const char* maxTooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const KindContext& kind, TableSections& sections) const;
	};

	struct SpeedRows
	{
		const char* label;
		const char* tooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const KindContext& kind, TableSections& sections) const;
	};

	struct SpreadRows
	{
		const char* label;
		const char* tooltip;
		const char* maxReductionTooltip;
		const char* halfDurationTooltip;

		void Render(const KindContext& kind, TableSections& sections) const;
	};

	struct FatigueRows
	{
		const char* label;
		const char* tooltip;
		const char* minThresholdTooltip;
		const char* maxThresholdTooltip;
		const char* maxPenaltyTooltip;

		void Render(const KindContext& kind, TableSections& sections) const;
	};

	// ---- Attack kinds ----

	/// The "Learn X" toggle with its skill multipliers, shared by every attack kind.
	struct AttackKindBase
	{
		const char* id;
		const char* label;
		const char* tooltip;

		const Options::AttackKind& kind;

		const char* accuracyTooltip;
		const char* precisionTooltip;
		const char* resetLabel;
	};

	/// Parts are rendered below the "Learn X" row in the order they are listed.
	template <class... Parts>
	struct AttackKind : AttackKindBase, Parts...
	{};

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const char* separator, const std::function<void(const KindContext&, TableSections&)>& renderParts);

	/// The width at which the preview is at its biggest. It doesn't grow past it with the window.
	float PreviewPaneWidth();

	/// Shows where shots of the kind land with the current settings, for a skill and distance picked in the pane itself.
	void RenderSpreadPreview(const KindContext& kind);

	/// Renders the attack kind below `separator`. Parts that have a group of their own (skill source, range, speed, steadying, fatigue) start
	/// a new section with their title; the rest stay in the section of the "Learn X" row.
	template <class... Parts>
	void RenderAttackKind(const AttackKind<Parts...>& kind, const PageContext& ctx, const char* separator = "Aiming") {
		RenderAttackKindTable(
			kind,
			ctx,
			separator,
			[&](const KindContext& context, TableSections& sections) { (static_cast<const Parts&>(kind).Render(context, sections), ...); });
	}

	inline constexpr const char* kScaleAimLabel = "Scale Aim With Skill";
	inline constexpr const char* kScaleDrawSpeedLabel = "Scale Draw Speed With Skill";
	inline constexpr const char* kScaleChargeSpeedLabel = "Scale Charge Speed With Skill";
	inline constexpr const char* kSteadyingLabel = "Holding Charge Steadies Aim";
	inline constexpr const char* kDrawSpeedGroup = "Draw Speed";
	inline constexpr const char* kChargeSpeedGroup = "Charge Speed";
	inline constexpr const char* kRangeGroup = "Range";

	/// Shared by spells, staves and scrolls; only the toggle's tooltip differs.
	constexpr FatigueRows CastFatigue(const char* tooltip) {
		return {
			"Holding Charge Fatigues Aim",
			tooltip,
			"Seconds a spell/staff/scroll can be held charged before fatigue sets in for a caster with no Magicka. A higher max Magicka extends it.",
			"Seconds a spell/staff/scroll can be held charged before fatigue sets in for a caster with 100 max Magicka. Max Magicka above that extends it further, with diminishing returns.",
			"The most spread that prolonged fatigue can add. Note, that in fatigued state, the bonus from steadying is negated before adding extra spread."
		};
	}
}
