  // Objects, locations and terrain
  SPACE,0,0,
  WALL,0,26,
  PORTCULLIS,33,22,
  OPEN_DOOR,32,26,
  CLOSED_DOOR,31,26,
  WHIRLWIND,42,18,
  ABYSS,7,22,
  LAVA,79,23,
  HEDGE,60,23,
  WATER,18,23, // Also RIVER
  FIRE,78,23,
  TRAP,33,10,
  LIFT,31,27,
  STAIRS_UP,42,22,
  STAIRS_DOWN,43,22,
  FLOOR,14,23, // Also ROAD
  PLAYER,0,4,
  CORPSE,45,7,
  STATUE,0,24,
  RUBBLE,55,7,
  ALTAR,34,26,
  CASH,9,7,
  PILE,12,7,
  FOOD,4,2,
  WEAPON,13,10,
  MISSILEWEAPON,60,10,
  SCROLL,75,7,
  POTION,40,8,
  ARMOR,48,0,
  SHIELD,94,0,
  CLOAK,5,0,
  BOOTS,20,0,
  STICK,28,10,
  RING,10,5,
  THING,9,6,
  ARTIFACT,42,5,
  // Terrain types
  PLAINS,9,23,
  TUNDRA,99,23,
  MOUNTAINS,117,23,
  PASS,96,21,
  CITY,84,25,
  VILLAGE,96,24,
  FOREST,54,23,
  JUNGLE,55,23,
  SWAMP,11,23,
  VOLCANO,117,22,
  CASTLE,53,26,
  TEMPLE,34,26,
  CAVES,82,22,
  DESERT,99,22,
  CHAOS_SEA,20,23,
  STARPEAK,87,23,
  DRAGONLAIR,31,12,
  MAGIC_ISLE,51,7,
  CHAIR,24,26,
  SAFE,5,7,
  FURNITURE,48,24,
  BED,25,26,
  // Monsters in effects code
  'n'|COL_RED,66,21, // nymph (and necromancer)
  's'|COL_RED,67,21, // satyr
  'I'|COL_RED,66,21, // incubus
  'S'|COL_RED,67,21, // succubus
  // Weapons in effects code
  '!'|COL_BROWN,65,1, // bolt
  '@'|COL_LIGHT_PURPLE,64,1, // ball
  '*'|COL_LIGHT_RED,68,1, // flame bolt and ball
  '^'|COL_LIGHT_BLUE,95,1, // electricity bolt and ball
  // Monsters
  'h'|COL_YELLOW,48,15, // hornet
  '@'|COL_RED,0,3, // men, priests, etc.
  'G'|COL_RED,5,4, // guardsman
  's'|COL_WHITE,75,15, // sheep
  'g'|COL_BROWN,32,19, // grunt
  't'|COL_PURPLE,56,17, // tse-tse fly
  'f'|COL_PURPLE,31,15, // fnord
  'r'|COL_BROWN,27,13, // sewer rat and rabbit
  'f'|COL_CYAN,91,14, // aggravator fungus
  'b'|COL_BRIGHT_WHITE|COL_FG_BLINK,78,14, // blipper
  'g'|COL_GREEN,88,3, // goblin
  'p'|COL_RED,54,15, // phantasticon
  'R'|COL_GREY,2,18, // household robot
  'g'|COL_WHITE,8,11, // pencil-necked geek
  'b'|COL_GREEN,8,19, // borogrove
  'q'|COL_BROWN,12,17, // quail
  'b'|COL_BROWN,25,13, // badger and bear
  'H'|COL_BROWN,37,19, // hawk
  'h'|COL_BROWN,27,19, // hyena and horse
  'd'|COL_BROWN,27,19, // deer and doberman death-hound
  'c'|COL_BROWN,27,19, // camel and radeligian cateagle
  'a'|COL_BROWN,42,16, // anteater
  't'|COL_CYAN,33,17, // trout
  'b'|COL_CYAN,34,17, // striped bass
  'p'|COL_PURPLE,15,17, // parrot
  'n'|COL_GREY,23,4, // ninjas, night gaunt and transparent nasty
  'e'|COL_GREEN,10,16, // floating eye and etheric grue
  't'|COL_GREEN,16,19, // tove
  'g'|COL_GREY,20,20, // ghost
  'e'|COL_RED,18,3, // enchanter
  'f'|COL_GREY,90,14, // murk fungus
  'G'|COL_GREEN,84,3, // goblin chieftain and goblin shaman
  'w'|COL_BROWN,76,15, // wolf
  'a'|COL_RED,21,19, // giant soldier ant
  'e'|COL_GREY,61,17, // elephant
  's'|COL_LIGHT_RED,13,19, // salamander
  'C'|COL_GREEN,30,17, // catoblepas
  'f'|COL_LIGHT_BLUE,52,13, // lesser frost demon
  'a'|COL_YELLOW,23,15, // acid cloud
  'p'|COL_GREY,21,20, // phantom
  'K'|COL_LIGHT_GREEN,82,3, // The Goblin King
  'P'|COL_PURPLE,59,14, // pterodactyl
  'l'|COL_YELLOW,77,15, // lion
  'b'|COL_RED,67,21, // brigand
  'm'|COL_RED,27,16, // mamba
  'M'|COL_BLUE,6,15, // man o' war
  'T'|COL_GREY,21,15, // thought-form
  'M'|COL_YELLOW,64,16, // manticore
  'T'|COL_BROWN,45,11, // tasmanian devil
  'a'|COL_GREY,4,18, // automatum minor
  's'|COL_GREEN,0,15, // denebian slime devil
  'j'|COL_PURPLE,36,19, // jub-jub bird
  'h'|COL_GREY,22,20, // haunt
  'c'|COL_GREEN,16,19, // giant crocodile
  't'|COL_BROWN,37,15, // torpor beast
  'F'|COL_GREY,25,15, // astral fuzzy
  'l'|COL_BLUE,30,4, // servant of law
  'c'|COL_RED,34,4, // servant of chaos
  's'|COL_YELLOW,48,15, // swarm
  'b'|COL_GREY,39,15, // ban sidhe
  'd'|COL_LIGHT_RED,13,12, // dragonette and outer circle demon
  't'|COL_LIGHT_BLUE,8,12, // tesla monster
  'W'|COL_LIGHT_RED,13,12, // wyvern and lava worm
  'f'|COL_WHITE,30,14, // frost demon
  'S'|COL_GREY,21,20, // spectre and shadow spirit
  'B'|COL_GREEN,40,21, // bog thing
  'V'|COL_GREY,70,21, // astral vampire
  'm'|COL_PURPLE,65,16, // manaburst
  'm'|COL_GREY,55,17, // mirrorshade
  'F'|COL_WHITE|COL_BG_RED,20,14, // fire elemental
  'A'|COL_WHITE|COL_BG_BLUE,21,14, // air elemental
  'W'|COL_WHITE|COL_BG_BLUE,24,14, // water elemental
  'E'|COL_WHITE|COL_BG_BROWN,23,14, // earth elemental
  'b'|COL_YELLOW|COL_BG_BROWN,71,18, // bandersnatch
  'L'|COL_BLACK|COL_BG_WHITE,35,16, // lich
  'T'|COL_LIGHT_GREEN|COL_BG_BLUE,8,19, // triton
  'T'|COL_GREEN|COL_BG_BROWN,47,16, // triceratops
  'R'|COL_YELLOW|COL_BG_WHITE,9,21, // rakshasa
  'S'|COL_GREEN|COL_BG_RED,27,16, // demon serpent
  'a'|COL_YELLOW|COL_BG_WHITE,65,21, // angel
  'n'|COL_BLACK|COL_BG_WHITE,23,4, // ninja (chunin) and ninja (jonin)
  'B'|COL_BLACK|COL_BG_BROWN,62,17, // behemoth
  'N'|COL_BLACK|COL_BG_WHITE,6,20, // nazgul
  'U'|COL_BLACK|COL_BG_WHITE,68,16, // unicorn
  'r'|COL_BLACK|COL_BG_BROWN,49,16, // rodent of unusual size
  'F'|COL_BLACK|COL_BG_WHITE,26,19, // illusory fiend
  'W'|COL_GREY|COL_BG_RED,47,17, // Great Wyrm
  'F'|COL_GREY|COL_BG_RED,36,21, // flame devil
  'S'|COL_YELLOW|COL_BG_BROWN,28,19, // sandman
  'm'|COL_RED|COL_BG_WHITE,54,17, // mirror master
  'G'|COL_GREY|COL_BG_GREEN,14,16, // elder etheric grue
  'S'|COL_GREY|COL_BG_GREEN,26,16, // loathly serpent
  'Z'|COL_BLACK|COL_BG_WHITE,26,20, // zombie overlord
  'r'|COL_GREY|COL_BG_BROWN,44,18, // ricochet
  'D'|COL_GREY|COL_BG_RED,21,12, // inner circle demon and dragon
  'f'|COL_YELLOW|COL_BG_WHITE,19,14, // fairy
  'A'|COL_GREY|COL_BG_WHITE,8,18, // automatum major
  'J'|COL_BROWN|COL_BG_RED,41,16, // jabberwock
  'F'|COL_LIGHT_BLUE|COL_BG_WHITE,37,21, // frost demon lord
  'T'|COL_YELLOW|COL_BG_BLUE,73,18, // tigershark
  'S'|COL_BLACK|COL_BG_WHITE,34,15, // shadow slayer
  'C'|COL_GREY|COL_BG_BROWN,9,19, // coma beast
  'A'|COL_YELLOW|COL_BG_WHITE,62,21, // high angel and archangel
  'J'|COL_GREY|COL_BG_BROWN,23,13, // jotun
  'i'|COL_BLACK|COL_BG_WHITE,72,21, // invisible slayer
  'W'|COL_GREEN|COL_BG_RED,29,12, // king wyvern
  'd'|COL_BLACK|COL_BG_WHITE,85,1, // deathstar
  'T'|COL_YELLOW|COL_BG_WHITE,6,4, // high thaumaturgist
  'V'|COL_BLACK|COL_BG_RED,18,20, // vampire lord
  'D'|COL_BLACK|COL_BG_RED,31,13, // demon prince
  'D'|COL_BLACK|COL_BG_WHITE,58,20, // Death
  'E'|COL_BLACK|COL_BG_WHITE,65,14, // The Eater of Magic
  'L'|COL_BRIGHT_WHITE|COL_BG_BLUE,18,3, // The LawBringer
  'D'|COL_BRIGHT_WHITE|COL_BG_RED,47,12, // The Dragon Lord
  'D'|COL_WHITE|COL_BG_RED,7,21, // The Demon Emperor
  'E'|COL_BROWN|COL_BG_WHITE,4,3, // The Elemental Lord of Earth
  'A'|COL_LIGHT_BLUE|COL_BG_WHITE,20,3, // The Elemental Lord of Air
  'W'|COL_BLUE|COL_BG_WHITE,9,3, // The Elemental Lord of Water
  'F'|COL_LIGHT_RED|COL_BG_WHITE,15,4, // The Elemental Lord of Fire
  'M'|COL_PURPLE|COL_BG_WHITE,89,21, // The Elemental Master
