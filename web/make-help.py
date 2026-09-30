#!/usr/bin/env python3
"""Writes the in-page game guide (dist/help.html) for the web build.

The game content comes from the desktop key guides in
~/Desktop/Games/Roguelikes/Docs (build-docs.py + guides.py), so both guides
stay in sync; only the saving and "playing in the browser" parts are
written here, because they differ on the web."""
import html, importlib.util, os, sys

DOCS = os.path.expanduser('~/Desktop/Games/Roguelikes/Docs')
PAGE = 'omega.html'

sys.path.insert(0, DOCS)
spec = importlib.util.spec_from_file_location('build_docs', os.path.join(DOCS, 'build-docs.py'))
docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(docs)
from guides import GUIDES   # noqa: E402

game = next(g for g in docs.GAMES if g['file'] == PAGE)
# rebirth: the command list is lib/help12.txt (build-docs.py still names omegalib/)
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
game['all'] = lambda: docs.parse_omega(os.path.join(ROOT, 'lib', 'help12.txt'))
guide = dict(GUIDES.get(PAGE, {}))
info = dict(game['info'])
kbd = docs.kbd
esc = html.escape

SAVING = '''<ul>
<li><strong>Saving is automatic.</strong> The game is stored in this browser (IndexedDB) between your commands (at most every two seconds), under your character's name. After a reload, pick it under <em>Saved games</em> in the start menu to continue.</li>
<li><kbd>S</kbd> saves and ends the session, as in the original; reload the page (or press <em>Play again</em>) to continue.</li>
<li>When your character dies or you quit with <kbd>Q</kbd>, the save is deleted: death is final.</li>
<li><em>Export save</em> downloads the save file; <em>Import save</em> loads one.</li>
<li>Private/incognito windows and "clear site data" delete the stored game. Export first if it matters.</li>
</ul>'''

WEB = '''<ul>
<li><em>Tiles</em> cycles the map between the 32×32 tiles of David Kinder's Windows Omega (github.com/DavidKinder/Omega), the 8×12 tiles of <strong>gromega</strong> 0.80.2a (omegatiles.xpm from www.alcyone.com/binaries/omega/gromega-0.80.2a-src.tar.gz, the graphical Omega at Erik Max Francis's Omega page) and the original 16-colour text; the choice is kept in this browser. gromega draws walls, rivers and mountains joined up; here every cell shows one whole tile. <em>A−</em> / <em>A+</em> on the Map title bar (shown on hover) change the size.</li>
<li><strong>Keys:</strong> <kbd>h</kbd><kbd>j</kbd><kbd>k</kbd><kbd>l</kbd><kbd>y</kbd><kbd>u</kbd><kbd>b</kbd><kbd>n</kbd>, the arrow keys or the numeric keypad move you; capital letters run.</li>
<li>Browsers keep a few shortcuts for themselves (<kbd>Ctrl+W</kbd>, <kbd>Ctrl+T</kbd>, <kbd>Ctrl+N</kbd>, and <kbd>Cmd</kbd> shortcuts on a Mac), so those never reach the game. <kbd>Ctrl+P</kbd> (previous message) works.</li>
<li>If the game ever crashes, a message appears at the top; reload the page to continue from the last autosave.</li>
</ul>'''

KEY_HINTS = [
    ('?', 'In-game help and command list'),
    ('X', 'Auto-explore: walk to the nearest unexplored spot'),
    ('Enter', 'Menu of all commands'),
    ('i', 'Slots and pack in one list: Enter = use, equip (w), take off (W), drop (d), call'),
    ('w / W', 'Equip from the pack / take off'),
    ('>', 'Go down (walks to the nearest known staircase)'),
]

# Questions, hints and spoilers: paraphrased and adapted to rebirth and this port from the
# 1994-95 alt.games.omega texts on Erik Max Francis's Omega page (www.alcyone.com/max/projects/omega/):
# the Omega FAQ (Cory L. Kerens), the players' hints collection and the Omega Spoilers file (A. Light).
# They were written for 0.75; details may differ in rebirth.
SOURCE = ('<p class="note">Adapted from the Omega FAQ (Cory L. Kerens, 1995), the alt.games.omega hints collection and the '
          'Omega Spoilers file (A. Light, 1994), all on <a href="http://www.alcyone.com/max/projects/omega/">Erik Max Francis\'s Omega page</a>. '
          'They were written for Omega 0.75; rebirth may differ in details.</p>')

