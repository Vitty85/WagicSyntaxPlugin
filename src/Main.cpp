// This file is part of Wagic Plugin.
// 
// Copyright (C)2023 Vitty85 <https://github.com/Vitty85>
// 
// Wagic Plugin is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "AboutDialog.h"
#include "resource.h"
#include "PluginInterface.h"
#include "ScintillaEditor.h"
#include "menuCmdID.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stack>
#include <map>

#include <iostream>
#include <fstream>

// The Scintilla Plugin Windows Proc
WNDPROC OldPluginWndProc = nullptr;
// The handle to the current DLL Module
static HANDLE dllModule;
// Data provided by Notepad++
static NppData nppData;
// The main (left) editor
static ScintillaEditor editor1;
// The secondary (right) editor
static ScintillaEditor editor2;
// References the current editor
static ScintillaEditor editor;
// The plugin activation control flag
static bool active = false;

// Flag for debug and logger
static bool debug = false;
static std::ofstream logger;

// Forward declaration of menu callbacks
static void SetStyles();
static void CheckWagicAllLinesSyntax();
static void CheckWagicCurrentLineSyntax();
static void CheckWagicLineSyntax(int i);
static void disablePlugin();
static void showAbout();

// The menu entries for the plugin
static FuncItem menuItems[] = {
    // name, function, 0, is_checked, shortcut
    { L"Disable Online Syntax Check", disablePlugin, 0, false, nullptr },
    { L"Enable and Perform Current Line Syntax Check", CheckWagicCurrentLineSyntax, 0, false, nullptr },
    { L"Enable and Perform All Lines Syntax Check (VERY SLOW)", CheckWagicAllLinesSyntax, 0, false, nullptr },
    { L"", nullptr, 0, false, nullptr }, // Separator
	{ L"About...", showAbout, 0, false, nullptr }
};

std::vector<std::string> keywords = {
    "any", "blink", "forsrc", "duplicateall", "halfall", "notrigger", "ability", "abilitycontroller", "absorb", "add", "addtype", "novent",
    "affinity", "after", "age", "all", "allsubtypes", "altercost", "alterdevoffset", "alterenergy", "alterexperience", "altermutationcounter", 
    "alternative", "alternativecostrule", "alterpoison", "altersurvoffset", "alteryidarocount", "and", "any", "aslongas", "assorcery", 
    "attackcost", "attackcostrule", "attackedalone", "attackpwcost", "attackrule", "aura", "backside", "becomes", "becomesmonarch", 
    "becomesmonarchfoeof", "becomesmonarchof", "belong", "bestow", "bestowrule", "blockcost", "blockcostrule", "blockrule", "bloodthirst", 
    "boasted", "bonusrule", "bottomoflibrary", "bstw", "bury", "bushido", "buyback", "buybackrule", "cantargetcard", "cantbeblockedby", 
    "cantbeblockerof", "cantbetargetof", "canuntap", "card", "cards", "cascade", "castcard", "restricted", "casted", "cdaactive", "changecost", 
    "charge", "checkex", "chooseaname", "clone", "coinflipped", "coloringest", "combatphases", "combatends", "colors", "combatspiritlink", 
    "combattriggerrule", "commandzonecast", "compare", "completedungeon", "conjure", "connect", "connectrule", "continue", "controller", "copied", 
    "copiedacard", "copy", "costx", "count", "countb", "counter", "countermod", "counterremoved", "countershroud", "countersoneone",
    "countertrack", "coven", "create", "cumulativeupcost", "cycled", "damage", "deadcreart", "deadpermanent", "deathtouchrule", "defense", "delayed", 
    "delirium", "deplete", "destroy", "didattack", "didblock", "didnotcastnontoken", "didntattack", "discard", "discardbyopponent", "doboast", "doesntempty", 
    "doforetell", "donothing", "dontremove", "dontshow", "dotrain", "doubleside", "draw", "dredgerule", "duplicate", "duplicatecounters", "dynamicability", 
    "eachother", "epic", "equalto", "equalto~", "equip", "equipment", "equipped", "evicttypes", "evolve", "except", "exchangelife", "exert", "exilecast", 
    "exileimp", "exploits", "explores", "facedown", "facedup", "faceup", "fade", "fading", "fizzle", "fizzleto", "flanker", "flanking", "flashbackrule", "flip", 
    "flipped", "foelost", "fog", "forceclean", "forcedalive", "forcefield", "forcetype", "foreach", "forever", "freeze", "from", "fromplay", "frozen", "gravecast", 
    "half", "hasdead", "hasdefender", "hasexerted", "haunt", "haunted", "head", "hiddenmoveto", "hmodifer", "identify", "imprint", "imprintedcard", "ingest", 
    "itself", "kamiflip", "keepname", "kicked!", "kicker", "kickerrule", "lastturn", "legendrule", "lessorequalcreatures", "lessorequallands", "lessthan", "lessthan~", 
    "level", "librarybottom", "librarycast", "librarysecond", "librarytop", "life", "lifeleech", "lifelinkrule", "lifeloss", "lifeset", "limit", "limit^", "livingweapon", 
    "lord", "loseabilities", "loseability", "losesatype", "losesubtypesof", "lost", "loyalty", "madnessplayed", "manafaceup", "manapool", "manifest", "max", "maxcast", 
    "maxlevel", "maxplay", "meld", "meldfrom", "message", "miracle", "modbenchant", "morbid", "morecardsthanopponent", "morethan", "morethan~", "morph", "morphrule", 
    "most", "movedto", "moverandom", "moveto", "mutated", "mutateover", "mutateunder", "mutationover", "mutationunder", "myfoe", "myname", "myorigname", "myself", 
    "myturnonly", "name", "named!", "nameingest", "never", "newability", "newcolors", "newhook", "newtarget", "next", "nextphase", "ninjutsu", "noevent", "nolegend", 
    "nonbasicland", "nonstatic", "noreplace", "normal", "notatarget", "notblocked", "notdelirum", "notexerted", "notpaid", "notrg", "notrigger", "once", "oneonecounters", 
    "oneshot", "only", "opponent", "opponentdamagedbycombat", "opponentpoisoned", "opponentpool", "opponentscontrol", "opponentturnonly", "options", "out", "outnumbered", 
    "overload", "overloadrule", "ownerscontrol", "paid", "paidmana", "pay", "payzerorule", "persistrule", "phaseaction", "phaseactionmulti", "phasealter", "phasedin", 
    "phaseout", "placefromthetop", "planeswalkerattack", "planeswalkerdamage", "planeswalkerrule", "plus", "poolsave", "positive", "powermorethancontrollerhand", 
    "powermorethanopponenthand", "prevent", "preventallcombatdamage", "preventalldamage", "preventallnoncombatdamage", "previous", "producecolor", "produceextra", "proliferate", 
    "propagate", "provoke", "prowl", "pumpboth", "pumppow", "pumptough", "putinplay", "putinplayrule", "raid", "rampage", "randomcard", "rebound", "reconfigure", "reduce", 
    "reduceto", "regenerate", "rehook", "reject", "remake", "removeallcounters", "removealltypes", "removecreaturesubtypes", "removedfromgame", "removefromcombat", "removemana", 
    "removemc", "removesinglecountertype", "removetypes", "repeat", "resetdamage", "restricted", "restriction", "result", "retarget", "retrace", "retracerule", "return", "reveal", 
    "revealend", "revealtype", "revealuntil", "revealzone", "revolt", "sacrifice", "scry", "scryend", "scryzone", "selectmana", "serumpowder", "setblocker", "sethand", "setpower", 
    "settoughness", "shackle", "shuffle", "sidecast", "single", "skill", "soulbondrule", "source", "sourceinplay", "sourcenottap", "sourcetap", "spellmover", "spent", "spiritlink", 
    "srccontroller", "srcopponent", "standard", "steal", "surveil", "suspended", "suspendrule", "swap", "tail", "tails", "takeninitiativeof", "taketheinitiative", "tap", "target", 
    "targetcontroller", "targetopponent", "teach", "terminate", "text", "thatmuch", "this", "thisforeach", "thisturn", "time", "toughnesslifegain", "to", "token", "tokencleanuprule", 
    "tokencreated", "tosrc", "total", "trained", "trainer", "transforms", "trigger", "turn", "turnlimited", "turns", "tutorial", "type", "type", "uent", "ueot", "undocpy", "unearthrule",
    "untap", "upcost", "upcostmulti", "uynt", "vampired", "vampirerule", "vanishing", "while", "winability", "wingame", "with", "withenchant", "won", "zerodead", "zone", "if", "then", 
    "else", "activate", "unattach", "choice", "may", "foreach", "aslongas", "beforenextturn", "~morethan~", "~lessthan~", "~equalto~", "other", "loseabilityend", "rolld", "end", 
    "afterrevealed", "afterrevealedend", "variable", "winabilityend", "and", "chooseanameopp", "chooseaname", "chooseend", "nonland", "upkeep", "optionone", "optiononeend", 
    "optiontwo", "optiontwoend", "my", "ifnot", "blockers", "combatbegins", "multi", "powerstrike", "can", "play", "endofturn", "replacedraw", "cycle", "nextphasealter", "scrycore", 
    "scrycoreend", "named", "beginofturn", "firstmain", "livingweapontoken", "flipacoin", "flipend", "chooseatype", "drawonly", "firstmainonly", "secondmain", "chooseacolor", 
    "retargetfromplay", "attackersonly", "blockersonly", "combatdamageonly", "combatendsonly", "combatbeginsonly", "activatechooseacolor", "activatechooseend", "powerpumpboth", 
    "targetedpersonsbattlefield", "myattackersonly", "combat", "manacostlifestrike", "chargedraw", "toughnesslifeloss", "manastrike", "activatedability", "each", "remove", 
    "manacostlifeloss", "combatbegin", "combatattackers", "combatblockers", "combatend", "grant", "grantend", "opponentupkeeponly", "myupkeeponly", "manacostlifestrike", 
    "toughnesslifeloss", "main", "powerpumppow", "manacostlifelose", "opponentreplacedraw", "chosencolorend", "activatechooseatype", "combatphaseswithmain", "agecountersoneone", 
    "cantblocktarget", "endstep", "before", "attackers", "during", "dredge", "cleanup", "manacostlifegain", "opponentblockersonly", "powerlifegain", "cumulativeupcostmulti", 
    "sourcenottapped", "oneonecountersstrike", "powercountersoneone", "powerlifeloss", "manacoststrike", "chargelifegain", "myupkeep", "untaponly", "toughnesstrike", "chargedeplete", 
    "chargestrike", "manacostpumppow", "didcombatdamagetofoe", "manacostpumpboth", "manacostpumptough", "powerdraw", "colorspumpboth", "postbattle", "nonwall", "value", "twist", 
    "emblem", "combatdamage"
    // Add any additional Wagic keyword here
};

