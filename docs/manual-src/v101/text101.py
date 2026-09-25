# -*- coding: utf-8 -*-
# SpaceX 1.0.1 - manual text (EN). The UI is the truth, not this file.

VERSION = "1.0.1"

INTRO_LEAD = ("Depth, fullness and dimension - without a single millisecond of reverb tail. "
              "No wash, no mud, nothing you have to EQ back out afterwards.")

INTRO = [
    "SpaceX gives a sound its own place in a mix that is already full. Wider, deeper, more "
    "three-dimensional - and still standing exactly where the song needs it.",
    "On the surface it is six sections, the Smart dice and a handful of knobs. Underneath it listens to the "
    "centre, the sides and everything in between, and keeps every move inside the range where it "
    "still sounds like a record. You get to the result fast. You do not get lost on the way.",
    "Most stereo tools are built to be safe. This one is built to be interesting - and then made "
    "safe by ear, one decision at a time.",
]

COVER_PILLARS = [
    ("Fast", "One click to a finished sound."),
    ("Light", "Low CPU. Zero latency without LCR."),
    ("Clean", "A real L / C / R split, not just mid-side."),
    ("Safe", "Hand-tuned. Nothing you have to undo."),
]
PILLARS = [
    ("Fast", "One click to a finished sound. Smart profiles for lead vocals, backings, adlibs and send FX."),
    ("Light", "Low CPU. Zero latency - unless you arm the LCR engine. Then you pay a little for the real thing."),
    ("Clean", "A next-generation L / C / R split instead of plain mid-side. Centre, left and right, for real."),
    ("Safe", "Hand-tuned and interlocked. Everything the dice can reach was tuned by ear, so it will not hand you a result you have to undo."),
]

BIO = [
    "I have been mixing professionally since 2019 - producer, mix and mastering engineer, on records "
    "and for television, among them The Voice. Alongside that I rebuild and optimise recording and "
    "mixing templates for other engineers, Jeff Ellis and Mosty among them. Sitting inside someone "
    "else's session is the fastest way to see where the real decisions are.",
    "Two terms from Jeff Ellis' Mixer Brain course stuck with me, because they put a name on something "
    "I had been doing for years: the forbidden zone and the fourth dimension. When every frequency in "
    "a mix is taken, width and depth are the only room left. SpaceX lives in that room.",
]

JOBS = [
    ("L / C / R extraction", "a dedicated centre-separation plugin"),
    ("Micro pitch and Haas doubling", "a pitch / delay widener"),
    ("Polarity, side gain, width, mid-side", "a utility plugin"),
    ("EQ on the sides only", "a mid-side EQ"),
    ("Autopan and stereo phaser", "a modulation plugin"),
    ("Goniometer and correlation", "a metering plugin"),
]
JOBS_NOTE = ("I own every one of those tools. Together they cost well over $500 and eat five slots. "
             "SpaceX does it in one window, with one preset and one latency.")

WHERE = [
    ("Lead vocals", "Width around a centre that does not move."),
    ("Backings", "Stacks, busses and doubles - wide, but tidy."),
    ("Adlibs", "Space, movement, clear sides."),
    ("Send FX", "Reverb and delay returns. Pure effect, nothing dry."),
    ("Drums", "Kick realism, hats to the sides, room without reverb."),
    ("Music", "Keys, pads, synths, whole music busses."),
    ("Bass", "Fatness and stereo - with a low end that stays put."),
]
CREATE_TITLE = "And where there is no room, it makes some"
CREATE = ("Width, depth and a real third dimension - elegant, and mono-compatible. That part matters. "
          "A mono lead becomes a convincing double that still sounds like a record. Drums, a kick, a mono sub "
          "or an 808 gain depth and weight. Space without reverb - more depth, more 3D.")
WHERE_RULE = ("Rule of thumb: the more room already sits in front of SpaceX, the further you can push it. "
              "Last in the chain is the usual spot. It is not a mastering EQ in disguise - on the master "
              "bus only if you know exactly why.")

QUICK = [
    "Put it on a backing bus, an adlib or an effect return.",
    "Click the profile until it names your source - Lead Vocal, Backings, Adlibs or Send FX.",
    "Hit the Smart dice. Listen. Hit it again, until something surprises you.",
    "Too much? Pull Mix down. Still too much? Click a section name to switch it off.",
    "Check Mono in the output. If it falls apart, bring Width down a little.",
]

