# -*- coding: utf-8 -*-
# Handbuchtexte EN. Stand: SpaceX 1.0 (Mistycat).
# Wer hier etwas aendert: die Oberflaeche ist die Wahrheit, nicht dieser Text.
EN = {}
def t(k, v): EN[k] = v

t('brand', 'MISTYCAT')
t('title_sub', 'SPATIAL INTELLIGENCE')
t('title_sub2', 'STEREO IMAGING, TUNED BY EAR')
t('manual', 'Manual')
t('cover_meta', 'Version 1.0 &middot; VST3 &middot; macOS')

t('h_what', 'What this is for')
t('p_what1',
  "SpaceX is not a corrective tool. It is a way to give a sound its own layer in a mix that is "
  "already full.")
t('p_what2',
  "Most stereo plugins are built to be safe. This one is built to be interesting. It moves things "
  "sideways, forwards, backwards and slightly out of phase - the area most engineers stay out of, "
  "either because nobody ever showed them how far it can be taken, or because they are afraid of "
  "what it does to mono. Used deliberately, that is exactly where the space is.")
t('p_what2b',
  "That comes out of the work. I have been mixing professionally since 2019 as a producer, mix and "
  "mastering engineer, on records and for television, among them The Voice. Alongside that I have "
  "been hired to rebuild and optimise recording and mixing templates for other engineers - Jeff "
  "Ellis and Mosty among them - so that their way of working would run on a different system. "
  "Sitting inside someone else's session is the fastest way to see where the decisions really are.")
t('p_what3',
  "Two terms from Jeff Ellis' Mixer Brain course stuck, because they put a name on something I had "
  "been doing for years without one: the forbidden zone, and the fourth dimension. When a mix is "
  "dense and every frequency is taken, width and depth are the only room left. SpaceX is built to "
  "work in that room.")
t('p_what4',
  "Think of it as an effect generator rather than a mastering tool. It takes something ordinary - "
  "a reverb return, a delay throw, a stack of backing vocals - and makes it worth listening to.")

t('h_instead', 'What it replaces')
t('p_instead',
  "Everything below normally needs four or five separate plugins:")
t('instead', [
  ['Centre extraction (L / C / R)', 'a dedicated centre-separation plugin'],
  ['Haas delay and micro pitch', 'a pitch / delay tool'],
  ['Polarity, side gain, side width', 'a utility plugin - usually an ugly one'],
  ['Mono check and correlation', 'a metering plugin'],
  ['Depth and distance', 'an EQ move you would otherwise do by hand'],
  ['Auto-pan and stereo movement', 'a panner'],
  ['Stereo phaser', 'a modulation plugin'],
])
t('p_instead2',
  "One window, one latency, one preset. That is most of the point.")

t('h_where', 'Where to put it')
t('p_where1',
  "It works best on anything that is already wet: backing vocals, ad-libs, a stack of harmonies, a "
  "reverb or a delay return. Throw tracks belong here too - those are usually single tracks rather "
  "than buses. A dry source is not off limits either: a lead vocal that is meant to be doubled "
  "works, and so do drums. The rule of thumb is simple - the more room effects sit in front of "
  "SpaceX, the further you can push it.")
t('p_where2',
  "Position in the chain is a taste decision. Last is the usual place: shape whatever comes out of "
  "the chain. First also works when the incoming signal is already drenched in reverb or echo - "
  "then SpaceX shapes the effect itself rather than the source.")
t('p_where3',
  "Throw tracks are the easiest win. They are 100 % wet by definition, nobody expects them to be "
  "mono-safe, and a little Orbit with some Parallax turns a plain delay into a hook.")
t('p_where4',
  "It is not a mastering tool. Do not put it on the mix bus and expect it to behave.")

t('h_quick', 'First result in one minute')
t('quick', [
  "Put it on a backing vocal bus or a throw track.",
  "Click the profile pill at the top until it says what the track is - Vocal, Backing, Adlib or FX.",
  "Click the dice next to it. Listen. Click again. Keep going until something surprises you.",
  "Too much? Pull Mix down to 70 %. Still too much? Click a section name to switch it off.",
  "Check Mono at the bottom. If it collapses, lower Size in Mid-Side a little.",
])
t('p_quick',
  "That is the whole workflow. The knobs below exist for when you already know what you want.")

t('h_smart', 'The Smart engine')
t('p_smart1',
  "The dice at the top builds a complete setting for you - and decides which sections belong in "
  "it. A section that would get nothing useful is switched off rather than left sitting there "
  "doing nothing, so you can always see what the roll actually did.")
