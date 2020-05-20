# JaffaQuake2
Ha, cake.

# Knowledge dump

- I want to focus on keeping this codebase as minimal and organised as possible, never let files get so big they become\
  a hassle to navigate.

- ref_gl has been updated to C++ (exceptions disabled), this allows for lots of new shortcuts.\
  I purposely haven't used any classes, templates or that sort of thing, to do so would just upset the flow\
  that currently exists. There's a place for them though... Somewhere.\
  Rewrote the windowing code, "video modes" aren't very important nowadays so I stripped that stuff entirely, will need to be\
  rewritten for fullscreen mode, which isn't supported right now. Ideally want to query the OS/GI for supported FS modes.\
  Every code file has its own header for public functions that need to be exposed, gl_local.h was becoming a huge pain to\
  navigate, so this felt like the right thing to do; it's also a precompiled header.\
  Removed QGL, implemented GLEW, GLAD didn't fit the bill.\
  Uses OpenGL 3 functions like glMipmapImage or whatever it is, didn't want to do it myself (lame).\
  Note: Quake 3 MipMap2 code doesn't support NPOT textures.
  
- Standard fare of quality of life updates, unlimited max texture resolution (HW dependent), NPOT texture support,\
  support for PNG and other formats thanks to STB Image.\
  Image substitute system, when loading any image format, ref_gl will try to load (in this order) PNG, TGA, JPG and then\
  the original format.\
  No more texture intensity, we now use a better lightmap blending mode to perfectly replicate this without\
  losing picture quality (OVERSIGHT: alias models aren't compensated for and are a bit too dark than they should be).\
  There is commented out Windows gamma code that sets the gamma ramp, this was commented out because it's horrible.\
  Not horrible code-wise, it's just you have to change the entire PC's gamma ramp, for all programs, this is NOT okay.\
  Derived from Quake 3's win_gamma.c code, it proved a nightmare for me to debug with.\
  If re-enabling, take the bit from Q3 that enables it only when in fullscreen mode, your desktop gamma will cry\
  if you don't do this. It looks REALLY nice when in effect though.\
  Shaders can replace gamma ramps entirely, but we don't have those because ref_gl is still fixed function, maybe some day.

- All Linux / Unix sys stuff has been removed, I have little knowedge on OS-specific things aside from Windows, so it\
  seemed pointless to keep them there while they just became too old to use.
  
- No makefiles yet, Visual Studio solutions only (pointless seeing as there's no sys code for anything but Windows).\
  Win64 support.
  
- I thought it'd be interesting to make the filesystem mostly OS-specific, to take advantage of file-sharing and whatnot.\
  This nets no speed benefit, or anything at all apart from another codeline to maintain. It was fun to write though :^),\
  it adds potential for OS-specific hijinks like memory mapping.\
  All usage of fopen and related functions has been replaced with FS functions. Saving and attract demo playback\
  is broken because I made a wrong assumption while converting the code to use the newer system.\
  The generic stdio filesystem has fallen behind and needs some minor support to become functional again.
  
- Windows DirectSound support uses code from Quake 3 / SW: Jedi Academy.

- Removed IPX support and upgraded to Winsock2 (This is probably a terrible idea, maybe, did it on impulse).
 
- Cleaned up a lot of old code that was left largely unmodified Since Quake 1 (mostly OS-specific stuff).

- Removed Windows joystick support, it was taking up a huge amount of room in win_input and it sucked, so it went.\
  Gain: No more dependency on winmm\
  PS: Sys_Milliseconds now uses QueryPerformanceCounter, this is weird because Quake 1 uses it, and Quake 2 uses MMTIME. hum.\
  Wonder if they did that for any specific reason. Who knows!
  
- The pmovefix branch fixes the drifting and imprecision when moving.\
  Pre-fix:  https://streamable.com/xh6ec2 \
  Post-fix: https://streamable.com/n1l2ym
  
- NOT compatible with mods, which really sucks, sorry! The import/export structs are different!\
  Could be easily made compatible again, I think I removed one function and added the non-printf logging functions.
  
- Based on Quake 2 3.21, the latest.
  
# TODO
  
  - Lots of stuff...
  
  - /external/ folder needs to point to GIT submodules rather than raw copies.
  
  - UI breaks frequently while resizing window.
  
  - FOV calcs should change depending on aspect ratio.
    Horz for 4:3, Vert for 16:9.
  
  - Quake 2 bug: weapon only sways whilst animating.
  
  - The ref_gl window is a huge pain.
    
  - The input code in the engine that handles placing the mouse cursor (search "cursor") is very annoying.
   
