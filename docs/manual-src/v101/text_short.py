# -*- coding: utf-8 -*-
# SpaceX 1.0.1 - short manual. Only what you cannot guess. **x** = control name.

SECTIONS_INTRO = "Every section: click the name to switch it on or off, Cmd-click to solo it. The lock keeps the Smart dice away."

SECTIONS = [
    ("LCR Matrix",
     "Splits the image into a real centre and real sides - not just mid-side. Runs while **LCR** is armed in the header, "
     "which adds latency. **L/R** sets how much of the sides you keep, **C-Weight** how firmly the centre is held, "
     "**HF Regain** brings back the highs the split takes away. **EQ → LCR** puts the Sides EQ on this split."),
    ("Polarity",
     "Flips the phase of the left or right side. **PRE / POST** decides whether that happens before or after Micropitch and "
     "Mid-Side - two different sounds. The biggest single change in the plugin, so check Mono."),
    ("Micropitch",
     "Makes a mono sound wide with tiny offsets in time and pitch. The four styles look simple, but each is its own "
     "hand-tuned trick: **Velvet** gentle and close, **Halo** a soft ring that holds up in a full mix, **Illusion** wider than "
     "it is but tight, **Double** the widest with the most character. Pick one, turn **Amount**."),
    ("Mid-Side",
     "**Width** opens or narrows the image, **Sides** lifts the sides without touching the centre. The Sides EQ curves were "
     "picked by ear for real mix problems: **Tight** cleans the lows, **Clear** adds air, **Focus** gives a calmer, brighter "
     "centre. The small fader sets how strong - left is gentle."),
    ("Autopan",
     "Moves the sound between left and right. The note syncs it to the song, the box sets the length - 1/16 up to 8 bars."),
    ("Phaser",
     "Moves the image, not the tone. **Sweep**, **Shimmer**, **Spin** or **Swirl** - then **Amount**. **FAST** runs it quicker, "
     "**LINK** makes it follow the Autopan at half its pace."),
]

CONTROL = [
    ("Smart profile & Smart dice",
     "Pick **Lead Vocal**, **Backings**, **Adlibs** or **Send FX**, then roll. The dice builds a complete setting for that "
     "source and switches on only the sections that belong. Every range it can reach was tuned by ear. **Undo** takes it back."),
    ("Life & Mod",
     "**Life** is how much everything moves - one knob for all modulation. **Mod** switches the modulation off without losing it."),
    ("LCR",
     "Arms the L/C/R engine. Off, SpaceX runs with zero latency."),
    ("Output",
     "**Mono** and **Dry** to check, **Mix** to blend, **Vol** at the very end. **AG** matches the level, so bypass is a fair "
     "comparison. Right-click **Mix** or **Vol** to lock them against presets and Reset."),
    ("Starfield",
     "Your stereo image, live. Click it for the goniometer. The bar at the bottom is correlation - right of centre is mono-safe."),
    ("Presets",
     "Sorted in folders. The arrows step through the current folder, a star means you changed something, **Save** lets you "
     "pick the folder. **A/B** compares two versions."),
    ("Settings",
     "Theme, LCR On Startup, Show Modulation, Save State as Default - plus the Tour, this manual and the Back Panel."),
    ("Help",
     "Click the **?** at the bottom left and hover anything for a short hint."),
]

RULE = ("The more room already sits in front of SpaceX, the further you can push it. Last in the chain is the usual spot. "
        "It is not a mastering EQ in disguise - on the master bus only if you know exactly why.")
CREATE = ("And where there is no room, it makes some. Width, depth and a real third dimension - elegant, and mono-compatible. "
          "A mono lead becomes a convincing double that still sounds like a record. Drums, a kick, a mono sub or an 808 gain "
          "depth and weight. Space without reverb - more depth, more 3D.")
LICENCE = ("Without a serial SpaceX runs as a demo: everything works, but about every 50 seconds the sound goes quiet for a "
           "moment. To activate, click the logo and enter your name and serial - the name exactly as in your order.")
THANKS = "Thanks to Jeff Ellis, whose words for the forbidden zone and the fourth dimension gave the idea a name."
CONTACT = "Paul Misty · Mistycat Studios · info@paulmisty.com · paulmisty.com · @paulmisty.studio"