t('p_smart2',
  "The pill next to it says what you are working on. Five positions: No Profile, Vocal, Backing, "
  "Adlib, FX. Click for the next one, Cmd-click to go back, or click a dot to jump straight to "
  "one. With a profile selected the dice stays inside what makes sense for that source; the line "
  "above the field says what the profile does.")
t('p_smart3',
  "Press it, listen, press it again. Keep going until something surprises you - and it will.")
t('p_smart4',
  "Don't worry about what it might do to your track. This is not the randomise button other "
  "plugins have. Everything it can reach was tuned by ear, and whatever did not work never made "
  "it in - so you will not get a result you have to undo.")
t('smart', [
  ['Profile pill', 'Vocal, Backing, Adlib, FX - or No Profile, where the dice may reach for anything'],
  ['Dice', 'rolls a new setting and decides which sections belong in it'],
  ['Lock (padlock)', 'this section is left alone by the dice and by Breathe'],
  ['Breathe', 'switches modulation on in every section with fresh depths'],
  ['Life', 'scales every modulation at once. At zero nothing moves'],
  ['Mod', 'switches all modulation off without losing the depths'],
  ['Undo / Redo', 'works on everything, the dice included'],
  ['A / B', 'two versions side by side; the arrow copies one into the other'],
])

t('h_layout', 'The signal path')
t('p_layout',
  "Left to right, top to bottom - the way you read it is the way the audio travels.")
t('chain', ['IN', 'GALAXY', 'ECLIPSE', 'PARALLAX', 'DIMENSION', 'HYPERDRIVE', 'RAYE', 'MIX', 'OUT'])
t('p_chain_pol',
  "Eclipse is the exception: it can sit at two points in that chain. Early flips right after "
  "Galaxy, Late at the very end. The button shows where it currently sits.")
t('p_chain_filter',
  "Latency is zero unless Galaxy is armed. Galaxy needs an analysis window to separate the centre, "
  "so switching it on costs a little delay. Everything else runs in real time - which is why the "
  "engine has its own switch in the header rather than being always on.")

t('h_common', 'On every section')
t('common', [
  ['Section name', 'click it to switch the section on or off'],
  ['Cmd + name', 'solo - hear this section on its own'],
  ['Padlock', 'the dice and Breathe leave this section alone'],
  ['Wave icon', 'modulation on / off for this section'],
  ['Small knob', 'modulation depth, scaled by Life'],
])

t('h_galaxy', 'GALAXY')
t('p_galaxy',
  "Pulls the centre out of the stereo image and treats centre and sides separately. This is the "
  "part that costs latency, and the part that makes everything after it sound different. It only "
  "runs while the engine is armed - the GALAXY button in the header.")
t('galaxy', [
  ['Orbit', 'how much of the sides comes back in. All the way down is centre only'],
  ['Gravity', 'how hard the centre is separated from the sides'],
  ['Regain', 'brings back the level the separation takes away'],
])
t('tip_galaxy',
  "On a backing stack, pull Orbit up. The lead stays where it is and the backings open up around "
  "it without getting louder.")

t('h_pol', 'ECLIPSE')
t('p_pol',
  "Flips the polarity of one channel. The most drastic thing in the plugin and the one that makes "
  "the biggest difference.")
t('pol', [
  ['L / R', 'flip the left or the right channel'],
  ['Link', 'both at once'],
  ['Early / Late', 'where in the chain the flip happens - click to switch'],
])
t('warn_pol',
  "A flip almost always breaks mono compatibility. On a throw or an effect return that is fine - "
  "nobody sums those to mono and expects them to survive. On anything that carries the song, check "
  "Mono first.")
t('p_pol2',
  "The two positions matter more than they look. Flipping before the width stage changes what "
  "everything after it is working on; flipping at the end only affects the final image. Try both "
  "on the same setting - they are two different sounds.")

t('h_tw', 'PARALLAX')
t('p_tw',
  "Turns a mono-ish sound into a wide one, using tiny differences between left and right in time "
  "and in pitch. One dial and one decision: pick a style, then set Amount.")
t('p_tw2',
  "The styles are finished settings, not presets you have to dial in. Amount runs from nothing to "
  "the full effect, and the dots under the button jump straight to a style.")
