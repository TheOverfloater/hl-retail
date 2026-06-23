# hl-retail
 Half-Life : Retail was a project to add back the features of the original Half-Life as it was at the time of the first retail release. Over time it evolved into something a bit different, and adds new features like per-vertex lighting on models and stencil shadows. Per-vertex lighting is added by comparing the lights.rad file used by hlrad to the geometry and texture info of the BSP file. This mod also adds stencil shadows without the need for a hacked DLL or any engine function hooks. For more on this, check svd_render.cpp.

The total list of changes includes:
 - Per-vertex lighting on models with enhanced brightness(overbright), coming from dynamic lights, map lights and muzzleflashes.
 - Stencil shadows for models which can come from a fixed light angle, or from a light in the map itself.
 - Specific per-vertex lighting calculations that take the emitter plane into account(eg. A vertex behind the light surface plane won't be lit.)
 - MP3 player by Killah ported from SOHL and modified to allow restoring a music track at the precise timestamp it was playing at the time of saving.
 - Modify weapon shots and beam/energy weapons to not impact the skybox when firing, thus giving an illusion of infinite distance.
 - Fix env_sound to be robust and reliable at setting environmenta/room sound effects on the player touching it.
 - Now all projectile weapons emit tracers when shot for that extra satisfaction.
 - The old view roll and view model bob are restored from the retail version.
 - Client weapon prediction has been stripped out to remove bugs with weapon animations.
 - Stencil shadow code supports non-hacky stencil buffers using FBOs on modern computers. Old PCs will need the hacked opengl32.dll found in the source code.
 - Grenades, rockets, etc will disappear when hitting sky brushes instead of colliding/exploding.
 - Like in the retail version, NPCs emit an audible flesh impact sound when shot at.
 - Momentary doors(doors whose movement is controlled by a rotating lever/button) now won't continue playing sounds after the player releases the lever.
 - The crowbar had it's idle animation code fixed, so that idle animations play properly now.