// List of Wagic zones
std::vector<std::string> zones = {
    "mycastingzone", "myrestrictedcastingzone", "mycommandplay", "myhandlibrary", "mygravelibrary", "opponentgravelibrary", "mygraveexile",
    "opponentgraveexile", "opponentcastingzone", "opponentrestrictedcastingzone", "opponentcommandplay", "opponenthandlibrary",
    "mynonplaynonexile", "opponentnonplaynonexile", "myhandexilegrave", "opponenthandexilegrave", "myzones", "opponentzones",
    "mysideboard", "mycommandzone", "myreveal", "mygraveyard", "mybattlefield", "myhand", "mylibrary", "mystack", "myexile",
    "opponentsideboard", "opponentcommandzone", "opponentreveal", "opponentgraveyard", "opponentbattlefield", "opponenthand",
    "opponentlibrary", "opponentstack", "opponentexile","ownersideboard", "ownercommandzone", "ownerreveal", "ownergraveyard",
    "ownerbattlefield", "ownerinplay", "ownerhand", "ownerlibrary", "ownerstack", "ownerexile", "sideboard", "commandzone", "reveal",
    "graveyard", "battlefield", "inplay", "hand", "library", "nonbattlezone", "stack", "exile", "previousbattlefield",
    "targetcontrollerbattlefield", "targetedpersonslibrary", "targetedplayerbattlefield", "targetcontrollergraveyard", "targetedpersonsstack", 
    "targetedpersonsgraveyard", "targetedpersonshand", "targetcontrollerlibrary"
    // Add any additional Wagic zone here
};

// List of Wagic triggers
std::vector<std::string> triggers = {
    "@tappedformana", "@becomesmonarchof", "@drawof", "@combat", "@boasted", "@targeted", "@lifelostfoeof",
    "@noncombatdamagefoeof", "@takeninitiativeof", "@energizedof", "@counteradded", "@totalcounteradded",
    "@scryed", "@producedmana", "@becomesmonarchfoeof", "@dierolled", "@damageof", "@sacrificed", "@combatdamaged",
    "@shuffledfoeof", "@cycled", "@poisonedfoeof", "@foretold", "@energizedfoeof", "@discarded", "@tokencreated",
    "@phasedin", "@untapped", "@drawfoeof", "@combatdamagefoeof", "@exploited", "@poisonedof", "@exerted", "@movedTo",
    "@experiencedof", "@counterremoved", "@vampired", "@countermod", "@experiencedfoeof", "@shuffledof", "@lifeof",
    "@noncombatdamaged", "@trained", "@defeated", "@dungeoncompleted", "@surveiled", "@damagefoeof", "@combatdamageof",
    "@coinflipped", "@noncombatdamageof", "@movedto", "@damaged", "@explored", "@lifefoeof", "@lifelostof", "@transformed",
    "@facedup", "@lord", "@drawn", "@castcard", "@tapped", "@mutated", "@each endofturn", "@each beginofturn", "@each upkeep", 
    "@each firstmain", "@each combatbegins", "@each attackers", "@each blockers", "@each combatdamage", "@each combatends", 
    "@each secondmain", "@each end", "@each cleanup", "@each untap", "@each my endofturn", "@each my beginofturn", "@each my upkeep",
    "@each my firstmain", "@each my combatbegins", "@each my attackers", "@each my blockers", "@each my combatdamage", "@each my combatends",
    "@each my secondmain", "@each my end","@each my cleanup","@each my untap", "@each opponent endofturn", "@each opponent beginofturn", 
    "@each opponent upkeep", "@each opponent firstmain", "@each opponent combatbegins", "@each opponent attackers", "@each opponent blockers", 
    "@each opponent combatdamage", "@each opponent combatends", "@each opponent secondmain", "@each opponent end", "@each opponent cleanup", 
    "@each opponent untap", "@next endofturn", "@next beginofturn", "@next upkeep", "@next firstmain", "@next combatbegins", "@next attackers", 
    "@next blockers", "@next combatdamage", "@next combatends", "@next secondmain", "@next end", "@next cleanup", "@next untap"
    // Add any additional Wagic trigger here
};

