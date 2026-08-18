# MineLearning Art Pipeline

`AGENTS.md` is the project entry point for art work. It selects the shared
`mining-game-3d-asset-production` skill for Blender, materials, props, and UE
art assets.

## Source of truth

Each new or revised asset needs one current Asset Brief. Create it from
`.agents/skills/mining-game-3d-asset-production/assets/asset-brief-template.md`
and keep only approved, current instructions in `ArtBriefs/`.

Do not use historical blockout prompts as production briefs. The old
`SM_Ore_Iron_01` blockout brief has been removed because the project now uses a
runtime mining-stage implementation rather than an initial A/B/C blockout task.

## Workflow

1. Read `AGENTS.md`, the shared art skill, and the current Asset Brief.
2. Preserve an editable rollback point and complete the required blockout.
3. Pass the brief's acceptance checks before moving to refinement.
4. For UE assets, verify references in the Editor before moving, renaming, or
   deleting; then fix redirectors and save affected assets and maps.

## Ore stage rule

`UOreDefinitionDataAsset::BreakThresholds` is the gameplay authority for ore
stage progression and stage drops. `UOreVisualComponent` is presentation only:
its valid mesh entries are ordered by mining-stage index and must not define a
second health-ratio rule.