FAQ = SOURCE + '''<dl>
<dt>How should I start?</dt><dd>Join a guild before you gain any experience (no fights, traps or quests first), because only experience earned as a member counts towards its ranks. Some guilds give you a weapon and armour, so join before you shop. Then buy what is missing at the armourer (Julie's), a little food at the fast-food place, and look into the pawn shop now and then. <kbd>M</kbd> walks you to any place in Rampart you have already found.</dd>
<dt>Which guild?</dt><dd>Every guild gives something good. Spells: a temple (priests of every god but Destiny also get spells and help with curses), the Collegium Magii (many spells, experience for research; free with Intelligence 18) or the Sorcerors (fewer spells; you must turn more chaotic to rise). Fighting: the Mercenaries (money and basic gear), the Order of Paladins (gear, a horse, meals and healing at their hall; you must turn ever more lawful) or the Arena (gym time, an extra combat maneuver, prize money). The Thieves identify and buy goods cheaply and teach picking pockets and locks. Some guilds exclude each other, and experience is split between all guilds you belong to, so start with one (plus the Arena if it allows).</dd>
<dt>What next?</dt><dd>Visit the Oracle: her first quest is safe and takes you to about level 3, and she later sends you to a place you cannot reach otherwise. She lives in the wilder part of Rampart. The Duke in the castle gives quests too.</dd>
<dt>How do I change my alignment?</dt><dd>Rampart itself has one place where a good deed makes you slightly lawful and one where a bad deed makes you slightly chaotic: enough to join any guild. Outside, lawful characters spare monsters that do not attack; betraying a monster is very chaotic. The Archdruid's ritual pulls you back towards neutral (less so the more experienced you are).</dd>
<dt>Monsters keep slowing me down.</dt><dd>Prevent it: being hasted (a stimtab, the haste spell, boots of speed) makes you immune. Once slowed, wait it out or rest a week in a rented condo.</dd>
<dt>Some monsters ignore my weapon.</dt><dd>A mace of disruption, a lightsabre, an acid whip or Demonblade (cursed, makes you berserk) hit them; a firestar burns them. Victrix, Defender, the vorpal sword and similar do not. Otherwise dispel them (wand, scroll or spell) or attack with fire, cold or disruption from a distance.</dd>
<dt>Wizard mode?</dt><dd>In rebirth <kbd>Ctrl+G</kbd> only works for the maintainer's user name, so on this page it just summons a warning.</dd>
</dl>'''

HINTS = SOURCE + '''<ul>
<li>Hit and run works for many quests: get in fast (high speed, levitation over water), strike, grab the prize and leave before the guards act.</li>
<li>Invisibility helps a lot in the Arena.</li>
<li>If you skip dungeon levels (lift, warp), you can miss a special level; you cannot warp straight to one, so go to the level above and walk down.</li>
<li>Curses are removed by blessing, sometimes several times: the bless spell, or a generous sacrifice to your god with a request for a blessing.</li>
<li>The alchemist's "transformation" turns some monster remains into magic items; parts the alchemist will buy can usually be transformed.</li>
<li>Sell to the Thieves' Guild rather than the pawn shop, and identify first. An item they will not bid on is probably cursed: do not wear it.</li>
<li>Breathing (potion or spell), levitation or riding keeps you from drowning.</li>
<li>Take plenty of food on long journeys (the swamps have nothing to hunt), and save before dangerous places: here the game also autosaves between commands.</li>
<li>Not every countryside altar is friendly; some take your pack.</li>
</ul>'''