// List of Wagic macros
std::vector<std::string> macros = {
    "__CYCLING__", "__BASIC_LANDCYCLING__", "_DIES_", "_TRAINING_", "_PARTNER_", "_GOAD_", "_REBOUND_", "_POPULATE_", "_FEROCIOUS_",
    "_ATTACKING_", "_BLOCKED_", "_HEROIC_", "_RALLY_", "_LANDFALL_", "_ADDENDUM_", "_CONSTELLATION_", "_AMASS_", "_SCRY_", "_SCRY1_",
    "_SCRY2_", "_SCRY3_", "_SCRY4_", "_SCRY5_", "_FABRICATE_", "_ENRAGE_", "_SECOND_DRAW_", "_ADAPT_", "_BATTALION_", "_CHAMPION_",
    "_METALCRAFT_", "_ECHO_", "_THRESHOLD_", "_SPLICEARCANE_", "_RIPPLE_", "_RECOVER_", "_CLASH_", "_PROLIFERATE_", "_SCAVENGE_",
    "_MONSTROSITY_", "_OUTLAST_", "_INVESTIGATE_", "_ASCEND_", "_CITY'S_BLESSING_","_MONARCH_CONTROLLER_""_MONARCH_OPPONENT_",
    "_INITIATIVE_CONTROLLER_""_INITIATIVE_OPPONENT_", "_EXPLORE_", "_TREASURE_", "_CASTHISTORIC_", "_MENTOR_", "_SURVEIL1_",
    "_SURVEIL2_", "_SURVEIL3_", "_UNDERGROWTH_", "_AFTERLIFETOKEN_", "_RIOT_", "_LEARN_", "_SPECTACLE_", "_ADVENTURE_", "_EXTORT_",
    "_FORETELL_", "_LOOT_", "_UNEARTH_", "__PLAY_TOP_FROM_EXILE__", "_WARD_", "_RENOWN_", "_BLINK_UEOT_", "_CONNIVES_", "_ETERNALIZE_",
    "_DISCARD&DRAW_", "_PUNCH_", "_MUST_BE_BLOCKD_", "_ANGELTOKEN_", "_BEASTTOKEN_", "_DRAGONTOKEN_", "_ELEPHANTTOKEN_", "_GOBLINTOKEN_",
    "_INSECTTOKEN_", "_KNIGHTTOKEN_", "_PHYREXIANMITETOKEN_", "_REDELEMENTALTOKEN_", "_SAPROLINGTOKEN_", "_SERVOTOKEN_", "_SOLDIERTOKEN_",
    "_SPIRITTOKEN_", "_THOPTERTOKEN_", "_WOLFTOKEN_", "_ZOMBIETOKEN_", "_HARNESSED_CONTROL_", "_HARNESSED_LIGHTNING_"
    // Add any additional Wagic macro here
};

// List of Wagic constants
std::vector<std::string> constants = {
    "abundantlife", "allbattlefieldcardtypes", "allgravecardtypes", "allmyname", "anycnt", "auras", "azorius", "boros", "bothalldead", "bushidopoints", "calculateparty", 
    "canforetellcast", "cantargetcre", "cantargetmycre", "cantargetoppocre", "cardcountabil", "cardcounttype", "castx", "chosencolor", "chosenname", "chosentype", "commonblack", 
    "commonblue", "commongreen", "commonred", "commonwhite", "controllerturn", "converge", "convertedcost:", "countallspell", "countedamount", "countedbamount", "countmycrespell", 
    "countmynoncrespell", "crewtotalpower", "currentphase", "currentturn", "cursedscrollresult", "diffcardcountabil", "diffcardcounttype", "dimir", "direction", "dualfaced", 
    "epicactivated", "evictb", "evictg", "evictmc", "evictpw", "evictr", "evictth", "evictu", "evictw", "excessdamage", "findfirsttype", "findlasttype", "fivetimes", "fourtimes", 
    "fullpaid", "gear", "genrand", "golgari", "gruul", "halfdown", "halfpaid", "halfup", "handsize", "hasability", "hascnt", "hasevict", "hasmansym", "hasprey", "hasstorecard", 
    "highest", "highestlifetotal", "iscopied", "isflipped", "ishuman", "izzet", "kicked", "lastdiefaces", "lastflipchoice", "lastflipresult", "lastrollchoice", "lastrollresult", 
    "lifegain", "lifelost", "lifetotal", "lowestlifetotal", "magusofscrollresult", "math", "mathend", "minus", "minusend", "mutations", "mybattlefieldcardtypes", "myblackpoolcount", 
    "mybluepoolcount", "mycolnum", "mycolorlesspoolcount", "mygravecardtypes", "mygreenpoolcount", "myname", "mypoisoncount", "mypoolcount", "mypos", "myredpoolcount", 
    "mysnowblackpoolcount", "mysnowbluepoolcount", "mysnowcolorlesspoolcount", "mysnowgreenpoolcount", "mysnowpoolcount", "mysnowredpoolcount", "mysnowwhitepoolcount", "mytarg", 
    "mywhitepoolcount", "numofcommandcast", "numoftypes", "oalldead", "oattackedcount", "ocoven", "ocycledcount", "odcount", "odevotionoffset", "odiffinitlife", "odnoncount", "odomain", 
    "odrewcount", "odungeoncompleted", "oenergy", "oexperience", "ohalfinitlife", "ohandcount", "oinitiative", "oinstsorcount", "olandb", "olandc", "olandg", "olandr", "olandu", 
    "olandw", "olastshlturn", "olibrarycount", "omonarch", "onumcreswp", "onumofcommandcast", "onumofidentitycols", "oplifegain", "oplifelost", "oppbattlefieldcardtypes", 
    "oppgravecardtypes", "oppofindfirsttype", "oppofindlasttype", "opponentblackpoolcount", "opponentbluepoolcount", "opponentcolorlesspoolcount", "opponentgreenpoolcount", 
    "opponentlifetotal", "opponentpoisoncount", "opponentpoolcount", "opponentredpoolcount", "opponentsnowblackpoolcount", "opponentsnowbluepoolcount", "opponentsnowcolorlesspoolcount", 
    "opponentsnowgreenpoolcount", "opponentsnowpoolcount", "opponentsnowredpoolcount", "opponentsnowwhitepoolcount", "opponentturn", "opponentwhitepoolcount", "oppototalcololorsinplay", 
    "oppototalmana", "oppsametypecreatures", "orzhov", "ostartinglife", "ostormcount", "osurveiloffset", "otherconvertedcost", "otherpower", "othertoughness", "othertype", "oyidarocount", 
    "palldead", "pancientooze", "pattackedcount", "pbasiclandtypes", "pcoven", "pcycledcount", "pdauntless", "pdcount", "pdevotionoffset", "pdiffinitlife", "pdnoncount", "pdomain", 
    "pdrewcount", "pdungeoncompleted", "penergy", "permanent", "pexperience", "pgbzombie", "pginstantsorcery", "pgmanainstantsorcery", "phalfinitlife", "phandcount", "pinitiative", 
    "pinstsorcount", "plandb", "plandc", "plandg", "plandr", "plandu", "plandw", "plastshlturn", "plibrarycount", "plusend", "pmonarch", "pnumcreswp", "pnumofcommandcast", 
    "pnumofidentitycols", "power", "powertotalinplay", "prex", "prodmana", "pstormcount", "psurveiloffset", "pwr", "pwrtotalinplay", "pwrtotatt", "pwrtotblo", "pyidarocount", "rakdos", 
    "restriction{", "revealedmana", "revealedp", "revealedt", "sametypecreatures", "scryedcards", "selesnya", "simic", "snowdiffmana", "srclastdiefaces", "srclastrollchoice",
    "srclastrollresult", "startinglife", "startingplayer", "stored", "sunburst", "targetedcurses", "thatmuch", "thirddown", "thirdpaid", "thirdup", "thrice", "ths:", "thstotalinplay", 
    "thstotatt", "thstotblo", "totalcololorsinplay", "totaldmg", "totalmana", "totcnt", "totcntall", "totcntart", "totcntbat", "totcntcre", "totcntenc", "totcntlan", "totcntpla", 
    "totmanaspent", "toughness", "toughnesstotalinplay", "treefolk", "twice", "urzatron", "usedmana", "withpartner", "worshipped", "colorless", "green", "blue", "red", "black", "white", 
    "waste", "alternative", "buyback", "flashback", "retrace", "kicker", "facedown", "bestow","anyamount","attacking", "backname", " blockable", "blocked", "blocking", "cantarget ", 
    "cardid", "children", "cmana", "color ", "controllerdamager", "damaged", "damager", "description", "discarded", "dredgeable", "enchanted", "equals", "equipped", "evictname", 
    "exerted", "expansion", "extracostshadow", "foretold", "geared", "hasbackside", "hasconvoke", "hasflashback", "haskicker", "haspartner", "hasx", "icon", "lastnamechosen", "leveler", 
    "manab", "manacost", "manag", "manar", "manau", "manaw", "mtgid", "multicolor", "mychild", "mycurses", "myeqp", "mysource", "mytgt", "mytotem", "notshare!", "numofcols", 
    "opponentdamager", "pairable", "parents", "partname", "preyname", "proliferation", "rarity", "recent", "shadow", "share", "sourcecard", "storedname", "tapped", "targetedplayer", 
    "targetter", "title", "unknown", "upto", "zpos", "notany", "modified", "battleready", "storedmanacost", "storedpower", "storedtoughness", "storedx", "storedthatmuch", 
    "storedmanacostplus", "storedhalfdownpower", "mystored", "storedhascnt", "mygraveyardplustype", "mybattlefieldplusend", "mybattlefieldplustype", "opponentbattlefieldplusend", 
    "findfirsttypenonland", "hascntacolyteffect", "share", "types", "hascnttime", "twicetype", "findfirsttypecreature", "lifetotalminusstartinglifeminusend", "controllerlife", 
    "currentturnplus", "twicepower", "countedamountplus", "hascntperpetualcollector", "findfirsttypecreature", "halfdownusedmanau", "halfdownusedmanab", "halfdownusedmanar", 
    "convertedcost", "untapped", "opponentbattlefieldminustype", "mybattlefieldminusend", "hascntalphaeffect", "myexileplustype", "mygraveyardplusend", "mybattlefieldminus",
    "bothalldeadcreature", "hascntilluminatoreffect", "battlefieldplustype", "stackplusend", "halfdownlifetotal", "hascntchance", "hascntcosimaeffect", "mygravecardtypesplus", 
    "ohandcountplustype", "opponentbattlefieldplusendminus", "phandcountplustype", "mybattlefieldplusendminus", "hascntcorpse", "halfuptype", "mybattlefieldplus", 
    "hasabilitytwoboastplus", "pbasiclandtypesplus", "psurveiloffsetplus", "myexileplusend", "lastrollresultplustoughnessplusend", "lastrollresultplustype", "myhandplusend", 
    "myhandplustype", "twicecardcountabiltwodngtrgplus", "pnumofcommandcastplus", "mylibraryplustype", "lastrollresultplusmanacostplusend", "opponentbattlefieldplus",
    "myhandplus", "fullpaidplus", "halfpaidplus", "mygreenpoolcountplus", "kickedplus", "manacostplus", "pginstantsorceryplustype", "graveyardplus", "calculatepartyplus", "pplus",
    "plushascntperpetualchargeplusend", "pbasiclandtypespluspbasiclandtypesplusend", "pinitiativeplusoinitiativeplusend", "gearplusaurasplusend", "manacostminus", "pminus", "xminus", 
    "lastrollresultminus", "powerminus", "minusphandcountminusend", "opponentbattlefieldminus", "lastrollresultminuslastrollchoiceminusend", "countedamountminus", "countedbamountminus", 
    "mybattlefieldminustype", "opponentbattlefieldminusend", "myhandminustype", "opponenthandminusend", "myhandminus", "mygraveyardminus", "lifetotalminusopponentlifetotalminusend", 
    "opponentlifetotalminuslifetotalminusend", "tminus", "ohandcountminusphandcountminusend", "phandcountminusohandcountminusend", "opponenthandminustype", "myhandminusend", 
    "phandcountminus", "lastrollresultminusphandcountminusend", "plibrarycountminus", "battlefieldminus", "opponentbattlefielddminus", "odcountminusodnoncountminusend", 
    "pbasiclandtypesminus", "hascntbstreduce", "hascntartificereffect", "hascntlevel", "hascntdscrd", "hascntcluesac", "hascntcodieeffect", "hascntwind", "hascntplot", 
    "hascntperpetualtapped", "hascntdamagedealt", "hascntexplore", "hascntbloodline", "hascntcharge", "hascntjudgment", "hascntperpetualdragoncostless", "hascntanycnt",
    "hascntaim", "hascntritual", "hascntsac", "hascntlif", "hascnttra", "hascntsoul", "hascnthymneffect", "hascntharmony", "hascntinvasionexiled", "hascnttreastoken",
    "hascntgoblinliar", "hascntminetunnels", "hascntreprieve", "hascntflame", "hascntmarchesaeffect", "hascntadvocateeffect", "hascntmeriekestolen", "hascntexperience",
    "hascntfirststrike", "hascntmonkeffect", "hascntnineeffect", "hascntincarnation", "hascntperpetualzombie", "hascntperpetualominous", "hascntopaleffect", "hascntpatheffect",
    "hascntplasmeffect", "hascntpreachereffect", "hascntquest", "hascntecho", "hascntblood", "halfdownhascntblood", "hascntnight", "hascntfade", "hascntsarevokeffect",
    "hascntlifenoted", "hascntember", "hascntloyalty", "hascntvoid", "plushascntperpetualchargeplusend", "hascnthellkiteeffect", "hascnthellkiteoppoeffect", "hascntpoint",
    "hascntsynthesiseffect", "hascntfaitheffect", "hascntlore", "hascntdepletedone", "hascntoubliette", "hascntforge", "hascntlostwell", "hascnttrap", "hascntstash",
    "hascntarena", "hascnthone", "hascntvisionseffect", "hascntinvitation", "hascnteffigyeffect", "findfirsttypeartifact", "findfirsttypepermanent", "oppofindfirsttypecreature", 
    "oppofindfirsttypenonpermanent", "findfirsttypenonpermanent", "oppofindfirsttypenonland", "findfirsttypeelemental", "findfirsttypeelf", "findfirsttypeland", "oppofindfirsttypeland", 
    "findfirsttypeplaneswalker", "findfirsttypeenchantment", "hascntoil", "hascntomen", "fivetimesthirdpaid", "thirdupopponentlifetotal", "thirduplifetotal", "thirduptype",
    "halfupstartinglife", "halfuplifetotal", "halfupopponentlifetotal", "halfsuptartinglife", "halfdownopponentlifetotal", "halfupcurrentturn", "halfdownstartinglife",
    "diffcardcounttypeland", "diffcardcounttypedemon", "diffcardcounttypegate", "twicethatmuch", "twicefullpaid", "storedtwicefullpaid", "twicecalculateparty", "twicestoredx",
    "hasmansymw", "hasmansymr", "hasmansymg", "hasmansymu", "hasmansymb", "mypoolsave", "prexx", "opponentdamagecount", "thatmuchcountersoneone", "totcntplaloyalty",
    "mytargx", "mytargkicked", "thricetype", "thricepower", "plifelost", "poisoncount", "pstormcountplus", "battlefieldplus", "allgravecardtypesplus", "xplus", "targetedpersonshandminus", 
    "opponenthandminusendmathend", "myhandminusendmathend", "opponenthandminus", "minustype", "mathlifetotalminusopponentlifetotalminusendmathend", "targetedpersonshandminusend",
    "opponentpoolsave","mathtype", "opponenthandminusendmathend", "myhandminusendmathend", "hascnttower", "thricekicked", "lowest", "myexilepluspginstantsorceryplusend",
    "hascntdavrieleffect", "hascntjaceeffect", "hascntghostform", "minusoplifelostminusend", "twicepdrewcount", "minusohandcountminusend", "manacostminus4minusend"
    // Add any additional Wagic constant here
};

