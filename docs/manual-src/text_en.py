# -*- coding: utf-8 -*-
EN = {}
def t(k, v): EN[k] = v

t('title_sub', 'SPATIAL INTELLIGENCE')
t('title_sub2', 'SMART IMAGING')
t('manual', 'Manual')
t('cover_meta', 'Version 1.0 - VST3 - macOS / Windows')

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
  "Everything below normally needs four or five separate plugins, and around 500 dollars:")
t('instead', [
  ['Centre extraction (L / C / R)', 'a dedicated centre-separation plugin'],
  ['Haas delay and micro pitch', 'a pitch / delay tool'],
  ['Polarity, side gain, side width', 'a utility plugin - usually an ugly one'],
  ['Mono check and correlation', 'a metering plugin'],
  ['Distance and tone tilt', 'an EQ'],
  ['Auto-pan and stereo movement', 'a panner'],
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
  "mono-safe, and a little Orbit with some modulated Timewarp turns a plain delay into a hook.")
t('p_where4',
  "It is not a mastering tool. Do not put it on the mix bus and expect it to behave.")

t('h_quick', 'First result in one minute')
t('quick', [
  "Put it on a backing vocal bus or a throw track.",
  "Click the Backings category at the top - or Vocals, Pads, whatever the track is.",
  "Click the Smart button. Listen. Click again. Keep going until something surprises you.",
  "Too much? Pull Mix down to 70 %. Still too much? Turn one section off.",
  "Check Mono at the bottom. If it collapses, lower Dimension Size a little.",
])
t('p_quick',
  "That is the whole workflow. The knobs below exist for when you already know what you want.")

t('h_smart', 'The Smart engine')
t('p_smart1',
  "The two buttons at the top build a new setting for you. The left one keeps every section "
  "playing. The right one also decides which sections belong in it, which usually gives the "
  "cleaner result.")
t('p_smart2',
  "The six category chips tell it what it is working on. Pick one and it takes over - from then "
  "on a single button is all you need.")
t('p_smart3',
  "Press it, listen, press it again. Keep going until something surprises you - and it will.")
t('p_smart4',
  "Don't worry about what it might do to your track. This is not the randomise button other "
  "plugins have. Everything it can reach was tuned by ear, and whatever did not work never made "
  "it in - so you will not get a result you have to undo.")
t('smart', [
  ['Lock (padlock)', 'this section is left alone by Smart and by Breathe'],
  ['Breathe', 'switches modulation on in every section with fresh random depths'],
  ['Undo / Redo', 'works on everything, Smart included'],
  ['A / B', 'two versions side by side; the arrow copies one into the other'],
])

t('h_layout', 'The signal path')
t('p_layout',
  "Left to right, top to bottom. Polarity can sit at four different points in that chain - that is "
  "what the 1-4 buttons do.")
t('chain', ['IN', 'GALAXY', 'POLARITY', 'TIMEWARP', 'DIMENSION', 'HYPERDRIVE', 'VISION', 'RAYE', 'MIX', 'OUT'])
t('p_chain_pol',
  "Latency is zero unless Galaxy is armed. Galaxy needs an analysis window to separate the centre, "
  "so switching it on costs a little delay. Everything else runs in real time.")
t('p_chain_filter',
  "Focus sits across the chain rather than in it. Galaxy, Dimension and Vision work inside its "
  "range only - wide open it does nothing at all. The small focus symbol in the Galaxy and "
  "Dimension headers takes that section back out again, so you can widen the top end while the "
  "centre separation still covers the whole spectrum.")

t('h_common', 'On every section')
t('common', [
  ['S', 'solo - hear this section on its own'],
  ['Padlock', 'Smart leaves this section alone'],
  ['Section name', 'click it to switch the section on or off'],
  ['Wave icon', 'modulation on / off for this section'],
  ['Small knob', 'modulation depth'],
])

t('h_galaxy', 'GALAXY')
t('p_galaxy',
  "Pulls the centre out of the stereo image and treats centre and sides separately. This is the "
  "part that costs latency, and the part that makes everything after it sound different.")
t('galaxy', [
  ['Gravity', 'how hard the centre is separated from the sides'],
  ['Orbit', 'down keeps only the centre, up keeps only the sides, middle is the original'],
])
t('tip_galaxy',
  "On a backing stack, pull Orbit up. The lead stays where it is and the backings open up around "
  "it without getting louder.")

t('h_pol', 'POLARITY')
t('p_pol',
  "Flips the phase of one channel. The most drastic thing in the plugin and the one that makes the "
  "biggest difference.")
t('pol', [
  ['L / R', 'flip the left or the right channel'],
  ['Link', 'both at once'],
  ['1 - 4', 'where in the chain the flip happens'],
])
t('warn_pol',
  "A flip almost always breaks mono compatibility. On a throw or an effect return that is fine - "
  "nobody sums those to mono and expects them to survive. On anything that carries the song, check "
  "Mono first.")
t('p_pol2',
  "The four positions matter more than they look. Flipping before Galaxy changes what Galaxy "
  "considers the centre. Flipping after Vision only affects the final image. Try 1 and 3 on the "
  "same setting - they are two different sounds.")

t('h_tw', 'TIMEWARP')
t('p_tw',
  "Tiny differences between left and right in time and in pitch. This is the classic width trick, "
  "and the reason two plugins used to be needed.")
t('tw', [
  ['Drift', 'delays one side by a few milliseconds (Haas). Left or right'],
  ['Shift', 'micro pitch offset between the channels, in cents'],
  ['Balance', 'evens out the loudness shift Drift causes'],
])

t('h_dim', 'DIMENSION')
t('p_dim',
  "Classic mid / side width. The section you reach for when the result is nearly right and just "
  "needs to be wider or narrower.")
t('dim', [
  ['Size', 'stereo width. Below 100 % narrower, above 100 % wider'],
  ['Boost', 'adds level to the sides'],
])
t('tip_dim',
  "Size above roughly 150 % together with Drift is where mono starts to suffer. The plugin already "
  "limits the worst combinations, but your ears decide.")

t('h_hyp', 'HYPERDRIVE')
t('p_hyp',
  "Slow automatic movement through the stereo field. Not a tremolo - the level stays put, only the "
  "position moves.")
t('hyp', [
  ['Flow', 'how far the sound travels left and right'],
  ['Pulse', 'a smoother, pulse-like movement instead of a sine'],
  ['Speed', 'movement rate in Hz'],
  ['Sync', 'lock the rate to the host tempo'],
  ['Bars', 'the note length used while Sync is on'],
])

t('h_vis', 'VISION')
t('p_vis',
  "Where the finished picture sits and how far away it is. This is the section that replaces an EQ "
  "move you would otherwise do by hand.")
t('vis', [
  ['Tilt', 'shifts the whole image left or right'],
  ['Width', 'final width of the image'],
  ['Distance', 'further away means a softer top end and a little less level'],
  ['Elevate', 'gentle tone tilt. Up is brighter, down is darker'],
])

t('h_raye', 'RAYE')
t('p_raye',
  "A soft phaser that moves the stereo image rather than the tone. Subtle on purpose - it is "
  "movement, not an effect you should be able to name.")
t('raye', [
  ['Strength', 'click to step through Light, Medium, Strong and Off'],
  ['Speed', 'how fast the movement cycles'],
  ['Pair', 'follow Hyperdrive at half its speed'],
])

t('h_footer', 'Output')
t('footer', [
  ['IN / OUT', 'input and output level'],
  ['Mono', 'listen to the result in mono'],
  ['Dry', 'while in mono, compare with the unprocessed input'],
  ['Mix', 'blend the processed sound with the original'],
  ['Vol', 'output trim, plus or minus 6 dB'],
  ['Correlation', 'right of centre is mono-safe, left of centre cancels in mono'],
])

t('h_prism', 'Focus')
t('p_prism',
  "The range SpaceX works in. Nothing is taken out of the sound here - outside the range the "
  "signal simply passes through untouched. Drag an edge to resize, the middle to move, up and "
  "down or scroll to widen.")
t('prism', [
  ['Power', 'focus on / off'],
  ['Range', 'where Galaxy, Dimension and Vision are allowed to work'],
  ['Focus symbol', 'in the Galaxy and Dimension headers: takes that section back out of the focus'],
])
t('tip_prism',
  "Set the lower edge to around 200 Hz and the low end stays mono and solid while everything above "
  "it opens up. This is the single most useful setting in the plugin.")

t('h_star', 'The starfield')
t('p_star1',
  "The picture on the left is not decoration only. Every object reacts to what the plugin is "
  "actually doing - the planet rises with Gravity, the star lines lean with Tilt, the golden lines "
  "are RAYE.")
t('p_star2',
  "Click the field to switch the scope on or off. Click the sun or the moon to switch between the "
  "full view and the clean meter view. The gear opens the display settings.")
t('star', [
  ['Goniometer', 'the classic vectorscope - a vertical line is mono, a circle is wide'],
  ['Scene', 'Space shows everything, Stars is the quiet meter view'],
  ['Speed / Density', 'how fast and how busy the field is'],
  ['Stars', 'switch the twinkling stars off'],
])

t('h_look', 'Themes and layout')
t('p_look',
  "The menu holds seven themes and three layouts. Layout changes how much framing the sections "
  "get: 3D has boxes and fill, Flat has neither, Outline has a thin frame and no fill. None of it "
  "changes the sound.")

t('h_recipes', 'Five settings that work')
t('recipes', [
  ['Backing stack that will not fit',
   "Backings category, then Smart. Orbit up, Dimension Size around 120 %, Focus from "
   "200 Hz. The lead keeps the centre, the backings take everything around it."],
  ['Delay throw with an identity',
   "Timewarp Drift at 8-12 ms, Shift around 6 cents, RAYE on Light. The throw stops sounding like "
   "a delay and starts sounding like a decision."],
  ['Reverb return with depth',
   "Vision Distance up, Elevate down a little, Dimension Size around 130 %. The reverb moves "
   "behind the dry signal instead of on top of it."],
  ['Ad-libs that appear and disappear',
   "Hyperdrive Flow around 40 %, Sync on, 4 bars. Slow enough that nobody notices the movement, "
   "fast enough that the part never sits still."],
  ['The forbidden one',
   "Polarity L on, position 1, Mix around 60 %. Mono will not survive it. On a throw in a dense "
   "chorus it is the widest thing you own."],
])

t('h_short', 'Clicks worth knowing')
t('short', [
  ['Double-click a knob', 'back to default'],
  ['Cmd / Ctrl + drag', 'fine adjustment'],
  ['Click the section name', 'section on / off'],
  ['Click the logo', 'bypass'],
  ['? bottom left', 'shows what the mouse is over'],
])

t('h_thanks', 'Credits')
t('p_thanks1',
  "SpaceX started as a way to stop opening five plugins for one idea. Nothing in it happened by "
  "accident: the order of the chain, the four polarity positions, how far each section is allowed "
  "to go - every one of those is a decision that was made by listening, and then made again.")
t('p_thanks2',
  "Thanks to Jeff Ellis, whose words for the forbidden zone and the fourth dimension gave the "
  "idea a name.")
t('p_thanks3',
  "Paul Misty - Mistycatstudios &middot; info@paulmisty.com &middot; paulmisty.com &middot; Germany")