TOUR = [
    ("Logo", "Click it for the back panel - who built this, which copy it is."),
    ("Smart profile", "What you are working on. Guides the dice."),
    ("Power · Smart dice · Life · Mod", "Bypass, roll, how much it moves, modulation on / off."),
    ("LCR", "Arms the L / C / R engine."),
    ("Presets", "Arrows, list, save."),
    ("Undo · A / B", "Undo / Redo, compare two settings, copy across."),
    ("Settings · Reset", "Themes, behaviour, back to Default."),
    ("Info line", "Your Smart profile in one sentence."),
    ("Starfield", "Goniometer and correlation - it reacts to everything."),
    ("Output", "Meters, Mono, Dry, Mix, Pan, Vol, AG."),
    ("Sections", "Six stages, top-left to bottom-right."),
]

PROFILES = [
    ("No Profile", "The dice may reach for anything."),
    ("Lead Vocal", "Wide, centre stays put."),
    ("Backings", "Stacks, busses and mono doubles - wide, but tidy."),
    ("Adlibs", "Space, movement, clear sides."),
    ("Send FX", "Reverb and delay returns - pure effect, nothing dry."),
]

HEADER = [
    ("Power", "Bypass for the whole plugin. While bypassed it is the only thing that glows - and it breathes, so you never forget."),
    ("Smart dice", "Rolls a complete setting and decides which sections belong in it. The face shows how many sections are active."),
    ("Life", "How much everything moves. One knob for every modulation depth. At zero, nothing moves."),
    ("Mod", "All modulation off - without losing the depths. Switch it back on and it is all still there."),
    ("LCR", "Arms the L / C / R engine. Needs an analysis window, so it adds latency while it runs."),
]

SMART_NOTE = ("This is not the randomise button other plugins have. Everything the dice can reach was tuned "
              "by ear, and whatever did not work never made it in. Sections you lock with the padlock are "
              "left alone. While a section is soloed the dice waits.")

CHAIN = ["IN", "LCR MATRIX", "POLARITY · PRE", "MICROPITCH", "MID-SIDE + SIDES EQ",
         "POLARITY · POST", "AUTOPAN", "PHASER", "MIX · PAN · VOL", "OUT"]
CHAIN_NOTE = ("Left to right, top to bottom - the way you read it is the way the audio travels. Latency is zero "
              "unless the LCR engine is armed. It needs 4096 samples to find the centre - about 85 ms at 48 kHz, "
              "reported to your DAW and compensated.")

COMMON = [
    ("Click the name", "section on / off. An off section goes dark."),
    ("Cmd-click the name", "solo - hear this section on its own."),
    ("Cmd + Shift-click", "reset this section to its defaults."),
    ("Padlock", "the dice leaves this section alone."),
    ("Double-click a knob", "back to its default."),
]

MS_VS_LCR = ("Mid-side is not L / C / R. Mid-side splits a stereo signal into sum and difference - everything "
             "that is not identical on both sides lands in the sides, including half of what you hear as centre. "
             "The LCR Matrix finds what really sits in the middle and keeps hard left and hard right as their own "
             "parts. That is why it can open a stack without touching the lead.")

LCR = [
    ("L / R", "level of the left and right parts. All the way down leaves the centre only."),
    ("C-Weight", "how strongly the centre is separated from the sides."),
    ("HF Regain", "brings back the highs the split takes away. 0 = off."),
    ("EQ → LCR", "the Sides EQ works on centre and sides of the LCR Matrix instead of mid and side."),
]
LCR_TIP = "On a backing stack, push L / R up. The lead keeps the centre, the backings open up around it - without getting louder."

POL = [
    ("ØL · ØR", "flip the phase of the left or the right channel. Cmd-click: only that one."),
    ("Link", "switch both together."),
    ("PRE / POST", "flip before Micropitch and Mid-Side, or after them. Two different sounds - try both."),
]
POL_WARN = ("A flip almost always costs mono. On a throw or an effect return that is fine - nobody expects those to "
            "survive a mono sum. On anything that carries the song, check Mono first.")

MP_TEXT = "Makes a mono sound wide with tiny offsets in time and pitch. One knob, one decision: pick a style, set Amount."
MP = [
    ("Amount", "how much of the style - from off to full."),
    ("Velvet", "the gentlest. Soft and close, the centre barely moves."),
    ("Halo", "a soft ring around the sound. Stays centred, holds up in a full mix."),
    ("Illusion", "sounds wider than it is, but stays tight."),
    ("Double", "the widest of the four, with the strongest character."),
]