// List of Wagic basic abilities
std::vector<std::string> basicabilities = {
    "trample", "forestwalk", "islandwalk", "mountainwalk", "swampwalk", "plainswalk", "flying", "first", "double", "strike", "fear", "flash", "haste", "lifelink",
    "reach", "shroud", "vigilance", "defender", "banding", "protection", "protection from green", "protection from blue", "protection from red", "protection from black", 
    "protection from white", "unblockable", "wither", "persist", "retrace", "exalted", "nofizzle", "shadow", "reachshadow", "foresthome", "islandhome", "mountainhome", "swamphome", 
    "plainshome", "cloud", "cantattack", "mustattack", "cantblock", "doesnotuntap", "opponentshroud", "indestructible", "intimidate", "deathtouch", "horsemanship", "cantregen", 
    "oneblocker", "infect", "poisontoxic", "poisontwotoxic", "poisonthreetoxic", "phantom", "wilting", "vigor", "changeling", "absorb", "treason", "unearth", "cantlose", "cantlifelose", 
    "cantmilllose", "snowlandwalk", "nonbasiclandwalk", "strong", "storm", "phasing", "split", "second", "weak", "affinityartifacts", "affinityplains", "affinityforests", 
    "affinityislands", "affinitymountains", "affinityswamps", "affinitygreencreatures", "cantwin", "nomaxhand", "leyline", "playershroud", "controllershroud", "sunburst", "flanking", 
    "exiledeath", "legendarylandwalk", "desertlandwalk", "snowforestlandwalk", "snowplainslandwalk", "snowmountainlandwalk", "snowislandlandwalk", "snowswamplandwalk", "canattack", 
    "hydra", "undying", "poisonshroud", "noactivatedability", "notapability", "nomanaability", "onlymanaability", "poisondamager", "soulbond", "lure", "nolegend", "canplayfromgraveyard", 
    "tokenizer", "mygraveexiler", "oppgraveexiler", "librarydeath", "shufflelibrarydeath", "offering", "evadebigger", "spellmastery", "nolifegain", "nolifegainopponent", "auraward", 
    "madness", "protectionfromcoloredspells", "mygcreatureexiler", "oppgcreatureexiler", "zerocast", "trinisphere", "canplayfromexile", "libraryeater", "devoid", "cantchangelife", 
    "combattoughness", "cantpaylife", "cantbesacrified", "skulk", "menace", "nosolo", "mustblock", "dethrone", "overload", "shackler", "flyersonly", "tempflashback", "legendruleremove", 
    "canttransform", "asflash", "conduited", "canblocktapped", "oppnomaxhand", "cantcrew", "hiddenface", "anytypeofmana", "necroed", "cantpwattack", "canplayfromlibrarytop", 
    "canplaylandlibrarytop", "canplaycreaturelibrarytop", "canplayartifactlibrarytop", "canplayinstantsorcerylibrarytop", "showfromtoplibrary", "showopponenttoplibrary", "totemarmor", 
    "discardtoplaybyopponent", "modular", "mutate", "adventure", "mentor", "prowess", "nofizzle alternative",  "hasotherkicker", "partner", "canbecommander", "iscommander", 
    "threeblockers", "handdeath", "inplaydeath", "inplaytapdeath", "gainedexiledeath", "gainedhanddeath", "cycling", "foretell", "anytypeofmanaability", "boast", "twoboast", 
    "replacescry", "hasnokicker", "undamageable", "lifefaker", "doublefacedeath", "gaineddoublefacedeath", "twodngtrg", "nodngopp", "nodngplr", "canplayauraequiplibrarytop", 
    "counterdeath", "dungeoncompleted", "perpetuallifelink", "perpetualdeathtouch", "noncombatvigor", "nomovetrigger", "wascommander", "showopponenthand", "showcontrollerhand", 
    "hasreplicate", "isprey", "hasdisturb", "daybound", "nightbound", "decayed", "hasstrive", "isconspiracy", "hasaftermath", "noentertrg", "nodietrg", "training", "energyshroud", 
    "expshroud", "countershroud", "nonight", "nodamageremoved", "backgroundpartner", "bottomlibrarydeath", "noloyaltydamage", "nodefensedamage", "affinityallcreatures", 
    "affinitycontrollercreatures", "affinityopponentcreatures", "affinityalldeadcreatures", "affinityparty", "affinityenchantments", "affinitybasiclandtypes", "affinitytwobasiclandtypes", 
    "affinitygravecreatures", "affinityattackingcreatures", "affinitygraveinstsorc", "canloyaltytwice", "attached", "fresh", "snowplainswalk", "snowislandwalk", "snowswampwalk", 
    "snowmountainwalk", "snowforestwalk", "desertwalk", "renown"
    // Add any additional Wagic basic ability here
};