SPOILERS = SOURCE + '''<p><strong>Spoiler warning:</strong> the rest of this section gives away places, quests and secrets. Open a part only if you want to know.</p>
<details><summary>Places in the countryside</summary><ul>
<li>The Goblin Caves lie a little south-west of Rampart; the Goblin King waits on level 10.</li>
<li>The Archdruid's temple (Woodhenge) is north of the city: follow the road north-east to a village, then search the forest north-west of it. Inside, search for secret passages behind the altar.</li>
<li>The Archmage's Castle is behind hidden mountain passes in the far north-west (search, <kbd>s</kbd>, on the map to find passes). Star Peak is in the north-east, the Dragon's Lair across the great desert in the east, the Eater of Magic's island and the Hellwell volcano in the swamps of the south-west, beside the Sea of Chaos (deadly for lawful characters).</li>
<li>Special levels: Rampart sewers 18 (the Great Wyrm), Archmage's Castle 16, Goblin Caves 10, volcano 20 (the Demon Emperor).</li>
</ul></details>
<details><summary>Quests</summary><ul>
<li>The Duke asks in turn for the Goblin King's head, the Great Wyrm's sword from the bottom of the sewers, dragonscale armour, and finally the Orb of Mastery.</li>
<li>The Collegium's Archmage quest is the heart of the Eater of Magic, on its island in the swamps.</li>
<li>Priests become high priest by bringing back their god's holy symbol; the priests at Woodhenge can be pickpocketed for maces and the druids' symbol.</li>
</ul></details>
<details><summary>Artifacts and wishing</summary><ul>
<li>The Amulet of the Planes (from the Demon Emperor) takes you to any village or temple for 100 mana: perhaps the most useful thing in the game.</li>
<li>The five Orbs come from the masters on the Astral Plane (reached through the Oracle). Keep them all equipped and activate the Orb of Mastery; show it to the Duke first.</li>
<li>Kolwynia, the Key that was Lost, teaches every spell; identify it before each use.</li>
<li>Wishes need the exact word: Health, Wealth, Knowledge, Location, Balance, Law, Chaos, Skill, Summoning, Destruction (kills everything, you too), Acquisition, Death.</li>
</ul></details>
<details><summary>The Adept's Challenge</summary><ul>
<li>Reach it by casting ritual magic in the fitting room, wishing for Location, the Amulet of the Planes, or on foot to the Temple of Destiny.</li>
<li>Walk over every free square of the challenge: three of them give the hints you need. You only have to kill Death once, for what he carries.</li>
</ul></details>'''


def dl(items):
    return '<dl>' + ''.join(f'<dt>{kbd(k)}</dt><dd>{esc(d)}</dd>' for k, d in items) + '</dl>'


def section(anchor, title, body):
    return f'<h2 id="h-{anchor}">{esc(title)}</h2>{body}'


parts = []
toc = [('about', 'About the game'), ('keys', 'Keyboard controls'), ('saving', 'Saving your game'),
       ('tips', 'Tips'), ('guide', "New player's guide"), ('faq', 'Questions and answers'),
       ('hints', 'Hints'), ('spoilers', 'Spoilers'), ('web', 'Playing in the browser')]
parts.append('<p>' + esc(game['tagline']) + '</p>' + info['About the game'] + '<ul class="toc">' +
             ''.join(f'<li><a href="#h-{a}">{esc(t)}</a></li>' for a, t in toc) + '</ul>')


parts.append(section('about', 'About the game',
                     guide.pop('How Omega differs from Angband')))

ess = ''.join(f'<div class="box"><h3>{esc(cat)}</h3>{dl(items)}</div>' for cat, items in game['essentials'])
all_keys = game['all']() if callable(game['all']) else game['all']
full = ''.join(f'<div>{kbd(k)}<span>{esc(d)}</span></div>' for k, d in all_keys)
parts.append(section('keys', 'Keyboard controls',
                     '<div class="box key"><h3>The keys to remember</h3>' + dl(KEY_HINTS) + '</div>'
                     '<h3>Essential keys</h3><div class="grid">' + ess + '</div>'
                     '<details><summary>Complete key list (' + str(len(all_keys)) + ' commands)</summary>'
                     '<div class="all">' + full + '</div></details>'))

parts.append(section('saving', 'Saving your game', SAVING))
parts.append(section('tips', 'Tips', info['Tips']))
parts.append(section('guide', "New player's guide",
                     ''.join(f'<h3>{esc(t)}</h3>{b}' for t, b in guide.items())))
parts.append(section('faq', 'Questions and answers', FAQ))
parts.append(section('hints', 'Hints', HINTS))
parts.append(section('spoilers', 'Spoilers', SPOILERS))
parts.append(section('web', 'Playing in the browser', WEB))

# RVIP: About this version
parts.append('<h2 id="h-version">About this version</h2><ul>'
             '<li>Based on <strong>Omega Rebirth v0.7.0</strong> (github.com/Lyle-Tafoya/omega-rebirth, C++23) by Lyle Tafoya, '
             'from Omega by Laurence R. Brothers and Erik Max Francis.</li>'
             '<li>Tiles: David Kinder\'s WinOmega sheet and the gromega 0.80.2a sheet (www.alcyone.com/binaries/omega/gromega-0.80.2a-src.tar.gz). '
             'Questions, hints and spoilers: paraphrased from the texts on www.alcyone.com/max/projects/omega/.</li>'
             '<li>Our changes (curses shim, auto-explore, stairs walking, command menu, one inventory list with w/W, choice menus, tile sets, web build) '
             'are local to this port; they are not published as a repository.</li></ul>')
print('\n'.join(parts))
