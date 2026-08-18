# MineLearning Technical Debt

This is the current, verified list. Do not treat historical task notes as
active requirements.

## UE asset maintenance

- `BP_OreFiedManager` is an `ObjectRedirector` still referenced by an External
  Actor in `ThirdPersonMap`. Resolve it in the UE Content Browser with Fix Up
  Redirectors and save the affected map; do not delete its `.uasset` directly.
- OreBuddy's Rig32 skeleton, AnimBP, collect/deliver montages, and the Gunner
  skeleton are actively referenced. Rename or reorganize these legacy `Test`
  names only as one UE-managed migration with reference checks and redirector
  cleanup.
- Both Mannequin directories remain necessary: the UE4 set owns the UE4/UE5
  retarget chain, while the UE5 set is used by the player and mining companion.
- The remaining Tripo/UUID-named Gunner and ore assets are active mesh/material/
  texture dependencies, not duplicate assets. Rename them only as a scoped
  UE-managed asset migration; do not delete or bulk-rename them for aesthetics.

## Code boundaries

- `GunnerCharacter` still combines combat, weapon presentation, UI, and bark
  feedback. The burst gameplay duplicate path has been removed; defer a combat
  component extraction until the public Blueprint surface and weapon-data
  requirements are defined.

## Verification gap

- After native-code reflection changes, restart the UE Editor before a final
  PIE pass. Verify ore stages, OreBuddy mining/pickup/delivery, and Gunner
  single fire, burst, reload, and headshots.