// List of Wagic card types
std::vector<std::string> types = {
    "abian", "abyss", "advisor", "aetherborn", "ajani", "alara", "ally", "aminatou", "amonkhet", "angel", "angrath", "antausia", "antelope", "ape",
    "arcane", "arcavios", "archer", "archon", "arkhos", "arlinn", "art", "artifact", "artificer", "ashiok", "assassin", "assembly", "atog", "aura",
    "aurochs", "autobot", "avatar", "azgol", "azra", "background", "baddest", "badger", "bahamut", "barbarian", "bard", "basic", "basilisk", "basri",
    "bat", "battle", "bear", "beast", "beaver", "beeble", "beholder", "belenon", "berserker", "biggest", "bird", "blood", "boar", "bolas", "brainiac",
    "bringer", "brushwagg", "bureaucrat", "calix", "camel", "capenna", "carrier", "cartouche", "cat", "centaur", "cephalid", "chameleon", "chandra",
    "chicken", "child", "chimera", "citizen", "clamfolk", "class", "cleric", "clue", "cockatrice", "conspiracy", "construct", "contraption", "cow",
    "coward", "crab", "creature", "cridhe", "crocodile", "curse", "cyborg", "cyclops", "dack", "dakkon", "daretti", "dauthi", "davriel", "deer",
    "demigod", "demon", "desert", "designer", "devil", "dihada", "dinosaur", "djinn", "dog", "dominaria", "domri", "donkey", "dovin", "dragon",
    "drake", "dreadnought", "drone", "druid", "dryad", "duck", "dungeon", "dwarf", "eaturecray", "echoir", "efreet", "egg", "elder", "eldraine",
    "eldrazi", "elemental", "elephant", "elf", "elk", "ellywick", "elminster", "elspeth", "elves", "enchant", "enchantment", "equilor",
    "equipment", "ergamon", "estrid", "etiquette", "eye", "fabacin", "faerie", "ferret", "fiora", "fish", "flagbearer", "food", "forest", "fox",
    "fractal", "freyalise", "frog", "fungus", "gamer", "gargantikar", "gargoyle", "garruk", "gate", "giant", "gideon", "gith", "gnoll", "gnome",
    "goat", "gobakhan", "goblin", "god", "golem", "gorgon", "grandchild", "gremlin", "griffin", "grist", "gus", "hag", "halfling", "harpy", "hatificer",
    "head", "hellion", "hero", "hippo", "hippogriff", "homarid", "homunculus", "horror", "horse", "host", "huatli", "human", "hydra", "hyena",
    "igpay", "ikoria", "illusion", "imp", "incarnation", "incubator", "insect", "instant", "interrupt", "inzerva", "iquatana", "ir", "island",
    "ixalan", "jace", "jackal", "jared", "jaya", "jellyfish", "jeska", "juggernaut", "kaladesh", "kaldheim", "kamigawa", "kangaroo", "karn", "karsus",
    "kasmina", "kavu", "kaya", "kephalai", "killbot", "kinshala", "kiora", "kirin", "kithkin", "knight", "kobold", "kolbahan", "kor", "koth", "kraken",
    "krav", "kylem", "kyneth", "lady", "lair", "lamia", "lammasu", "land", "lathiel", "leech", "legend", "legendary", "lesson", "leviathan", "lhurgoyf",
    "licid", "liliana", "lizard", "locus", "lolth", "lord", "lorwyn", "lukka", "luvion", "manticore", "master", "masticore", "meditation", "mercadia",
    "mercenary", "merfolk", "metathran", "mime", "mine", "minion", "minotaur", "minsc", "mirrodin", "moag", "mole", "monger", "mongoose", "mongseng", "monk",
    "monkey", "moonfolk", "mordenkainen", "mountain", "mouse", "mummy", "muraganda", "mutant", "myr", "mystic", "naga", "nahiri", "narset", "nastiest",
    "nautilus", "nephilim", "new", "nightmare", "nightstalker", "niko", "ninja", "nissa", "nixilis", "noble", "noggle", "nomad", "nymph", "octopus",
    "ogre", "oko", "ongoing", "ooze", "orc", "orgg", "otter", "ouphe", "ox", "oyster", "pangolin", "paratrooper", "peasant", "pegasus", "penguin",
    "pest", "phelddagrif", "phenomenon", "phoenix", "phyrexia", "phyrexian", "pilot", "pirate", "plains", "plane", "planeswalker", "plant", "player",
    "power", "powerstone", "praetor", "processor", "proper", "pyrulea", "rabbit", "rabiah", "raccoon", "ral", "ranger", "rat", "rath", "ravnica", "realm",
    "rebel", "reflection", "regatha", "rhino", "rigger", "rogue", "rowan", "rune", "sable", "saga", "saheeli", "salamander", "samurai", "samut", "saproling",
    "sarkhan", "satyr", "scarecrow", "scheme", "scientist", "scorpion", "scout", "segovia", "serpent", "serra", "shade", "shaman", "shandalar", "shapeshifter",
    "shark", "sheep", "shenmeng", "ship", "shrine", "siege", "siren", "skeleton", "slith", "sliver", "slug", "snake", "snow", "soldier", "soltari", "sorcery",
    "sorin", "spawn", "specter", "spellshaper", "sphere", "sphinx", "spider", "spike", "spirit", "sponge", "spy", "squid", "squirrel", "starfish", "summon",
    "surrakar", "survivor", "swamp", "szat", "tamiyo", "tarkir", "tasha", "teferi", "tentacle", "tetravite", "teyo", "tezzeret", "thalakos", "theros", "thopter",
    "thrull", "tibalt", "tiefling", "tower", "townsfolk", "trap", "treasure", "treefolk", "tribal", "trilobite", "triskelavite", "troll", "turtle", "tyvar",
    "ugin", "ulgrotha", "unicorn", "urza", "valla", "vanguard", "vampire", "vampyre", "vedalken", "vehicle", "venser", "viashino", "villain", "vivien", "volver",
    "vraska", "vryn", "waiter", "wall", "walrus", "warlock", "warrior", "weird", "werewolf", "whale", "wildfire", "will", "windgrace", "wizard", "wolf",
    "wolverine", "wombat", "worker", "world", "worm", "wraith", "wrenn", "wrestler", "wurm", "xenagos", "xerex", "yanggu", "yanling", "yeti", "zariel",
    "zendikar", "zhalfir", "zombie", "zubera", "creature", "instant", "tobecast", "sur", "tomb", "of", "annihilation", "agadeem", "the", "undercrypt",
    "tobereturn", "marauder", "airlift", "chaplain", "valiant", "protector", "akoum", "teeth", "watchdog", "alpine", "watchdog", "igneous", "cur", 
    "whispering", "raven", "ancestral", "anger", "day", "night", "amplifire", "destiny", "marauders", "anthem", "treasureartifacttoken", "inkling",
    "disenchant", "terror", "braingeyser", "shivan", "regrowth", "lotus", "br", "monstrous","Arni", "Brokenbrow", "bloodletting", "phandelver", "affected",
    "gathering", "goliath", "crossbreed", " labs", "watermark", "ru", "genesis", "mage", "brambleweft", "behemoth", "gladewalker", "ritualist", "jiang",
    "halfdowntype", "halfdownx", "halfupx", "halfuppower", "twicemyname", "twicex", "twicekicked", "twiceothertype", "twicepbasiclandtypes", "m",
    "control", "two", "or", "more", "vampires", "gr", "rb", "peer", "through", "depths", "mists", "kgoblin", "kfox", "kmoonfolk", "krat", "ksnake",
    "a", "spell", "aether", "burst", "kjeldoran", "war", "cry", "one", "kind", "saclands", "less", "creatures", "artifacts", "enchantments", "lands",
    "t", "r", "b", "u", "d", "s", "w", "g", "l", "x", "e", "c", "p", "gw", "rw", "wu", "gu", "bg", "wb", "rg", "h", "i", "ub", "q", "ur", "xx", "n",
    "color", "kindle", "scion", "skyshipped", "splinter", "stangg", "twin", "sunweb"
    // Add any additional Wagic types here
};