t('tw', [
  ['Double', 'the plain doubler. Widest at full Amount, still solid in mono'],
  ['Wide', 'a touch of detune with it - more air, less obvious doubling'],
  ['Illusion', 'the big one. Delay and detune together, clearly out front'],
  ['3D', 'leans the image and pushes it outward. Good under a lead'],
  ['Drift', 'the same idea tilted to one side - asymmetric on purpose'],
  ['Flux', 'a macro: starts small and ends at the full effect'],
])
t('tw_tech',
  "Under the hood: Drift is a Haas delay of a few milliseconds, Shift a micro pitch offset in "
  "cents, Tilt a pan of the widened signal. Technical Labels renames the section MicroPitch.")

t('h_dim', 'DIMENSION')
t('p_dim',
  "Classic mid / side width, plus depth. The section you reach for when the result is nearly right "
  "and just needs to be wider, heavier or further away.")
t('dim', [
  ['Size', 'stereo width. Below 100 % narrower, above 100 % wider'],
  ['Boost', 'adds level to the sides without touching the centre'],
  ['Depth', 'left moves the sound back into the room and darkens it, right pulls it close'],
])
t('tip_dim',
  "Size above roughly 150 % together with a strong Parallax style is where mono starts to suffer. "
  "The plugin already limits the worst combinations, but your ears decide.")

t('h_hyp', 'HYPERDRIVE')
t('p_hyp',
  "Slow automatic movement through the stereo field. Not a tremolo - the level stays put, only the "
  "position moves.")
t('hyp', [
  ['Flow', 'how far the sound travels left and right'],
  ['Sine / Pulse', 'the shape of the movement. Click to switch'],
  ['Speed', 'movement rate in Hz'],
  ['Sync', 'lock the rate to the host tempo'],
  ['Bars', 'the note length used while Sync is on'],
])

t('h_raye', 'RAYE')
t('p_raye',
  "A phaser that moves the stereo image rather than the tone. Subtle on purpose - it is movement, "
  "not an effect you should be able to name. Amount is the whole control; the character decides "
  "how it moves.")
t('raye', [
  ['Amount', 'how strong the movement is. At zero the section is silent'],
  ['Sweep', 'the classic slow sweep, low and wide'],
  ['Shimmer', 'high and quick - air rather than body'],
  ['Spin', 'low, fast and thick. The obvious one'],
  ['Swirl', 'very slow and broad. You notice it after a while, not straight away'],
  ['Fast', 'runs the current character about a third quicker'],
  ['Pair', 'follow Hyperdrive at half its speed, never faster than half a bar'],
])

t('h_footer', 'Output')
t('footer', [
  ['In / Out', 'input and output level'],
  ['Mono', 'listen to the result in mono'],
  ['Dry', 'while in mono, compare with the unprocessed input'],
  ['Mix', 'blend the processed sound with the original'],
  ['Vol', 'output trim, plus or minus 6 dB'],
  ['Correlation', 'the thin bar in the field: right of centre is mono-safe, left of it cancels'],
  ['AG', 'auto gain - matches the output to the input so bypass is an honest comparison'],
])

t('h_mod', 'Modulation and Life')
t('p_mod1',
  "Every section can breathe. The wave icon in a section header switches its modulation on, the "
  "small knob next to it sets how far it moves. Switch Show Advanced Modulation on in the settings "
  "and both appear in every section; leave it off and the sections stay clean.")
t('p_mod2',
  "Life at the top scales all of it at once. At zero nothing moves at all, no matter what the "
  "individual depths say - which makes it the fastest way to calm a setting down without losing "
  "it. The moving dots on the knobs show what the modulation is doing right now.")

t('h_star', 'The starfield')
t('p_star1',
  "The picture on the left is not decoration only. Every object reacts to what the plugin is "
  "actually doing - the planet rises with Gravity, the blue lines come from Regain, the field "
  "curves with Orbit, and the whole scene leans with the movement.")
t('p_star2',
  "Click the field to switch the scope on or off. The gear opens the display settings.")
t('star', [
  ['Goniometer', 'the classic vectorscope - a vertical line is mono, a circle is wide'],
  ['Scene', 'Space shows everything, Stars is the quiet meter view'],
  ['Speed / Density', 'how fast and how busy the field is'],
  ['Stars / Lines', 'switch the twinkling stars or the warp lines off'],
])

t('h_presets', 'Presets')
t('p_presets',
  "The name field in the header shows what is loaded; the arrows step through the list. A star "
  "after the name means you have changed something since loading it.")
