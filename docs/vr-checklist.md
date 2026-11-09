# VR mod feature checklist

Status: [x] done and confirmed in the headset, [~] built but not yet confirmed, [ ] not started.

## 1. Core display
- [x] OpenXR session on the game's Direct3D 12 device (SteamVR, Index)
- [x] Headset paces the game instead of the 60 fps cap
- [x] Menus, movies and cut scenes on a floating screen
- [x] On-foot world view at 100 degrees, filling the headset
- [~] True stereo: every frame drawn once per eye, with the headset's real eye spacing
- [ ] World scale tuned so objects and rooms feel life-size (setting `vr_units`, guessed at 32 per metre)
- [ ] Driving missions and mounted guns in world view with stereo (flat screen today)
- [ ] Clean hand-over between the three programs (intro, on foot, driving) with the headset on
- [ ] Desktop mirror window showing one clean eye for streaming and recording

## 2. Head tracking
- [x] Head yaw turns the view; stick and mouse still turn the body
- [x] Head pitch looks up and down
- [ ] Head roll and leaning (positional 6DOF) drawn by the game, not only by reprojection
- [ ] Player height: standing and seated modes, with a height offset
- [ ] Recenter button
- [ ] Wider culling so scenery does not pop at the edges when the head leads the camera

## 3. Controllers
- [~] Index controllers as the gamepad: move, turn, fire, aim, jump, crouch, reload, gadgets, pause
- [ ] Snap turn and smooth turn, with speed and angle options
- [ ] Movement direction follows the head or the left hand (option)
- [ ] Left-handed mode
- [ ] Controller vibration from the game's rumble
- [ ] Bindings editable in SteamVR, and shown correctly in the game's button prompts
- [ ] Oculus Touch and other controller profiles tested

## 4. Hands and weapons
- [ ] Gun aims with the right hand instead of the head
- [ ] Gun model follows the hand; crosshair or laser dot shows where it points
- [ ] Two-handed steadying for rifles
- [ ] Scopes: zoomed view inside the scope, not across the whole view
- [ ] Gadgets (decryptor, grapple, laser) aimed and used by hand
- [ ] Bond's hands or body visible
- [ ] Manual reload gestures (stretch goal)

## 5. HUD and menus in VR
- [ ] HUD at a comfortable depth instead of infinity, with size and distance options
- [ ] HUD pieces on a wrist or follow-the-head panel (option)
- [ ] Menus operated by pointing a controller at the floating screen
- [ ] Subtitles and mission text readable in the headset
- [ ] Floating screen: distance, size and curve options; stays put or follows the head
- [ ] Loading screens and fades that do not flash or freeze in the headset

## 6. VR options menu (in the game's RECOMP tab and the F1 window)
- [~] VR on/off
- [ ] Render resolution per eye (percent of the headset's recommended size)
- [ ] World scale
- [ ] Turn mode, snap angle, smooth speed
- [ ] Dominant hand
- [ ] Seated or standing, height offset
- [ ] HUD distance and scale
- [ ] Comfort vignette when moving or turning
- [ ] Cut scenes: floating screen or immersive (head-free camera)
- [ ] Aim mode: head, hand, or hand with aim assist
- [ ] Settings usable from inside the headset, not only on the desktop

## 7. Comfort
- [ ] Vignette during locomotion
- [ ] No forced camera motion in the headset: cut scenes, deaths and scripted turns handled gently
- [ ] Camera shake and hit flash reduced or off in VR
- [ ] Stable horizon in vehicles

## 8. Performance and quality
- [ ] Per-eye render size matched to the headset instead of the desktop's 2160 lines
- [ ] Hold the headset's refresh rate (90, 120, 144 Hz) with two eyes drawn
- [ ] MSAA and sharpening working per eye
- [ ] Effects correct in both eyes: shadows, glows, water, sky, particles, mirrors
- [ ] Draw each eye's view without doubling CPU work where possible

## 9. Robustness and release
- [ ] Starts flat when no headset or runtime is present, with a clear log line
- [ ] Headset put on or taken off, SteamVR restarted, mid-game
- [ ] Works from the player launcher, loader DLL included in the package
- [ ] PAL disc and French and German builds tested
- [ ] Linux harness still builds (the VR module is Windows only)
- [ ] README section and a short setup guide
- [ ] Offer the work upstream to the recomp and SDK authors