std::vector<std::string> allVectors;

static bool containsWordBetween(const std::string openingTag, const std::string closingTag, const std::string& line, const std::string& word, const int pos) {
    std::string wordToCheck = word;
    std::string lineToCheck = line;
    wordToCheck.erase(wordToCheck.find_last_not_of(" \n\r\t") + 1);
    if (wordToCheck[0] == '(' || wordToCheck[0] == '[' || wordToCheck[0] == '{' || wordToCheck[0] == ':' ||
        wordToCheck[0] == '~' || wordToCheck[0] == ' ' || wordToCheck[0] == '!' || wordToCheck[0] == '$')
        wordToCheck = wordToCheck.substr(1, wordToCheck.length());
    
    size_t openingPos = 0;
    size_t delta = 0;
    while (openingPos != std::string::npos)
    {
        // Find openingTag pos
        openingPos = lineToCheck.find(openingTag);
        if (openingPos == std::string::npos) {
            return false;  // openingTag not found
        }

        // Find closingTag pos after openingTag
        size_t closingPos = lineToCheck.find(closingTag, openingPos + openingTag.length()) + 1;
        if (closingPos == std::string::npos) {
            return false;  // closingTag not found
        }

        // Get the substring between openingTag and closingTag
        std::string nameValue = lineToCheck.substr(openingPos + openingTag.length(), closingPos - openingPos - openingTag.length());

        // Trim the spaces
        nameValue.erase(std::remove_if(nameValue.begin(), nameValue.end(), ::isspace), nameValue.end());

        // Find the word in the substring
        size_t wordPos = nameValue.find(wordToCheck);
        if (wordPos != std::string::npos && pos > openingPos && pos < (closingPos + delta))
            return true;

        // Try to seek row to search on next iteration
        delta += openingPos + openingTag.length() + 1;
        lineToCheck = lineToCheck.substr(openingPos + openingTag.length() + 1, lineToCheck.length());
    }
    return false;
}

static void SetStyles() {
    active = true;

    // Init the logger
    if (debug && (!logger || !logger.is_open())) {
        remove("log.txt");
        logger.open("log.txt", std::ios_base::app);
    }

    // Set the default style
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLECLEARALL, 0, 0);
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, STYLE_DEFAULT, RGB(0, 0, 0));
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(255, 255, 255));

    // Set the styles for keywords and operators
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_WORD, RGB(0, 0, 255)); // Blue style for keywords
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_COMMENT, RGB(0, 155, 0)); // Green style for comments
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_TASKMARKER, RGB(140, 0, 0)); // Bordeaux style for triggers
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_COMMENTDOC, RGB(220, 100, 200)); // Purple style for zones
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_HASHQUOTEDSTRING, RGB(255, 165, 0)); // Orange style for macros
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_PREPROCESSOR, RGB(120, 120, 120)); // Gray style for macros
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_OPERATOR, RGB(0, 0, 0)); // Black style for operators

    // Set the style for mismatched parentheses
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_ESCAPESEQUENCE, RGB(255, 0, 0)); // Red style for errors
}