t('presets', [
  ['Save', 'store the current settings. Name it Default to overwrite the default'],
  ['Delete', 'remove the loaded preset'],
  ['Rename / Preset Folder', 'in the preset list, below the names'],
  ['Save State as Default', 'in the settings: every new instance starts like this'],
])
t('p_presets2',
  "Presets that use Galaxy are greyed out while the engine is off, and the arrows skip them. "
  "Nothing loads a setting that cannot be heard.")

t('h_look', 'Themes, layout and labels')
t('p_look',
  "Five themes and two layouts. Layout changes how much framing the sections get: 3D has soft "
  "shading and depth, Outline a thin frame and no fill. None of it changes the sound. Hover a "
  "theme to preview it, hover a layout to see what the framing does.")
t('p_look2',
  "Technical Labels renames everything by what it does rather than what it feels like. Useful if "
  "you would rather see the engineering than the story:")
t('labels', [
  ['Galaxy', 'LCR &middot; Orbit becomes L/R, Gravity becomes C-Weight'],
  ['Eclipse', 'Polarity'],
  ['Parallax', 'MicroPitch'],
  ['Dimension', 'Mid-Side &middot; Size becomes Width, Boost becomes Gain'],
  ['Hyperdrive', 'Autopan &middot; Flow becomes Width'],
  ['Raye', 'Phaser'],
])

t('h_help', 'Finding your way')
t('help', [
  ['? bottom left', 'switches the hint line on. It says what the mouse is over'],
  ['Take the Tour', 'a short guided walk through the sections. In the settings'],
  ['Back Panel', 'click the logo: who built this, how to reach him, which copy this is'],
  ['i next to the profile', 'hides the Smart profile line if you do not want it'],
])

t('h_demo', 'Demo and activation')
t('p_demo',
  "Without a serial number SpaceX runs in demo mode: everything works, but the sound goes quiet "
  "for a moment every 50 seconds. The DEMO badge next to the logo lights up while that happens, "
  "so it is never a mystery. Click it and you can enter a serial, open the shop, or carry on.")
t('p_demo2',
  "Activation asks for your name and your serial number. The two belong together - the number is "
  "derived from the name, and the plugin checks them against each other. Type the name exactly as "
  "it appears in your order; upper and lower case and spaces do not matter. Your name then sits on "
  "the back panel under Registered to.")

t('h_recipes', 'Five settings that work')
t('recipes', [
  ['Backing stack that will not fit',
   "Backing profile, then the dice. Orbit up, Size around 120 %. The lead keeps the centre, the "
   "backings take everything around it."],
  ['Delay throw with an identity',
   "Parallax on Illusion, Amount around 60 %, Raye on Sweep at a third. The throw stops sounding "
   "like a delay and starts sounding like a decision."],
  ['Reverb return with depth',
   "Depth to the left, Size around 130 %, Boost a touch. The reverb moves behind the dry signal "
   "instead of sitting on top of it."],
  ['Ad-libs that appear and disappear',
   "Hyperdrive Flow around 40 %, Sync on, 4 bars. Slow enough that nobody notices the movement, "
   "fast enough that the part never sits still."],
  ['The forbidden one',
   "Eclipse L on, Early, Mix around 60 %. Mono will not survive it. On a throw in a dense chorus "
   "it is the widest thing you own."],
])

t('h_short', 'Clicks worth knowing')
t('short', [
  ['Double-click a knob', 'back to default'],
  ['Cmd / Ctrl + drag', 'fine adjustment'],
  ['Click a section name', 'section on / off'],
  ['Cmd + section name', 'solo that section'],
  ['Click a style pill', 'next style. Cmd-click goes back, a dot jumps straight there'],
  ['Click the logo', 'back panel'],
  ['Right-click a knob', 'lock it against the dice'],
])

t('h_thanks', 'Credits')
t('p_thanks1',
  "SpaceX started as a way to stop opening five plugins for one idea. Nothing in it happened by "
  "accident: the order of the chain, where the polarity flip is allowed to sit, how far each "
  "section may go - every one of those is a decision that was made by listening, and then made "
  "again.")
t('p_thanks2',
  "Thanks to Jeff Ellis, whose words for the forbidden zone and the fourth dimension gave the "
  "idea a name. And to everyone who tested SpaceX and sent feedback - most of what is good about "
  "the last hundred details came from them.")
t('p_thanks3',
  "Paul Misty &middot; Mistycat Studios &middot; info@paulmisty.com &middot; paulmisty.com "
  "&middot; @paulmisty.studio")
