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

	/// Begins a 4-column table: a label column followed by three slider columns. Call EndTable() if it returns true.
	bool BeginPageTable(const char* id);

	/// Lays an attack kind out as consecutive tables, each titled by a separator. The label column has a fixed width,
	/// so sliders line up across tables.
	class Sections
	{
	public:
		Sections() = default;
		~Sections();
		Sections(const Sections&) = delete;
		Sections& operator=(const Sections&) = delete;

		/// Starts a new table below `separator`, or continues the current one when it is null (except for the first call).
		/// Returns false if the table isn't visible, in which case the rows must be skipped.
		bool Section(const char* separator);

	private:
		bool open = false;
		int  count = 0;
	};

	/// Text that has a tooltip, which gets the hand cursor on hover like every item with a tooltip. Without a tooltip it is plain text.
	void TooltipLabel(const char* label, const char* tooltip);

	/// A column title, with an optional tooltip.
	struct HeaderLabel
	{
		const char* text = nullptr;
		const char* tooltip = nullptr;

		HeaderLabel() = default;
		HeaderLabel(const char* text, const char* tooltip = nullptr) :
			text(text), tooltip(tooltip) {}
	};

	/// A row with headers over the slider columns. Null or empty headers leave their column blank.
	void HeaderRow(HeaderLabel first, HeaderLabel second = {}, HeaderLabel third = {});
	void EmptyRow();

	/// The width that a table from `BeginPageTable` takes.
	float PageTableWidth();

	/// Leaves the next column blank.
	void EmptyColumn();

	/// How a field shows its value.
	enum class ValueFormat : std::uint8_t
	{
		kMultiplier,  // x1.25
		kPercent,     // The value is a fraction: 0.25 shows as 25%.
		kSeconds,
		kPlain
	};

	/// A slider in a column. With no `value` the column is left blank.
	struct SentenceField
	{
		const char*    id = nullptr;
		float*         value = nullptr;
		Options::Limit limit{};
		ValueFormat    format = ValueFormat::kMultiplier;
		const char*    tooltip = nullptr;
	};

	/// A row with `label` and up to three sliders, one in each slider column. `labelTooltip` says what the row is.
	void ValueRow(const char* label, const char* labelTooltip, const SentenceField& first, const SentenceField& second = {}, const SentenceField& third = {});

	/// A row with a checkbox in the label column and up to two sliders in the first two slider columns, greyed out while the box is unchecked. Put a
	/// `HeaderRow` over it to title the sliders.
	void ToggleRow(const char* label, bool& value, const char* tooltip, const SentenceField& first, const SentenceField& second = {});

	/// A little air between two groups of rows.
	void RowGap();

	/// The row that opens a page: the checkbox of the whole kind, lined up with the other checkboxes, and a button that resets the page, which takes
	/// the two last slider columns. Returns true when the button is clicked. The checkbox label may run into the first slider column.
	bool KindRow(const char* label, bool& enabled, const char* tooltip);

	/// Two rows for the skill a part scales with. The first is the "Skill Mult" header, and the second has a checkbox with "Scale with" in the label
	/// column, the skill drop-down across the next two columns and the skill multiplier in the third. With no `enabled` the part has no switch of its own.
	/// `source` null means the skill is fixed (Archery), as it is for bows and crossbows; only the magic kinds can pick. `subject` is what the skill
	/// scales, as the tooltip names it.
	void ScaleRows(const char* id, bool* enabled, const char* enabledTooltip, Options::SkillSource* source, const char* subject, float& skillMultiplier, const char* multiplierTooltip);

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

		void Render(const KindContext& kind, Sections& sections) const;
	};

	struct RangeRows
	{
		const char* minTooltip;
		const char* maxTooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const KindContext& kind, Sections& sections) const;
	};

	struct SpeedRows
	{
		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const KindContext& kind, Sections& sections) const;
	};

	struct SpreadRows
	{
		const char* tooltip;
		const char* maxReductionTooltip;
		const char* halfDurationTooltip;

		void Render(const KindContext& kind, Sections& sections) const;
	};

	struct FatigueRows
	{
		const char* tooltip;
		const char* minThresholdTooltip;
		const char* maxThresholdTooltip;
		const char* maxPenaltyTooltip;

		void Render(const KindContext& kind, Sections& sections) const;
	};

	// ---- Attack kinds ----

	/// The "Learn X" toggle with its skill multipliers, shared by every attack kind.
	struct AttackKindBase
	{
		const char* id;

		/// The checkbox that turns the whole kind on and off, e.g. "Improve Bows".
		const char* toggleLabel;

		const Options::AttackKind& kind;

		const char* accuracyTooltip;
		const char* precisionTooltip;
	};

	/// Parts are rendered below the "Learn X" row in the order they are listed.
	template <class... Parts>
	struct AttackKind : AttackKindBase, Parts...
	{};

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const std::function<void(const KindContext&, Sections&)>& renderParts);

	/// The width at which the preview is at its biggest. It doesn't grow past it with the window.
	float PreviewPaneWidth();

	/// Shows where shots of the kind land with the current settings, for a skill and distance picked in the pane itself.
	void RenderSpreadPreview(const KindContext& kind);

	/// Renders the attack kind: the aim sections (Accuracy, Precision, Retargeting), then its parts. Parts that have a group of their own (skill shots, range, speed, steadying, fatigue) start
	/// a new section with their title.
	template <class... Parts>
	void RenderAttackKind(const AttackKind<Parts...>& kind, const PageContext& ctx) {
		RenderAttackKindTable(
			kind,
			ctx,
			[&](const KindContext& context, Sections& sections) { (static_cast<const Parts&>(kind).Render(context, sections), ...); });
	}

	inline constexpr const char* kDrawSpeedGroup = "Draw Speed";
	inline constexpr const char* kChargeSpeedGroup = "Charge Speed";
	inline constexpr const char* kRangeGroup = "Range";

	/// Shared by spells, staves and scrolls; only the toggle's tooltip differs.
	constexpr FatigueRows CastFatigue(const char* tooltip) {
		return {
			tooltip,
			"How long a charge can be held before the caster tires, with no Magicka. More Magicka extends it.",
			"How long a charge can be held before the caster tires, with 100 max Magicka. More Magicka extends it further.",
			"The most extra spread that tiring can add. It also cancels any steadying bonus."
		};
	}
}