static LRESULT handleScnModified(SCNotification* notification) {
    if (notification->modificationType & (SC_MOD_INSERTTEXT)) {
        if (notification->text != NULL && (!strcmp(notification->text, "\n") || !strcmp(notification->text, "\t"))) {
            return -1;
        }
        int currentPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETCURRENTPOS, 0, 0);
        int currentLine = ::SendMessage(nppData._scintillaMainHandle, SCI_LINEFROMPOSITION, currentPosition, 0);
        int lineStartPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_POSITIONFROMLINE, currentLine, 0);
        int lineEndPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETLINEENDPOSITION, currentLine, 0);
        int wordStartPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_WORDSTARTPOSITION, currentPosition, true);
        int wordEndPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_WORDENDPOSITION, currentPosition, true);

        std::string lineText = editor.GetLine(currentLine);
        int index = lineText.find("=");
        if ((index < 0) || !(lineText.find("text=") != 0 && lineText.find("partner=") != 0 && lineText.find("backside=") != 0 && lineText.find("name=") != 0 && lineText.find("power=") != 0 && lineText.find("toughness=") != 0 &&
            lineText.find("type=") != 0 && lineText.find("subtype=") != 0 && lineText.find("grade=") != 0 && lineText.find("#") != 0))
            return -1;

        editor.AutoCSetAutoHide(false);

        if (wordStartPosition != wordEndPosition) {
            std::string currentWord = editor.GetText().substr(wordStartPosition, wordEndPosition);
            currentWord = currentWord.substr(0, currentWord.find_first_of('\r'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('\n'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('\t'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(' '));
            currentWord = currentWord.substr(0, currentWord.find_first_of(')'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('('));
            currentWord = currentWord.substr(0, currentWord.find_first_of(']'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(']'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('{'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('}'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('$'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('$'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('!'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('!'));
            if (editor.GetText().substr(wordStartPosition - 1, 1) == "@" || editor.GetText().substr(wordStartPosition - 1, 1) == "_" ||
                editor.GetText().substr(wordStartPosition - 1, 1) == "~")
                currentWord = editor.GetText().substr(wordStartPosition - 1, 1) + currentWord;
            if (currentWord[currentWord.length() - 1] == ')' || currentWord[currentWord.length() - 1] == ']' || 
                currentWord[currentWord.length() - 1] == '}' || currentWord[currentWord.length() - 1] == '!')
                currentWord = currentWord.substr(0, currentWord.length() - 1);

            std::vector<std::string> matchingSuggestions;

            for (const std::string& suggestion : allVectors) {
                if (suggestion.compare(0, currentWord.length(), currentWord) == 0) {
                    matchingSuggestions.push_back(suggestion);
                }
            }

            if (!matchingSuggestions.empty()) {
                // Merge all the suggetsions in a unique string with '\n' sperator
                std::string suggestionsString;
                for (const std::string& suggestion : matchingSuggestions) {
                    suggestionsString += suggestion + '\n';
                }

                // Set the suggestion menu of Notepad++
                editor.AutoCCancel();
                editor.AutoCShow(currentWord.length(), suggestionsString);
                editor.AutoCSetSeparator('\n');
                editor.AutoCSetAutoHide(true);
            }

            SetStyles();
            CheckWagicLineSyntax(currentLine);
            return 0;
        }
    }
    return -1;
}

static void showAbout() {
	ShowAboutDialog((HINSTANCE)dllModule, MAKEINTRESOURCE(IDD_ABOUTDLG), nppData._nppHandle);
}

static void disablePlugin() {
    active = false;
    int endpos = editor.GetLength();
    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, 0, 0x1f);
    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endpos, SCE_C_OPERATOR);
    if (debug && logger && logger.is_open()) {
        logger.close();
        remove("log.txt");
    }
}

static void CheckWagicCurrentLineSyntax() {
    SetStyles();
    CheckWagicLineSyntax(-1);
}

static void CheckWagicAllLinesSyntax() {
    SetStyles();
    int lineCount = editor.GetLineCount();
    for (int i = 0; i < lineCount; i++)
        CheckWagicLineSyntax(i);
}

static void CheckWagicLineSyntax(int i) {
    if (i < 0)
        i = editor.LineFromPosition(editor.GetCurrentPos());
    std::string lineText = editor.GetLine(i);
    std::transform(lineText.begin(), lineText.end(), lineText.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // Check if it's a comment row
    if (lineText[0] == '#' && lineText.find("auto_define") == std::string::npos) {
        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, editor.PositionFromLine(i), 0x1f);
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, editor.GetLineEndPosition(i) - editor.PositionFromLine(i), SCE_C_COMMENT);
        return;
    }
    else {
        // Set default color for row before the analysis
        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, editor.PositionFromLine(i), 0x1f);
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, editor.GetLineEndPosition(i) - editor.PositionFromLine(i), SCE_C_OPERATOR);
    }
    // Check if it's a row to control
    if (lineText.find("text=") != 0 && lineText.find("name=") != 0 && lineText.find("power=") != 0 && lineText.find("toughness=") != 0 &&
        lineText.find("type=") != 0 && lineText.find("subtype=") != 0 && lineText.find("grade=") != 0 && lineText.find("backside=") != 0 && 
        lineText.find("partner=") != 0) {
        // Remove the row prefix
        int offset = lineText.find("auto_define");
        if (offset < 0) {
            offset = offset = lineText.find("=");
            if (offset < 0)
                return;
        }
        lineText = lineText.substr(offset);

        size_t pos = 0;
        bool triggerRow = false;
        bool macroRow = false;
        while (pos != std::string::npos && pos < lineText.length()) {
            while (pos < lineText.length() && lineText[pos] != '_' && lineText[pos] != '@' && (lineText[pos] < 97 || lineText[pos] > 122)) {
                if (pos < lineText.length())
                    pos++;
            }
            int startPos = pos;
            bool triggerWord = false;
            bool macroWord = false;
            if (lineText[startPos] == '@') {
                triggerRow = true;
                triggerWord = true;
            }
            else if (lineText[startPos] == '_') {
                macroRow = true;
                macroWord = true;
            }
            if (pos < lineText.length() && lineText[pos] == '@')
                pos++;
            if (pos < lineText.length() && lineText[pos] == '_')
                pos++;
            if (pos < lineText.length() && lineText[pos] == '_')
                pos++;
            while (pos < lineText.length() && (lineText[pos] >= 97 && lineText[pos] <= 122)) {
                if (pos < lineText.length())
                    pos++;
            }
            if (pos < lineText.length() && lineText[pos] == '@')
                pos++;
            if (pos < lineText.length() && lineText[startPos] == '_' && lineText[pos] == '_')
                pos++;
            if (pos < lineText.length() && lineText[startPos] == '_' && lineText[pos] == '_')
                pos++;
            int endPos = pos;
            std::string word = lineText.substr(startPos, endPos - startPos);
            if (!containsWordBetween("name(", ")", lineText, word, startPos) && !containsWordBetween("counter(", ")", lineText, word, startPos) &&
                !containsWordBetween("counter{", "}", lineText, word, startPos) && !containsWordBetween("counters(", ")", lineText, word, startPos) &&
                !containsWordBetween("c(", ")", lineText, word, startPos)){
                startPos = editor.PositionFromLine(i) + startPos + offset;
                endPos = startPos + word.length();
                if (endPos > editor.GetLineEndPosition(i))
                    endPos = editor.GetLineEndPosition(i);
                // Apply the correct color
                if (std::find(keywords.begin(), keywords.end(), word) != keywords.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_WORD);
                    if (word == "reveal" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) && 
                        lineText[endPos - offset - editor.PositionFromLine(i)] != ':') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_COMMENTDOC);
                    }
                }
                else if (std::find(zones.begin(), zones.end(), word) != zones.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_COMMENTDOC);
                }
                else if (std::find(constants.begin(), constants.end(), word) != constants.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                }
                else if (std::find(basicabilities.begin(), basicabilities.end(), word) != basicabilities.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                }
                else if (std::find(types.begin(), types.end(), word) != types.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_OPERATOR);
                }
                else if(!word.empty() && !triggerWord  && !macroWord && 
                    ((startPos - offset - editor.PositionFromLine(i)) > 0 && lineText[startPos - offset - editor.PositionFromLine(i) - 1] != '_') &&
                    !containsWordBetween("all(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("target(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("token(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("create(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("cards(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("counteradded(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("flip(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("doubleside(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("named!", "!", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("sacrifice(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("s(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("d(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("meldfrom(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("meld(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("except(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("from(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("foreach(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("lord(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("thisturn(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("type(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)))
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos , 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                    if (debug && logger && logger.is_open()) {
                        word = "\"" + word + "\"" + "\n";
                        logger << "{ERR} - " << "unknown word in line (" << i << "): " << word;
                        logger.flush();
                    }
                }
            }
        }
        if (triggerRow) {
            // Find the all the possibile composite triggers in the current row
            for (const std::string& trigger : triggers) {
                int lastfound = 0;
                while ((lastfound = lineText.find(trigger, lastfound)) != std::string::npos) {
                    int startPos = editor.PositionFromLine(i) + lastfound + offset;
                    int endPos = startPos + trigger.length();
                    if (endPos > editor.GetLineEndPosition(i))
                        endPos = editor.GetLineEndPosition(i);
                    // Apply the correct color
                    if (std::find(triggers.begin(), triggers.end(), trigger) != triggers.end())
                    {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_TASKMARKER);
                    }
                    lastfound += trigger.length();
                    break;
                }
            }
        }
        if (macroRow) {
            // Find the all the possibile composite macros in the current row
            std::transform(lineText.begin(), lineText.end(), lineText.begin(),
                [](unsigned char c) { return std::toupper(c); });
            for (const std::string& macro : macros) {
                int lastfound = 0;
                while ((lastfound = lineText.find(macro, lastfound)) != std::string::npos) {
                    int startPos = editor.PositionFromLine(i) + lastfound + offset;
                    int endPos = startPos + macro.length();
                    if (endPos > editor.GetLineEndPosition(i))
                        endPos = editor.GetLineEndPosition(i);
                    // Apply the correct color
                    if (std::find(macros.begin(), macros.end(), macro) != macros.end())
                    {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_HASHQUOTEDSTRING);
                    }
                    lastfound += macro.length();
                    pos = lastfound;
                    break;
                }
            }
        }
        // Check the unbalanced parenthesis
        std::string openBrackets = "([{";
        std::string closeBrackets = ")]}";

        std::stack<int> bracketsStack1;
        std::stack<int> bracketsStack2;
        std::stack<int> bracketsStack3;
        std::stack<int> bracketsStack4;
        std::stack<int> bracketsStack5;

        for (size_t j = 0; j < lineText.length(); j++) {
            char currentChar = lineText[j];
            int startPos = editor.PositionFromLine(i) + j + offset;
            if ((openBrackets.find(currentChar) != std::string::npos) || (currentChar == '$' && lineText[j + 1] == '!') || (currentChar == '!')) {
                int delta = 0;
                if (currentChar == '(') {
                    bracketsStack1.push(startPos);
                }
                else if (currentChar == '[') {
                    bracketsStack2.push(startPos);
                }
                else if (currentChar == '{') {
                    bracketsStack3.push(startPos);
                }
                else if (currentChar == '$') {
                    bracketsStack4.push(startPos);
                    delta = 1;
                }
                else if (currentChar == '!') {
                    bracketsStack5.push(startPos);
                    if ((lineText[j + 1] == '(') || (lineText[j + 1] == ':'))
                        delta = 1;
                }
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1 + delta, SCE_C_WORD);
            }
            else if ((closeBrackets.find(currentChar) != std::string::npos) || (currentChar == '$' && lineText[j - 1] == '!')) {
                if ((currentChar == ')' && bracketsStack1.empty()) || (currentChar == ']' && bracketsStack2.empty()) || (currentChar == '}' && bracketsStack3.empty()) || (currentChar == '$' && bracketsStack4.empty())) {
                    // Unbalanced closing parenthesis
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);
                }
                else if ((currentChar == ')' && !bracketsStack1.empty()) || (currentChar == ']' && !bracketsStack2.empty()) || (currentChar == '}' && !bracketsStack3.empty()) || (currentChar == '$' && !bracketsStack4.empty())) {
                    int delta = 0;
                    if (currentChar == ')') {
                        bracketsStack1.pop();
                        if (j < lineText.size() - 1 && lineText[j + 1] == '!')
                            delta = 1;
                    }
                    else if (currentChar == ']') {
                        bracketsStack2.pop();
                    }
                    else if (currentChar == '}') {
                        bracketsStack3.pop();
                    }
                    else if (currentChar == '$') {
                        bracketsStack4.pop();
                        if (lineText[j - 1] == '!') {
                            delta = 1;
                            startPos--;
                        }
                    }
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1 + delta, SCE_C_WORD);
                    // Handle with the last char of row
                    if (j == lineText.length() - 1) {
                        // Unbalanced opening parenthesis at the end of row
                        int lastPos = editor.PositionFromLine(i) + j + offset + 1;
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, lastPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_ESCAPESEQUENCE, RGB(0, 0, 255)); // Imposta il colore blu per la parentesi aperta non bilanciata
                    }
                }
            }
        }

        // Check if there are '(' still remaining
        while (!bracketsStack1.empty()) {
            // Unbalanced opening parenthesis
            int startPos = bracketsStack1.top();
            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

            bracketsStack1.pop();
        }

        // Check if there are '[' still remaining
        while (!bracketsStack2.empty()) {
            // Unbalanced opening parenthesis
            int startPos = bracketsStack2.top();
            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

            bracketsStack2.pop();
        }

        // Check if there are '{' still remaining
        while (!bracketsStack3.empty()) {
            // Unbalanced opening parenthesis
            int startPos = bracketsStack3.top();
            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

            bracketsStack3.pop();
        }

        //Check if there are '$' still remaining
        while (!bracketsStack4.empty()) {
            // Unbalanced opening dollar
            int startPos = bracketsStack4.top();
            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

            bracketsStack4.pop();
        }

        // Check if there are '!' still remaining
        int even = bracketsStack5.size() % 2;
        while (!bracketsStack5.empty()) {
            // Unbalanced opening mark
            if (even != 0) {
                int startPos = bracketsStack5.top();
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);
            }
            bracketsStack5.pop();
        }
    }
}

static void SetCurrentEditor() {
    int which = -1;
    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, SCI_UNUSED, (LPARAM)&which);
    editor = (which == 0) ? editor1 : editor2;

    // Init the logger
    if (debug && (!logger || !logger.is_open())) {
        remove("log.txt");
        logger.open("log.txt", std::ios_base::app);
    }

    // Initialize the full vector starting from the single ones.
    if (allVectors.empty()) {
        allVectors.insert(allVectors.end(), zones.begin(), zones.end());
        allVectors.insert(allVectors.end(), constants.begin(), constants.end());
        allVectors.insert(allVectors.end(), basicabilities.begin(), basicabilities.end());
        allVectors.insert(allVectors.end(), keywords.begin(), keywords.end());
        allVectors.insert(allVectors.end(), types.begin(), types.end());
        allVectors.insert(allVectors.end(), triggers.begin(), triggers.end());
        allVectors.insert(allVectors.end(), macros.begin(), macros.end());
    }
}

LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_HOTKEY && active) {
        SCNotification* notification = new SCNotification();
        notification->modificationType = SC_MOD_INSERTTEXT;
        handleScnModified(notification);
        return 0;
    }

    // Pass the other events to main handle
    return ::CallWindowProc(OldPluginWndProc, hWnd, message, wParam, lParam);
}

// Notification function
extern "C" __declspec(dllexport) void beNotified(SCNotification * notifyCode) {
    switch (notifyCode->nmhdr.code) {
    case NPPN_BUFFERACTIVATED:
        SetCurrentEditor();
        CheckWagicCurrentLineSyntax();
        break;
    case SCN_UPDATEUI:
    case NPPN_FILEOPENED:
    case NPPN_READY:
        if(active)
            CheckWagicCurrentLineSyntax();
        break;
    case SCN_MODIFIED:
        if(active)
            handleScnModified(notifyCode);
        break;
    case NPPN_SHUTDOWN:
        if(active)
            disablePlugin();
        break;
    }

}

HWND GetScintillaHandle(HWND nppHandle) {
    HWND scintillaHandle = nullptr;
    HWND childWindow = ::FindWindowEx(nppHandle, nullptr, L"Scintilla", nullptr);

    while (childWindow != nullptr) {
        wchar_t className[256];
        ::GetClassName(childWindow, className, sizeof(className) / sizeof(wchar_t));

        if (std::wcscmp(className, L"Scintilla") == 0) {
            scintillaHandle = childWindow;
            break;
        }

        childWindow = ::FindWindowEx(nppHandle, childWindow, L"Scintilla", nullptr);
    }

    return scintillaHandle;
}

// Plugin initialization
extern "C" __declspec(dllexport) void setInfo(NppData nppData) {
    ::nppData = nppData;
	editor1.SetScintillaInstance(nppData._scintillaMainHandle);
	editor2.SetScintillaInstance(nppData._scintillaSecondHandle);
    editor = editor1;

    // Register window handle of plugin
    OldPluginWndProc = (WNDPROC)::SetWindowLongPtr(nppData._nppHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(PluginWndProc));

    // Register the Hotkey CTRL + SPACE
    ::RegisterHotKey(nppData._nppHandle, 1, MOD_CONTROL, VK_SPACE);
}

// Get plugin name
extern "C" __declspec(dllexport) const wchar_t* getName() {
    return L"Wagic Syntax Plugin";
}

// Get plugin functions
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbFuncs) {
    *nbFuncs = sizeof(menuItems) / sizeof(menuItems[0]);
    return menuItems;
}

// Message processing function
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam) {
    return TRUE;
}

// Check if plugin supports Unicode
extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID lpReserved) {
	switch (reasonForCall) {
		case DLL_PROCESS_ATTACH:
			dllModule = hModule;
			break;
		case DLL_PROCESS_DETACH:
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