MS_TEXT = "Width, side level and an EQ that only touches the sides. Reach for it when it is nearly right and just needs to open up."
MS = [
    ("Width", "how far the image reaches. Below 100 % narrower, above wider."),
    ("Sides", "level of the sides. The centre stays exactly as it is."),
    ("Sides EQ", "Flat, Tight (cleans the lows), Clear (clean plus air), Focus (calmer, brighter centre)."),
    ("EQ amount", "the small fader in the header. Left is gentle and the default, right is the strongest."),
]
MS_TIP = "Tight on anything with low end. The sides stay open, the bass stays in the middle where mono needs it."

AP_TEXT = "Slow automatic movement through the stereo field. Not a tremolo - the level stays put, only the position moves."
AP = [
    ("Amount", "how far the sound travels left and right."),
    ("Shape", "sine, or a smoother pulse-like movement."),
    ("Speed", "how fast it moves. Locked while synced."),
    ("Sync", "the note: lock the speed to the song tempo."),
    ("Rate", "note length of one movement - 1/16 up to 8 bars."),
]

PH_TEXT = "A phaser that moves the image rather than the tone. Subtle on purpose - movement, not an effect you could name."
PH = [
    ("Amount", "how strong it is. At zero it is silent."),
    ("Sweep", "the classic slow sweep, low and wide."),
    ("Shimmer", "high and quick - air rather than body."),
    ("Spin", "low, fast and thick. The obvious one."),
    ("Swirl", "very slow and broad. You notice it after a while."),
    ("FAST", "runs the character a bit quicker."),
    ("LINK", "follows Autopan at half its speed."),
]

OUT = [
    ("In / Out", "input and output level."),
    ("Mono", "listen to the result in mono."),
    ("Dry", "while in mono, compare with the unprocessed input."),
    ("Mix", "blend original and processed. Right-click locks it against presets, A / B, Reset and the dice."),
    ("Pan", "balance at the very end."),
    ("Vol", "output, plus or minus 6 dB. Right-click locks it against presets and Reset."),
    ("AG", "auto gain - matches output to input, so bypass is an honest comparison."),
]

STAR = [
    ("Click", "scope on / off - a classic goniometer. A vertical line is mono, a circle is wide."),
    ("Cmd-click the centre", "change the trace colour."),
    ("Shift-click", "change the look."),
    ("Gear", "display settings for the starfield."),
    ("The bar", "correlation. Right of centre is mono-safe, left of it cancels."),
]
STAR_NOTE = ("It is not decoration only. The scene reacts to what the plugin is actually doing - the planets, the "
             "lines, the way the field leans with the movement.")

HELP = ("The ? at the bottom left switches the info line on. Hover anything and it tells you what it is - the name in "
        "your theme colour, and only the commands you could not guess, lightly highlighted.")

PRESET_FOLDERS = [
    ("Lead Vocal", ["Exciting Vocals", "Softer Than Before", "Double My Lead", "Modern Bridge Lead"]),
    ("Backings", ["How Far Can You Go", "Prestine Backings", "Modern Phone", "Sweet & Soft BGVs",
                  "Exciting BGV Stacks", "Wide But Clean BGVs"]),
    ("Adlibs", ["Orbital Sway", "Illusion", "1_4 Adlib Sauce", "Adlib Drift"]),
    ("Send FX", ["Forbidden Echo Zone", "Basic Echo Spreader", "4th Dimension Echoes", "Modern Vocal Reverb",
                 "Yes Still Mono Save", "Moving Reverb", "Tight Reverb"]),
    ("Vocals", ["Clear Vocal Bus"]),
    ("Drums", ["Kick Roundness", "Kick Realism", "Hihats De-Harsher", "Hihats To The Sides",
               "Room Without Reverb", "Hard But Soft", "Drum Bus Depth"]),
    ("Bass", ["Synth Bass Fatness", "Deep & Tight Bass", "Mono 2 Stereo Bass"]),
    ("Music", ["Super Wide Bus", "Slow Keys & Pads", "Future Synths"]),
]

PRESETS = [
    ("‹ ›", "step through the presets of the current folder."),
    ("Name", "opens the list. The folder of the loaded preset opens by itself."),
    ("✱ after the name", "you changed something since loading."),
    ("Save", "name it, pick a folder - or New Folder right in the list. Open Preset Folder takes you there in Finder."),
    ("In the list", "Rename, Delete, Preset Folder - and Default, the clean start."),
    ("A / B", "two settings side by side. The arrow copies the active side to the other one."),
    ("Undo / Redo", "works on everything, the dice included."),
    ("Reset", "back to the Default preset."),
]
PRESET_NOTE = ("Factory presets are copied into Documents / SpaceX / Presets. SpaceX never overwrites a file that is "
               "already there - your edits stay yours.")

SETTINGS = [
    ("Theme", "Day & Night, Fairy Tale, Science Fiction. None of it changes the sound."),
    ("LCR On Startup", "the engine is armed when the plugin opens - latency from the start."),
    ("Show Modulation", "the moving dots on the knobs that show what the modulation is doing."),
    ("Save State as Default", "every new instance starts with the settings you have right now."),
    ("Take the Tour", "a short guided walk through the plugin."),
    ("Manual", "opens this manual."),
    ("Back Panel", "about this copy - and where your serial goes."),
]
LABELS = [
    ("LCR Matrix", "Galaxy"), ("Polarity", "Eclipse"), ("Micropitch", "Parallax"),
    ("Mid-Side", "Dimension"), ("Autopan", "Hyperdrive"), ("Phaser", "Raye"),
]

RECIPES = [
    ("Backing stack that will not fit", "Backings / Wide But Clean BGVs",
     "Arm LCR, push L / R up, Width around 140 %, Sides EQ on Focus. The lead keeps the centre, the stack "
     "takes everything around it."),
    ("Delay throw with an identity", "Send FX / Forbidden Echo Zone",
     "Micropitch on Illusion, Amount around 60 %, Phaser on Sweep at a third. The throw stops sounding like a "
     "delay and starts sounding like a decision."),
    ("Reverb return with depth, no mud", "Send FX / Tight Reverb",
     "Width around 130 %, Sides a touch up, Sides EQ on Tight. The reverb moves behind the dry signal instead "
     "of sitting on top of it."),
    ("Adlibs that appear and disappear", "Adlibs / Adlib Drift",
     "Autopan Amount around 40 %, Sync on, 4 bars. Slow enough that nobody notices the movement, fast enough "
     "that the part never sits still."),
    ("Bass with width and a solid middle", "Bass / Mono 2 Stereo Bass",
     "Micropitch on Velvet, a little Amount, Sides EQ on Tight. Stereo on top, mono where the club system needs it."),
    ("The forbidden one", "your throw track",
     "ØL on, PRE, Mix around 60 %. Mono will not survive it. On a throw in a dense chorus it is the widest thing "
     "you own."),
]

SHORT = [
    ("Double-click a knob", "back to default"),
    ("Click a section name", "section on / off"),
    ("Cmd-click a section name", "solo"),
    ("Cmd + Shift-click a section name", "reset that section"),
    ("Click a style · Cmd-click", "next · previous. The dots jump straight there"),
    ("Cmd-click ØL / ØR", "flip only that side"),
    ("Right-click Mix / Vol", "lock against presets and Reset"),
    ("Click the starfield", "scope on / off"),
    ("Cmd-click · Shift-click starfield", "trace colour · look"),
    ("Click the logo", "back panel"),
    ("?", "info line on / off"),
]

DEMO = [
    "Without a serial SpaceX runs as a demo. Everything works, but about every 50 seconds the sound goes quiet "
    "for a moment. The DEMO badge next to the logo lights up while that happens - never a mystery.",
    "Activation asks for your name and your serial. The two belong together: type the name exactly as it "
    "appears in your order - upper and lower case and spaces do not matter. Your name then sits on the back "
    "panel under Registered to.",
]

CREDITS = [
    "SpaceX started as a way to stop opening five plugins for one idea. Nothing in it happened by accident: "
    "the order of the chain, where the polarity flip may sit, how far each section is allowed to go. Every one of "
    "those decisions was made by listening - and then made again.",
    "Thanks to Jeff Ellis, whose words for the forbidden zone and the fourth dimension gave the idea a name. And "
    "to everyone who tested SpaceX and sent feedback - a lot of what is good in the details came from you.",
]
CONTACT = "Paul Misty · Mistycat Studios · info@paulmisty.com · paulmisty.com · @paulmisty.studio"
