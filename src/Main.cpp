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
#include <locale>
#include <codecvt>
#include <thread>

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
// The stream for logger instance
static std::ofstream logger;
// The markers for current text, current visible lines and total lines count
static int currentLineCount = 0;
static int currentFirstLine = 0;
static int currentVisibleLineCount = 0;
static std::string currentText;

// Forward declaration of menu callbacks
static void CheckWagicAllLinesSyntax();
static void CheckWagicVisibleLinesSyntax();
static void CheckWagicLineSyntax(int i);
static void DisablePlugin();
static void ShowAbout();

// The menu entries for the plugin
static FuncItem menuItems[] = {
    // name, function, 0, is_checked, shortcut
    { L"Disable Inline Syntax Check", DisablePlugin, 0, false, new ShortcutKey(false, false, true, 32) },
    { L"Enable and Perform Visible Lines Syntax Check", CheckWagicVisibleLinesSyntax, 0, false, new ShortcutKey(false, true, false, 32) },
    { L"Enable and Perform All Lines Syntax Check (VERY SLOW)", CheckWagicAllLinesSyntax, 0, false, nullptr},
    { L"", nullptr, 0, false, nullptr }, // Separator
	{ L"About...", ShowAbout, 0, false, new ShortcutKey(false, true, true, 32) }
};

std::vector<std::string> keywords = {
    "any", "blink", "forsrc", "duplicateall", "halfall", "notrigger", "ability", "abilitycontroller", "add", "addtype", "novent", "delayed",
    "affinity", "after", "age", "all", "allsubtypes", "altercost", "alterdevoffset", "alterenergy", "alterexperience", "altermutationcounter", 
    "alternativecostrule", "alterpoison", "altersurvoffset", "alteryidarocount", "and", "aslongas", "assorcery", "attackcost", "attackcostrule", 
    "attackedalone", "attackpwcost", "attackrule", "backside", "becomes", "becomesmonarch", "becomesmonarchfoeof", "becomesmonarchof", "belong", 
    "bestowrule", "blockcost", "blockcostrule", "blockrule", "bloodthirst", "boasted", "bonusrule", "bottomoflibrary", "bstw", "bury", "bushido", 
    "buybackrule", "cantargetcard", "cantbeblockedby", "cantbeblockerof", "cantbetargetof", "canuntap", "card", "cards", "cascade", "castcard", 
    "restricted", "casted", "cdaactive", "changecost", "checkex", "clone", "coinflipped", "coloringest", "combatphases", "combatends", "colors", 
    "combatspiritlink", "combattriggerrule", "commandzonecast", "compare", "completedungeon", "conjure", "connect", "connectrule", "continue", 
    "controller", "copied", "copiedacard", "copy", "costx", "count", "countb", "counter", "countermod", "counterremoved", "countershroud", 
    "countersoneone", "countertrack", "coven", "create", "cumulativeupcost", "cycled", "damage", "deadcreart", "deadpermanent", "deathtouchrule", 
    "delirium", "deplete", "destroy", "didattack", "didblock", "didnotcastnontoken", "didntattack", "discard", "discardbyopponent", "doboast", 
    "doesntempty", "doforetell", "donothing", "dontremove", "dontshow", "dotrain", "doubleside", "draw", "dredgerule", "duplicate", "exilecast",
    "duplicatecounters", "dynamicability", "eachother", "epic", "equalto", "equalto~", "equip", "evicttypes", "evolve", "except", "exchangelife", 
    "exert", "exileimp", "exploits", "explores", "fade", "fading", "fizzle", "fizzleto", "flanker", "flashbackrule", "flip", "flipped", "foelost", 
    "fog", "forceclean", "forcedalive", "forcefield", "forcetype", "foreach", "forever", "freeze", "from", "fromplay", "frozen", "gravecast", 
    "half", "hasdefender", "hasexerted", "haunt", "haunted", "head", "hiddenmoveto", "hmodifer", "identify", "imprint", "imprintedcard", 
    "ingest", "itself", "kamiflip", "keepname", "kicked!", "kickerrule", "lastturn", "legendrule", "lessorequalcreatures", "lessorequallands", 
    "lessthan", "lessthan~", "level", "librarybottom", "librarycast", "librarysecond", "librarytop", "life", "lifeleech", "lifelinkrule", "limit",
    "lifeloss", "lifeset",  "livingweapon", "lord", "loseabilities", "loseability", "losesatype", "losesubtypesof", "lost", "madnessplayed", "max",
    "manafaceup", "manapool", "manifest",  "maxcast", "maxlevel", "maxplay", "meld", "meldfrom", "message", "miracle", "modbenchant", "morbid", 
    "morecardsthanopponent", "morethan", "morethan~", "morph", "morphrule", "most", "movedto", "moverandom", "moveto", "mutated", "mutateover", 
    "mutateunder", "mutationover", "mutationunder", "myfoe", "myorigname", "myself", "myturnonly", "name", "named!", "nameingest", "never", "next",
    "newability", "newcolors", "newhook", "newtarget", "nextphase", "ninjutsu", "noevent", "nonbasicland", "nonstatic", "noreplace", "normal", 
    "notatarget", "notblocked", "notdelirum", "notexerted", "notpaid", "notrg", "once", "oneonecounters", "oneshot", "only", "opponent", "out",
    "outnumbered", "opponentdamagedbycombat", "opponentpoisoned", "opponentpool", "opponentscontrol", "opponentturnonly", "options", "overloadrule", 
    "ownerscontrol", "paid", "paidmana", "pay", "payzerorule", "persistrule", "phaseaction", "phaseactionmulti", "phasealter", "phasedin", 
    "phaseout", "placefromthetop", "planeswalkerattack", "planeswalkerdamage", "planeswalkerrule", "plus", "poolsave", "positive", "proliferate",
    "powermorethancontrollerhand", "produceextra", "powermorethanopponenthand", "prevent", "preventallcombatdamage", "preventalldamage", "prowl",
    "propagate", "preventallnoncombatdamage", "previous", "producecolor", "provoke", "pumpboth", "pumppow", "pumptough", "putinplay", "raid",
    "putinplayrule", "rampage", "randomcard", "rebound", "reconfigure", "reduce", "reduceto", "regenerate", "rehook", "reject", "remake", "reveal",
    "removeallcounters", "removealltypes", "removecreaturesubtypes", "removedfromgame", "removefromcombat", "removemana", "removemc", "revealend",
    "removesinglecountertype", "removetypes", "repeat", "resetdamage", "restriction", "result", "retarget", "retracerule", "return", "revealtype", 
    "revealuntil", "revealzone", "revolt", "sacrifice", "scry", "scryend", "scryzone", "selectmana", "serumpowder", "setblocker", "sethand", 
    "setpower", "settoughness", "shackle", "shuffle", "sidecast", "single", "skill", "soulbondrule", "source", "sourceinplay", "sourcenottap", 
    "sourcetap", "spellmover", "spent", "spiritlink", "srccontroller", "srcopponent", "standard", "steal", "surveil", "suspended", "suspendrule", 
    "swap", "tail", "tails", "taketheinitiative", "tap", "target", "targetcontroller", "targetopponent", "teach", "terminate", "becomesringbearer",
    "text", "this", "thisforeach", "thisturn", "time", "toughnesslifegain", "to", "token", "tokencleanuprule", "tokencreated", "tosrc", "total", 
    "trained", "trainer", "transforms", "trigger", "turn", "turnlimited", "turns", "tutorial", "type", "uent", "ueot", "undocpy", "unearthrule",
    "untap", "upcost", "upcostmulti", "uynt", "vampired", "vampirerule", "vanishing", "while", "winability", "wingame", "with", "withenchant", 
    "won", "zerodead", "zone", "if", "then", "else", "activate", "unattach", "choice", "may", "beforenextturn", "~morethan~", "~lessthan~", "other",
    "~equalto~", "loseabilityend", "rolld", "end", "afterrevealed", "afterrevealedend", "variable", "winabilityend", "chooseanameopp", "chooseend",
    "chooseaname", "nonland", "upkeep", "optionone", "optiononeend", "optiontwo", "optiontwoend", "my", "ifnot", "blockers", "combatbegins", 
    "multi", "powerstrike", "can", "play", "endofturn", "replacedraw", "cycle", "nextphasealter", "scrycore", "scrycoreend", "named", "beginofturn", 
    "firstmain", "livingweapontoken", "flipacoin", "flipend", "chooseatype", "drawonly", "firstmainonly", "secondmain", "chooseacolor", "remove",
    "retargetfromplay", "attackersonly", "blockersonly", "combatdamageonly", "combatendsonly", "combatbeginsonly", "activatechooseacolor", "each",
    "activatechooseend", "powerpumpboth", "targetedpersonsbattlefield", "myattackersonly", "combat", "chargedraw", "manastrike", "activatedability", 
    "manacostlifeloss", "combatbegin", "combatattackers", "combatblockers", "combatend", "grant", "grantend", "opponentupkeeponly", "myupkeeponly", 
    "manacostlifestrike", "toughnesslifeloss", "main", "powerpumppow", "manacostlifelose", "opponentreplacedraw", "chosencolorend", "dredge",
    "activatechooseatype", "combatphaseswithmain", "agecountersoneone", "cantblocktarget", "endstep", "before", "attackers", "during", "noproftrg",
    "cleanup", "manacostlifegain", "opponentblockersonly", "powerlifegain", "cumulativeupcostmulti", "sourcenottapped", "oneonecountersstrike", 
    "powercountersoneone", "powerlifeloss", "manacoststrike", "chargelifegain", "myupkeep", "untaponly", "toughnesstrike", "chargedeplete", 
    "chargestrike", "manacostpumppow", "didcombatdamagetofoe", "manacostpumpboth", "manacostpumptough", "powerdraw", "colorspumpboth", "postbattle", 
    "nonwall", "value", "twist", "emblem", "combatdamage", "convoke", "crew", "improvise", "delve", "emerge", "readytofight", "theringtempts",
    "oppoattacked", "untp", "nocost", "removeallsubtypes", "removeallcolors"
    // Add any additional Wagic keyword here
};

// List of Wagic zones
std::vector<std::string> zones = {
    "mycastingzone", "myrestrictedcastingzone", "mycommandplay", "myhandlibrary", "mygravelibrary", "opponentgravelibrary", "mygraveexile",
    "opponentgraveexile", "opponentcastingzone", "opponentrestrictedcastingzone", "opponentcommandplay", "opponenthandlibrary", "mynonplaynonexile", 
    "opponentnonplaynonexile", "myhandexilegrave", "opponenthandexilegrave", "myzones", "opponentzones", "mysideboard", "mycommandzone", "myreveal", 
    "mygraveyard", "mybattlefield", "myhand", "mylibrary", "mystack", "myexile", "opponentsideboard", "opponentcommandzone", "opponentreveal", 
    "opponentgraveyard", "opponentbattlefield", "opponenthand", "opponentlibrary", "opponentstack", "opponentexile","ownersideboard", "ownerreveal",
    "ownercommandzone",  "ownergraveyard", "ownerbattlefield", "ownerinplay", "ownerhand", "ownerlibrary", "ownerstack", "ownerexile", "sideboard", 
    "commandzone", "reveal", "graveyard", "battlefield", "inplay", "hand", "library", "nonbattlezone", "stack", "exile", "previousbattlefield", 
    "targetedpersonssideboard", "targetedpersonsreveal", "targetedpersonscommandzone", "targetedpersonsgraveyard", "targetedpersonsbattlefield",
    "targetedpersonsinplay", "targetedpersonshand", "targetedpersonslibrary", "targetedpersonsstack", "targetedpersonsexile", "targetcontrollerexile",
    "targetcontrollersideboard", "targetcontrollerreveal", "targetcontrollercommandzone", "targetcontrollergraveyard", "targetcontrollerbattlefield",
    "targetcontrollerinplay", "targetcontrollerhand", "targetcontrollerlibrary", "targetcontrollerstack", "targetedplayersideboard", 
    "targetedplayerhand", "targetedplayercommandzone", "targetedplayergraveyard", "targetedplayerbattlefield", "targetedplayerinplay", 
    "targetedplayerlibrary", "targetedplayerexile", "targetedplayerstack", "targetedplayerreveal"
    // Add any additional Wagic zone here
};

// List of Wagic triggers
std::vector<std::string> triggers = {
    "@tappedformana", "@becomesmonarchof", "@drawof", "@combat", "@boasted", "@targeted", "@lifelostfoeof", "@noncombatdamagefoeof", 
    "@takeninitiativeof", "@takeninitiativefoeof", "@energizedof", "@counteradded", "@totalcounteradded", "@scryed", "@producedmana",
    "@dierolled", "@damageof", "@sacrificed", "@combatdamaged", "@shuffledfoeof", "@cycled", "@poisonedfoeof", "@foretold", "@energizedfoeof", 
    "@discarded", "@tokencreated", "@phasedin", "@untapped", "@drawfoeof", "@combatdamagefoeof", "@exploited", "@poisonedof", "@exerted", 
    "@movedTo", "@experiencedof", "@counterremoved", "@vampired", "@countermod", "@experiencedfoeof", "@shuffledof", "@lifeof", "@noncombatdamaged", 
    "@trained", "@defeated", "@dungeoncompleted", "@surveiled", "@damagefoeof", "@combatdamageof", "@coinflipped", "@noncombatdamageof", "@movedto", 
    "@damaged", "@explored", "@lifefoeof", "@lifelostof", "@transformed", "@facedup", "@lord", "@drawn", "@castcard", "@tapped", "@mutated", 
    "@each endofturn", "@each beginofturn", "@each upkeep", "@each firstmain", "@each combatbegins", "@each attackers", "@each blockers", 
    "@each combatdamage", "@each combatends", "@each secondmain", "@each end", "@each cleanup", "@each untap", "@each my endofturn", 
    "@each my beginofturn", "@each my upkeep", "@each my firstmain", "@each my combatbegins", "@each my attackers", "@each my blockers", 
    "@each my combatdamage", "@each my combatends", "@each my secondmain", "@each my end","@each my cleanup","@each my untap", "@each draw",
    "@each opponent endofturn", "@each opponent beginofturn", "@each opponent upkeep", "@each opponent firstmain", "@each opponent combatbegins", 
    "@each opponent attackers", "@each opponent blockers", "@each opponent combatdamage", "@each opponent combatends", "@each opponent secondmain", 
    "@each opponent end", "@each opponent cleanup", "@each opponent untap", "@next endofturn", "@next beginofturn", "@next upkeep", "@each my draw",
    "@next firstmain", "@next combatbegins", "@next attackers", "@next blockers", "@next combatdamage", "@next combatends", "@next secondmain", 
    "@each opponent draw", "@next end", "@next cleanup", "@next untap", "@proliferateof", "@proliferatefoeof", "@ninjutsued", "@ringtemptedof", 
    "@ringtemptedfoeof", "@becomesmonarchfoeof", "@bearerchosen", "@bearernewchosen", "@each targetedplayer endofturn", "@each targetedplayer end",
    "@each targetedplayer beginofturn", "@each targetedplayer upkeep", "@each targetedplayer firstmain", "@each targetedplayer combatbegins", 
    "@each targetedplayer attackers", "@each targetedplayer blockers", "@each targetedplayer combatdamage", "@each targetedplayer combatends", 
    "@each targetedplayer secondmain", "@each targetedplayer cleanup", "@each targetedplayer untap"

    // Add any additional Wagic trigger here
};

// List of Wagic macros
std::vector<std::string> macros = {
    "__CYCLING__", "__BASIC_LANDCYCLING__", "_DIES_", "_TRAINING_", "_PARTNER_", "_GOAD_", "_REBOUND_", "_POPULATE_", "_FEROCIOUS_", "_ATTACKING_", 
    "_BLOCKED_", "_HEROIC_", "_RALLY_", "_LANDFALL_", "_ADDENDUM_", "_CONSTELLATION_", "_SCRY_", "_SCRY1_", "_SCRY2_", "_SCRY3_", "_ZOMBIETOKEN_",
    "_SCRY4_", "_SCRY5_", "_FABRICATE_", "_ENRAGE_", "_SECOND_DRAW_", "_ADAPT_", "_BATTALION_", "_CHAMPION_", "_METALCRAFT_", "_KNIGHTTOKEN_",
    "_THRESHOLD_", "_SPLICEARCANE_", "_RIPPLE_", "_RECOVER_", "_CLASH_", "_PROLIFERATE_", "_SCAVENGE_", "_MONSTROSITY_", "_OUTLAST_", "_ECHO_",
    "_INVESTIGATE_", "_ASCEND_", "_CITY'S_BLESSING_", "_EXPLORE_", "_MONARCH_CONTROLLER_", "_MONARCH_OPPONENT_", "_INITIATIVE_CONTROLLER_",
    "_INITIATIVE_OPPONENT_", "_TREASURE_", "_CASTHISTORIC_", "_MENTOR_", "_SURVEIL1_", "_SURVEIL2_", "_SURVEIL3_", "_UNDERGROWTH_", "_RIOT_",
    "_AFTERLIFETOKEN_", "_LEARN_", "_SPECTACLE_", "_ADVENTURE_", "_EXTORT_", "_FORETELL_", "_LOOT_", "_UNEARTH_", "__PLAY_TOP_FROM_EXILE__",
    "_WARD_", "_RENOWN_", "_BLINK_UEOT_", "_CONNIVES_", "_ETERNALIZE_", "_DISCARD&DRAW_", "_PUNCH_", "_MUST_BE_BLOCKD_", "_ANGELTOKEN_", 
    "_BEASTTOKEN_", "_DRAGONTOKEN_", "_ELEPHANTTOKEN_", "_GOBLINTOKEN_", "_INSECTTOKEN_", "_PHYREXIANMITETOKEN_", "_REDELEMENTALTOKEN_", 
    "_HARNESSED_LIGHTNING_", "_SAPROLINGTOKEN_", "_SERVOTOKEN_", "_SOLDIERTOKEN_", "_SPIRITTOKEN_", "_THOPTERTOKEN_", "_WOLFTOKEN_", 
    "_HARNESSED_CONTROL_", "_AMASSORC1_", "_AMASSORC2_", "_AMASSORC3_", "_AMASSORC4_", "_AMASSORC5_", "_AMASSZOMBIE1_", "_AMASSZOMBIE2_", 
    "_AMASSZOMBIE3_", "_AMASSZOMBIE4_", "_AMASSZOMBIE5_", "_RINGTEMPTS_"
    // Add any additional Wagic macro here
};

// List of Wagic constants
std::vector<std::string> constants = {
    "abundantlife", "allbattlefieldcardtypes", "allgravecardtypes", "allmyname", "auras", "azorius", "boros", "bothalldead", "bushidopoints", 
    "calculateparty", "canforetellcast", "cantargetcre", "cantargetmycre", "cantargetoppocre", "castx", "chosencolor", "chosenname", "chosentype", 
    "commonblack", "commonblue", "commongreen", "commonred", "commonwhite", "controllerturn", "converge", "convertedcost:", "countallspell", 
    "countedamount", "countedbamount", "countmycrespell", "countmynoncrespell", "crewtotalpower", "currentphase", "currentturn", "evictth",
    "cursedscrollresult", "dimir", "direction", "dualfaced", "epicactivated", "evictb", "evictg", "evictmc", "evictpw", "evictr", "evictu", 
    "evictw", "excessdamage", "findfirsttype", "findlasttype", "fivetimes", "fourtimes", "fullpaid", "gear", "genrand", "golgari", "gruul", 
    "halfdown", "halfpaid", "halfup", "handsize", "hasability", "hascnt", "hasevict", "hasmansym", "hasprey", "hasstorecard", "highest", 
    "highestlifetotal", "iscopied", "isflipped", "ishuman", "izzet", "kicked", "lastdiefaces", "lastflipchoice", "lastflipresult", "lastrollchoice", 
    "lastrollresult", "lifegain", "lifelost", "lifetotal", "lowestlifetotal", "magusofscrollresult", "math", "mathend", "minus", "minusend", 
    "mutations", "mybattlefieldcardtypes", "myblackpoolcount", "mybluepoolcount", "mycolnum", "mycolorlesspoolcount", "mygravecardtypes", 
    "mygreenpoolcount", "myname", "mypoisoncount", "mypoolcount", "mypos", "myredpoolcount", "mysnowblackpoolcount", "mysnowbluepoolcount", 
    "mysnowcolorlesspoolcount", "mysnowgreenpoolcount", "mysnowpoolcount", "mysnowredpoolcount", "mysnowwhitepoolcount", "mytarg", "odomain",
    "mywhitepoolcount", "numofcommandcast", "numoftypes", "oalldead", "oattackedcount", "ocoven", "ocycledcount", "odcount", "odevotionoffset", 
    "odiffinitlife", "odnoncount", "odrewcount", "odungeoncompleted", "oenergy", "oexperience", "ohalfinitlife", "ohandcount", "oinitiative", 
    "oinstsorcount", "olandb", "olandc", "olandg", "olandr", "olandu", "olandw", "olastshlturn", "olibrarycount", "omonarch", "onumcreswp", 
    "onumofcommandcast", "onumofidentitycols", "oplifegain", "oplifelost", "oppbattlefieldcardtypes", "oppgravecardtypes", "oppofindfirsttype", 
    "oppofindlasttype", "opponentblackpoolcount", "opponentbluepoolcount", "opponentcolorlesspoolcount", "opponentgreenpoolcount", "opponentturn",
    "opponentlifetotal", "opponentpoisoncount", "opponentpoolcount", "opponentredpoolcount", "opponentsnowblackpoolcount", "oppototalmana",
    "opponentsnowbluepoolcount", "opponentsnowcolorlesspoolcount", "opponentsnowgreenpoolcount", "opponentsnowpoolcount", "otherpower", "orzhov", 
    "opponentsnowredpoolcount", "opponentsnowwhitepoolcount","opponentwhitepoolcount", "oppototalcololorsinplay", "oppsametypecreatures", 
    "pdnoncount", "ostartinglife", "ostormcount", "osurveiloffset", "otherconvertedcost","othertoughness", "othertype", "oyidarocount", "prex",
    "palldead", "pdomain", "pancientooze", "pattackedcount", "pbasiclandtypes", "pcoven", "pcycledcount", "pdauntless", "pdcount", "pinitiative",
    "pdevotionoffset", "pdiffinitlife", "pdrewcount", "pdungeoncompleted", "penergy", "permanent", "pexperience", "pgbzombie", "pginstantsorcery", 
    "pgmanainstantsorcery", "phalfinitlife", "phandcount", "pinstsorcount", "plandb", "plandc", "plandg", "plandr", "plandu", "plandw", "prodmana",
    "plastshlturn", "plibrarycount", "plusend", "pmonarch", "pnumcreswp", "pnumofcommandcast", "pnumofidentitycols", "power", "powertotalinplay", 
    "pstormcount", "psurveiloffset", "pwr", "pwrtotalinplay", "pwrtotatt", "pwrtotblo", "pyidarocount", "rakdos", "revealedmana", "revealedp", 
    "revealedt", "sametypecreatures", "scryedcards", "selesnya", "simic", "snowdiffmana", "srclastdiefaces", "srclastrollchoice", "thirdup", "ths:",
    "thrice", "srclastrollresult", "startinglife", "startingplayer", "stored", "targetedcurses", "thatmuch", "thirddown", "thirdpaid", "waste",
    "thstotalinplay", "thstotatt", "thstotblo", "totalcololorsinplay", "totaldmg", "totalmana", "totcnt", "totcntall", "totcntart", "totcntbat", 
    "totcntcre", "totcntenc", "totcntlan", "totcntpla", "totmanaspent", "toughness", "toughnesstotalinplay", "twice", "urzatron", "usedmana", 
    "withpartner", "worshipped", "colorless", "green", "blue", "red", "black", "white", "alternative", "buyback", "flashback", "kicker", "facedown", 
    "faceup", "bestow", "anyamount", "attacking", "backname", "blockable", "blocked", "blocking", "cantarget ", "cardid", "children", "cmana", 
    "color ", "controllerdamager", "damaged", "damager", "description", "discarded", "dredgeable", "enchanted", "equals", "equipped", "evictname", 
    "exerted", "expansion", "extracostshadow", "foretold", "geared", "hasbackside", "hasconvoke", "hasflashback", "haskicker", "haspartner", "hasx", 
    "icon", "lastnamechosen", "leveler", "manab", "manacost", "manag", "manar", "manau", "manaw", "mtgid", "multicolor", "mychild", "mycurses", 
    "myeqp", "mysource", "mytgt", "mytotem", "notshare!", "numofcols", "opponentdamager", "pairable", "parents", "partname", "preyname", 
    "rarity", "recent", "sourcecard", "tapped", "targetedplayer", "targetter", "title", "unknown", "upto", "zpos", "notany", "oppohasdead",
    "modified", "battleready", "findfirsttypenonland", "share", "types", "findfirsttypecreature", "controllerlife", "convertedcost", "untapped", 
    "bothalldeadcreature", "findfirsttypeartifact", "findfirsttypepermanent", "oppofindfirsttypecreature", "oppofindfirsttypenonpermanent", 
    "findfirsttypenonpermanent", "findfirsttypeelemental", "findfirsttypeelf", "findfirsttypeland", "oppofindfirsttypeland", "opponentpoolsave",
    "findfirsttypeplaneswalker", "findfirsttypeenchantment", "hasmansymw", "hasmansymr", "hasmansymg", "hasmansymu", "hasmansymb", "mypoolsave", 
    "prexx", "opponentdamagecount", "thatmuchcountersoneone", "plifelost", "poisoncount", "oppofindfirsttypenonland", "lowest", "usedmanab", 
    "usedmanag", "usedmanaw", "usedmanau", "usedmanar", "usedmanatot", "toxicity", "hastoxic", "ninelands", "mytgtforced", "numofactivation", 
    "pringtemptations", "oringtemptations", "myhasdead", "oppotgt", "ctrltgt", "ohandsize", "isattacker", "couldattack"
    // Add any additional Wagic constant here
};

// List of Wagic basic abilities
std::vector<std::string> basicabilities = {
    "trample", "forestwalk", "islandwalk", "mountainwalk", "swampwalk", "plainswalk", "flying", "first", "double", "strike", "fear", "flash", 
    "haste", "lifelink", "reach", "shroud", "vigilance", "defender", "banding", "protection", "protection from green", "protection from blue", 
    "protection from red", "protection from black", "protection from white", "unblockable", "wither", "persist", "retrace", "exalted", "weak",
    "nofizzle", "shadow", "reachshadow", "foresthome", "islandhome", "mountainhome", "swamphome", "plainshome", "cloud", "cantattack", "split",
    "mustattack", "cantblock", "doesnotuntap", "opponentshroud", "indestructible", "intimidate", "deathtouch", "horsemanship", "cantregen", 
    "oneblocker", "infect", "poisontoxic", "poisontwotoxic", "poisonthreetoxic", "phantom", "wilting", "vigor", "changeling", "absorb", "treason", 
    "unearth", "cantlose", "cantlifelose", "cantmilllose", "snowlandwalk", "nonbasiclandwalk", "strong", "storm", "phasing", "second", "flanking",
    "affinityartifacts", "affinityplains", "affinityforests", "affinityislands", "affinitymountains", "affinityswamps", "exiledeath", "sunburst",
    "affinitygreencreatures", "cantwin", "nomaxhand", "leyline", "playershroud", "controllershroud", "legendarylandwalk", "desertlandwalk", 
    "snowforestlandwalk", "snowplainslandwalk", "snowmountainlandwalk", "snowislandlandwalk", "snowswamplandwalk", "canattack", "hydra", "undying", 
    "poisonshroud", "noactivatedability", "notapability", "nomanaability", "onlymanaability", "poisondamager", "soulbond", "lure", "nolegend", 
    "canplayfromgraveyard", "tokenizer", "mygraveexiler", "oppgraveexiler", "librarydeath", "shufflelibrarydeath", "offering", "evadebigger", 
    "spellmastery", "nolifegain", "nolifegainopponent", "auraward", "madness", "protectionfromcoloredspells", "mygcreatureexiler","devoid",
    "oppgcreatureexiler", "zerocast", "trinisphere", "canplayfromexile", "libraryeater", "cantchangelife", "combattoughness", "cantpaylife", 
    "cantbesacrified", "skulk", "menace", "nosolo", "mustblock", "dethrone", "overload", "shackler", "flyersonly", "tempflashback", "renown",
    "legendruleremove", "canttransform", "asflash", "conduited", "canblocktapped", "oppnomaxhand", "cantcrew", "hiddenface", "anytypeofmana", 
    "necroed", "cantpwattack", "canplayfromlibrarytop", "canplaylandlibrarytop", "canplaycreaturelibrarytop", "canplayartifactlibrarytop", 
    "canplayinstantsorcerylibrarytop", "showfromtoplibrary", "showopponenttoplibrary", "totemarmor", "discardtoplaybyopponent", "modular", 
    "adventure", "mentor", "prowess", "nofizzle alternative",  "hasotherkicker", "partner", "canbecommander", "iscommander", "threeblockers", 
    "handdeath", "inplaydeath", "inplaytapdeath", "gainedexiledeath", "gainedhanddeath", "cycling", "foretell", "anytypeofmanaability", "boast", 
    "twoboast", "replacescry", "hasnokicker", "undamageable", "lifefaker", "doublefacedeath", "gaineddoublefacedeath", "twodngtrg", "nodngopp", 
    "nodngplr", "canplayauraequiplibrarytop", "counterdeath", "dungeoncompleted", "perpetuallifelink", "perpetualdeathtouch", "noncombatvigor", 
    "nomovetrigger", "wascommander", "showopponenthand", "showcontrollerhand", "hasreplicate", "isprey", "hasdisturb", "daybound", "nightbound", 
    "decayed", "hasstrive", "isconspiracy", "hasaftermath", "noentertrg", "nodietrg", "training", "energyshroud", "expshroud", "countershroud", 
    "nonight", "nodamageremoved", "backgroundpartner", "bottomlibrarydeath", "noloyaltydamage", "nodefensedamage", "affinityallcreatures", 
    "affinitycontrollercreatures", "affinityopponentcreatures", "affinityalldeadcreatures", "affinityparty", "affinityenchantments", "mutate",
    "affinitybasiclandtypes", "affinitytwobasiclandtypes", "affinitygravecreatures", "affinityattackingcreatures", "affinitygraveinstsorc", 
    "canloyaltytwice", "attached", "fresh", "snowplainswalk", "snowislandwalk", "snowswampwalk", "snowmountainwalk", "snowforestwalk", "desertwalk",
    "poisonfourtoxic", "poisonfivetoxic", "poisonsixtoxic", "poisonseventoxic", "poisoneighttoxic", "poisonninetoxic", "poisontentoxic", "ringbearer",
    "eqpasinst", "canloyaltyasinst"
    // Add any additional Wagic basic ability here
};

// List of Wagic card types
std::vector<std::string> types = {
    "abian", "abyss", "advisor", "aetherborn", "ajani", "alara", "ally", "aminatou", "amonkhet", "angel", "angrath", "antausia", "antelope",
    "ape", "arcane", "arcavios", "archer", "archon", "arkhos", "arlinn", "art", "artifact", "artificer", "ashiok", "assassin", "assembly", "atog", 
    "aura", "aurochs", "autobot", "avatar", "azgol", "azra", "background", "baddest", "badger", "bahamut", "barbarian", "bard", "basic", "basilisk", 
    "basri", "bat", "battle", "bear", "beast", "beaver", "beeble", "beholder", "belenon", "berserker", "biggest", "bird", "blood", "boar", "bolas", 
    "brainiac", "bringer", "brushwagg", "bureaucrat", "calix", "camel", "capenna", "carrier", "cartouche", "cat", "centaur", "cephalid", "chandra",
    "chameleon", "chicken", "child", "chimera", "citizen", "clamfolk", "class", "cleric", "clue", "cockatrice", "conspiracy", "construct", "cow",
    "contraption", "coward", "crab", "creature", "cridhe", "crocodile", "curse", "cyborg", "cyclops", "dack", "dakkon", "daretti", "dauthi", "deer",
    "davriel", "demigod", "demon", "desert", "designer", "devil", "dihada", "dinosaur", "djinn", "dog", "dominaria", "domri", "donkey", "dovin", 
    "dragon", "drake", "dreadnought", "drone", "druid", "dryad", "duck", "dungeon", "dwarf", "eaturecray", "echoir", "efreet", "egg", "elder", 
    "eldraine", "eldrazi", "elemental", "elephant", "elf", "elk", "ellywick", "elminster", "elspeth", "elves", "enchant", "enchantment", "equilor",
    "equipment", "ergamon", "estrid", "etiquette", "eye", "fabacin", "faerie", "ferret", "fiora", "fish", "flagbearer", "food", "forest", "fox",
    "fractal", "freyalise", "frog", "fungus", "gamer", "gargantikar", "gargoyle", "garruk", "gate", "giant", "gideon", "gith", "gnoll", "gnome",
    "goat", "gobakhan", "goblin", "god", "golem", "gorgon", "grandchild", "gremlin", "griffin", "grist", "gus", "hag", "halfling", "harpy", 
    "hatificer", "head", "hellion", "hero", "hippo", "hippogriff", "homarid", "homunculus", "horror", "horse", "host", "huatli", "human", "hydra", 
    "hyena", "igpay", "ikoria", "illusion", "imp", "incarnation", "incubator", "insect", "instant", "interrupt", "inzerva", "iquatana", "ir", 
    "island", "ixalan", "jace", "jackal", "jared", "jaya", "jellyfish", "jeska", "juggernaut", "kaladesh", "kaldheim", "kamigawa", "kangaroo", 
    "karn", "karsus", "kasmina", "kavu", "kaya", "kephalai", "killbot", "kinshala", "kiora", "kirin", "kithkin", "knight", "kobold", "kolbahan", 
    "kor", "koth", "kraken", "krav", "kylem", "kyneth", "lady", "lair", "lamia", "lammasu", "land", "lathiel", "leech", "legend", "legendary", 
    "lesson", "leviathan", "lhurgoyf", "licid", "liliana", "lizard", "locus", "lolth", "lord", "lorwyn", "lukka", "luvion", "manticore", "master", 
    "masticore", "meditation", "mercadia", "mercenary", "merfolk", "metathran", "mime", "mine", "minion", "minotaur", "minsc", "mirrodin", "moag", 
    "mole", "monger", "mongoose", "mongseng", "monk", "monkey", "moonfolk", "mordenkainen", "mountain", "mouse", "mummy", "muraganda", "mutant", 
    "myr", "mystic", "naga", "nahiri", "narset", "nastiest", "nautilus", "nephilim", "new", "nightmare", "nightstalker", "niko", "ninja", "nissa", 
    "nixilis", "noble", "noggle", "nomad", "nymph", "octopus", "ogre", "oko", "ongoing", "ooze", "orc", "orgg", "otter", "ouphe", "ox", "oyster", 
    "pangolin", "paratrooper", "peasant", "pegasus", "penguin", "pest", "phelddagrif", "phenomenon", "phoenix", "phyrexia", "phyrexian", "pilot", 
    "pirate", "plains", "plane", "planeswalker", "plant", "player", "powerstone", "praetor", "processor", "proper", "pyrulea", "rabbit", "rabiah", 
    "raccoon", "ral", "ranger", "rat", "rath", "ravnica", "realm", "rebel", "reflection", "regatha", "rhino", "rigger", "rogue", "rowan", "rune", 
    "sable", "saga", "saheeli", "salamander", "samurai", "samut", "saproling", "sarkhan", "satyr", "scarecrow", "scheme", "scientist", "scorpion", 
    "scout", "segovia", "serpent", "serra", "shade", "shaman", "shandalar", "shapeshifter", "shark", "sheep", "shenmeng", "ship", "shrine", "siege", 
    "siren", "skeleton", "slith", "sliver", "slug", "snake", "snow", "soldier", "soltari", "sorcery", "sorin", "spawn", "specter", "spellshaper", 
    "sphere", "sphinx", "spider", "spike", "spirit", "sponge", "spy", "squid", "squirrel", "starfish", "summon", "surrakar", "survivor", "swamp", 
    "szat", "tamiyo", "tarkir", "tasha", "teferi", "tentacle", "tetravite", "teyo", "tezzeret", "thalakos", "theros", "thopter", "thrull", "tibalt", 
    "tiefling", "token", "tower", "townsfolk", "trap", "treasure", "treefolk", "tribal", "trilobite", "triskelavite", "troll", "turtle", "tyvar",
    "ugin", "ulgrotha", "unicorn", "urza", "valla", "vanguard", "vampire", "vampyre", "vedalken", "vehicle", "venser", "viashino", "villain", 
    "vivien", "volver", "vraska", "vryn", "waiter", "wall", "walrus", "warlock", "warrior", "weird", "werewolf", "whale", "wildfire", "will", "cur",
    "windgrace", "wizard", "wolf", "wolverine", "wombat", "worker", "world", "worm", "wraith", "wrenn", "wrestler", "wurm", "xenagos", "xerex", 
    "yanggu", "yanling", "yeti", "zariel", "zendikar", "zhalfir", "zombie", "zubera", "tobecast", "sur", "tomb", "of", "annihilation", "agadeem", 
    "the", "undercrypt", "tobereturn", "marauder", "airlift", "chaplain", "valiant", "protector", "akoum", "teeth", "watchdog", "alpine", "igneous",  
    "whispering", "raven", "ancestral", "anger", "day", "night", "amplifire", "destiny", "marauders", "anthem", "treasureartifacttoken", "inkling",
    "disenchant", "terror", "braingeyser", "shivan", "regrowth", "lotus", "br", "monstrous","Arni", "Brokenbrow", "bloodletting", "phandelver", 
    "affected", "gathering", "goliath", "crossbreed", " labs", "watermark", "ru", "genesis", "mage", "brambleweft", "behemoth", "gladewalker", 
    "ritualist", "jiang", "m", "control", "two", "or", "more", "vampires", "gr", "rb", "peer", "through", "depths", "mists", "kgoblin", "kfox", 
    "kmoonfolk", "krat", "ksnake", "a", "spell", "aether", "burst", "kjeldoran", "war", "cry", "one", "kind", "saclands", "less", "creatures", "rg",
    "artifacts", "enchantments", "lands", "t", "r", "b", "u", "d", "s", "w", "g", "l", "x", "e", "c", "p", "gw", "rw", "wu", "gu", "bg", "wb", 
    "h", "i", "ub", "q", "ur", "xx", "n", "color", "kindle", "scion", "skyshipped", "splinter", "stangg", "twin", "sunweb", "visitation", "vecna", 
    "mirror", "mad", "phantasm", "cost", "word", "autostack", "anycnt", "anytarget", "propagation", "proliferation"
    // Add any additional Wagic types here
};

std::vector<std::string> allVectors;

static void initLogger() {
    // Init the logger
    if (!logger || !logger.is_open()) {
        remove("WagicSyntaxPlugin.log");
        logger.open("WagicSyntaxPlugin.log", std::ios_base::app);
    }
}

static void stopLogger() {
    // Stop the logger and remove logfile
    if (logger && logger.is_open()) {
        logger.close();
        remove("WagicSyntaxPlugin.log");
    }
}

static void log(std::string logLevel, std::string text) {
    // Log the string with the selected loglevel
    initLogger();
    logger << "{" << logLevel << "} - " << text;
    logger.flush();
}

// Trim all the white spaces at the begin and the end of an UTF-8 stringa
static std::string trimUTF8(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wstr = converter.from_bytes(str);

    size_t first = wstr.find_first_not_of(L" \t\n\r\f\v");
    if (first == std::wstring::npos)
        return std::string();

    size_t last = wstr.find_last_not_of(L" \t\n\r\f\v");
    if (last == std::wstring::npos)
        return std::string();

    return converter.to_bytes(wstr.substr(first, last - first + 1));
}

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

        // Trim the white spaces
        nameValue = trimUTF8(nameValue);

        // Find the word in the substring
        size_t wordPos = nameValue.find(wordToCheck);
        if (wordPos != std::string::npos && pos > (openingPos + delta) && pos < (closingPos + delta))
            return true;

        // Try to seek row to search on next iteration
        delta += openingPos + openingTag.length() + 1;
        lineToCheck = lineToCheck.substr(openingPos + openingTag.length() + 1, lineToCheck.length());
    }
    return false;
}

static void SetStyles() {
    active = true;

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

    // Set the style for mismatched brackets
    ::SendMessage(nppData._scintillaMainHandle, SCI_STYLESETFORE, SCE_C_ESCAPESEQUENCE, RGB(255, 0, 0)); // Red style for errors

    // Init the logger
    initLogger();
}

static void ShowAbout() {
	ShowAboutDialog((HINSTANCE)dllModule, MAKEINTRESOURCE(IDD_ABOUTDLG), nppData._nppHandle);
}

static void DisablePlugin() {
    active = false;
    int endpos = editor.GetLength();
    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, 0, 0x1f);
    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endpos, SCE_C_OPERATOR);
    // Stop the logger
    stopLogger();
}

static void CheckWagicAllLinesSyntax() {
    SetStyles();
    currentLineCount = editor.GetLineCount();
    for (int i = 0; i < currentLineCount; i++)
        CheckWagicLineSyntax(i);
}

static void CheckWagicVisibleLinesSyntax(SCNotification* notification)
{
    SetStyles();
    int lineCount = editor.GetLineCount();
    int firstLine = editor.GetFirstVisibleLine();
    int visibleLineCount = editor.LinesOnScreen();
    int selStart = editor.GetSelectionStart();
    int selEnd = editor.GetSelectionEnd();
    std::string newText = editor.GetText();
    bool force = (notification->nmhdr.code == NPPN_BUFFERACTIVATED) || (notification->nmhdr.code == NPPN_FILEOPENED) ||
        (notification->nmhdr.code == NPPN_READY) || (notification->nmhdr.code == SCN_ZOOM) || (notification->nmhdr.code == SCN_FOCUSIN) ||
        ((notification->nmhdr.code == SCN_UPDATEUI && notification->updated == 1) && (newText == currentText)) ||
        ((notification->nmhdr.code == SCN_UPDATEUI && notification->updated == 1) && (newText != currentText) && (lineCount == currentLineCount)) ||
        ((notification->nmhdr.code == SCN_UPDATEUI && notification->updated == 1) && (lineCount != currentLineCount));
    if (force || (currentText != newText) || (currentFirstLine != firstLine) || 
        (currentVisibleLineCount != visibleLineCount) || (currentLineCount != lineCount)) {
        int startcheck = 0;
        int endcheck = 0;
        if (!force && (firstLine > currentFirstLine)) {
            startcheck = currentFirstLine + visibleLineCount;
            endcheck = startcheck + (firstLine - currentFirstLine);
        }
        if (!force && (firstLine < currentFirstLine)) {
            startcheck = firstLine;
            endcheck = firstLine + (currentFirstLine - firstLine);
        }
        if (!force && (lineCount > currentLineCount)) {
            int currentPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETCURRENTPOS, 0, 0);
            int currentLine = ::SendMessage(nppData._scintillaMainHandle, SCI_LINEFROMPOSITION, currentPosition, 0);
            startcheck = currentLine - (lineCount - currentLineCount);
            endcheck = currentLine + (lineCount - currentLineCount);
        }
        if (!force && (lineCount < currentLineCount)) {
            if (lineCount > visibleLineCount) {
                int currentPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETCURRENTPOS, 0, 0);
                int currentLine = ::SendMessage(nppData._scintillaMainHandle, SCI_LINEFROMPOSITION, currentPosition, 0);
                startcheck = currentLine - (currentLineCount - lineCount);
                endcheck = currentLine + visibleLineCount - (currentLine - firstLine);
            }
            else {
                startcheck = firstLine + lineCount - (currentLineCount - lineCount);
                endcheck = firstLine + lineCount + (currentLineCount - lineCount);
            }
        }
        if (force || (endcheck - startcheck) > visibleLineCount) {
            startcheck = firstLine;
            endcheck = (lineCount > visibleLineCount) ? (firstLine + visibleLineCount) : (firstLine + lineCount);
        }
        CheckWagicLineSyntax(-1);
        if(startcheck < endcheck) {
            if (startcheck < 0)
                startcheck = 0;
            if (endcheck < lineCount)
                endcheck++;
            if (endcheck > lineCount)
                endcheck = lineCount;
            for (int i = startcheck; i < endcheck; i++)
                CheckWagicLineSyntax(i);
        }
    }
    currentFirstLine = firstLine;
    currentVisibleLineCount = visibleLineCount;
    currentLineCount = lineCount;
    currentText = newText;
}

static void CheckWagicVisibleLinesSyntax()
{
    SCNotification* notification = new SCNotification();
    notification->nmhdr.code = NPPN_BUFFERACTIVATED;
    CheckWagicVisibleLinesSyntax(notification);
}

static LRESULT HandleScnModified(SCNotification* notification) {
    int checked = 0;
    // Get the info about the current line and position
    int currentPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETCURRENTPOS, 0, 0);
    int currentLine = ::SendMessage(nppData._scintillaMainHandle, SCI_LINEFROMPOSITION, currentPosition, 0);
    int lineStartPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_POSITIONFROMLINE, currentLine, 0);
    int lineEndPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_GETLINEENDPOSITION, currentLine, 0);
    int wordStartPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_WORDSTARTPOSITION, currentPosition, true);
    int wordEndPosition = ::SendMessage(nppData._scintillaMainHandle, SCI_WORDENDPOSITION, currentPosition, true);
    if (notification->modificationType & (SC_MOD_INSERTTEXT)) {
        if (notification->text != NULL)
            checked = -1;
        // Check if the current row needs to receive suggestions or not
        std::string lineText = editor.GetLine(currentLine);
        if (!(lineText.find("text=") != 0 && lineText.find("partner=") != 0 && lineText.find("backside=") != 0 &&
            lineText.find("name=") != 0 && lineText.find("power=") != 0 && lineText.find("toughness=") != 0 &&
            lineText.find("type=") != 0 && lineText.find("subtype=") != 0 && lineText.find("grade=") != 0))
            checked = -1;
        if (lineText[0] == '#' && lineText.find("#AUTO_DEFINE") == std::string::npos)
            checked = -1;
        int index = lineText.find("#AUTO_DEFINE");
        if (index < 0) {
            index = lineText.find("=");
            if (index < 0)
                checked = -1;
        }
        if (!checked && (wordStartPosition != wordEndPosition)) {
            std::string currentWord = editor.GetText().substr(wordStartPosition, wordEndPosition);
            // Remove all the chars which can disturb the word recognition
            currentWord = currentWord.substr(0, currentWord.find_first_of('\r'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('\n'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('\t'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('\"'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(' '));
            currentWord = currentWord.substr(0, currentWord.find_first_of(')'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('('));
            currentWord = currentWord.substr(0, currentWord.find_first_of(']'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(']'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('{'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('}'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('$'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('!'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(':'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('^'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('/'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('<'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('>'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(','));
            currentWord = currentWord.substr(0, currentWord.find_first_of('.'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('|'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('='));
            currentWord = currentWord.substr(0, currentWord.find_first_of('-'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('+'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('%'));
            currentWord = currentWord.substr(0, currentWord.find_first_of(';'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('*'));
            currentWord = currentWord.substr(0, currentWord.find_first_of('&'));
            // Include some special chars in the word (e.g for triggers and macros)
            if (editor.GetText().substr(wordStartPosition - 1, 1) == "@" || editor.GetText().substr(wordStartPosition - 1, 1) == "_" ||
                editor.GetText().substr(wordStartPosition - 1, 1) == "~")
                currentWord = editor.GetText().substr(wordStartPosition - 1, 1) + currentWord;
            // Fill the list of possibile suggestions matching the current word
            std::vector<std::string> matchingSuggestions;
            for (const std::string& suggestion : allVectors) {
                if (suggestion.compare(0, currentWord.length(), currentWord) == 0) {
                    matchingSuggestions.push_back(suggestion);
                }
            }
            if (!matchingSuggestions.empty()) {
                // Merge all the suggestions in a sorted unique string with '\n' sperator
                std::sort(matchingSuggestions.begin(), matchingSuggestions.end());
                std::string suggestionsString;
                for (const std::string& suggestion : matchingSuggestions) {
                    suggestionsString += suggestion + '\n';
                }
                // Set the suggestion menu of Notepad++
                editor.AutoCCancel();
                editor.AutoCShow(currentWord.length(), suggestionsString);
                editor.AutoCSetSeparator('\n');
                editor.AutoCCancel();
                editor.AutoCShow(currentWord.length(), suggestionsString);
                editor.AutoCSetSeparator('\n');
            }
            checked = 1;
        }
    }
    return checked;
}

static void CheckWagicLineSyntax(int i) {
    if (i < 0)
        i = editor.LineFromPosition(editor.GetCurrentPos());
    std::string lineText = editor.GetLine(i);
    std::transform(lineText.begin(), lineText.end(), lineText.begin(),
        [](unsigned char c) { return std::tolower(c); });
    // Check if it's a comment row
    if (lineText[0] == '#' && lineText.find("#auto_define") == std::string::npos) {
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
        else {
            offset += 12;
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
            bool found = false;
            std::string word = lineText.substr(startPos, endPos - startPos);
            if (!containsWordBetween("name(", ")", lineText, word, startPos) && !containsWordBetween("named!:", ":!", lineText, word, startPos)){
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
                    if (word == "from" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    }
                    if (word == "plus" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    }
                    if (word == "type" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    }
                    if (word == "countershroud" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    }
                    if (word == "token" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_OPERATOR);
                    }
                    if (word == "head" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != ')') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_OPERATOR);
                    }                    
                    if (word == "lord" && (endPos - offset - editor.PositionFromLine(i) < lineText.length()) &&
                        lineText[endPos - offset - editor.PositionFromLine(i)] != '(') {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_OPERATOR);
                    }
                    found = true;
                }
                else if (std::find(zones.begin(), zones.end(), word) != zones.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_COMMENTDOC);
                    found = true;
                }
                else if (std::find(constants.begin(), constants.end(), word) != constants.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    found = true;
                }
                else if (std::find(basicabilities.begin(), basicabilities.end(), word) != basicabilities.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_PREPROCESSOR);
                    found = true;
                }
                else if (std::find(types.begin(), types.end(), word) != types.end())
                {
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_OPERATOR);
                    found = true;
                }
                bool subfound = false;
                if (!found && !word.empty() && (word.find("halfup") != std::string::npos || word.find("halfdown") != std::string::npos ||
                    word.find("thirdup") != std::string::npos || word.find("thirddown") != std::string::npos ||
                    word.find("twice") != std::string::npos || word.find("thrice") != std::string::npos ||
                    word.find("fourtimes") != std::string::npos || word.find("fivetimes") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("halfup");
                    int toklen = 6;
                    if (starttok < 0) {
                        starttok = word.find("halfdown");
                        toklen = 8;
                    }
                    if (starttok < 0) {
                        starttok = word.find("thirdup");
                        toklen = 7;
                    }
                    if (starttok < 0) {
                        starttok = word.find("thirddown");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("twice");
                        toklen = 5;
                    }
                    if (starttok < 0) {
                        starttok = word.find("thrice");
                        toklen = 6;
                    }
                    if (starttok < 0) {
                        starttok = word.find("fourtimes");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("fivetimes");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_COMMENTDOC);
                        }
                        else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_OPERATOR);
                        }
                        else if (!subword.empty() && (word.find("stored") == std::string::npos) && (word.find("mytarg") == std::string::npos) &&
                            (word.find("hasability") == std::string::npos) && (word.find("hascnt") == std::string::npos) &&
                            (word.find("cardcountabil") == std::string::npos) && (word.find("cardcounttype") == std::string::npos)) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("stored") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("stored");
                    int toklen = 6;
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        int startsubtok = subword.find("plus");
                        int subtoklen = 4;
                        if (startsubtok < 0) {
                            startsubtok = subword.find("minus");
                            subtoklen = 5;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("twice");
                            subtoklen = 4;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("halfup");
                            subtoklen = 6;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("halfdown");
                            subtoklen = 8;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thirdup");
                            subtoklen = 7;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thirddown");
                            subtoklen = 9;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thrice");
                            subtoklen = 6;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("fourtimes");
                            subtoklen = 9;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("fivetimes");
                            subtoklen = 9;
                        }
                        if (startsubtok >= 0) {
                            subword = subword.substr(0, startsubtok);
                        }
                        else {
                            startsubtok = 0;
                            subtoklen = 0;
                        }
                        if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (!subword.empty() && subtoklen == 0 && (word.find("hascnt") == std::string::npos) && 
                            (word.find("hasability") == std::string::npos) && (word.find("cardcounttype") == std::string::npos) &&
                            (word.find("cardcountabil") == std::string::npos)) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("mytarg") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("mytarg");
                    int toklen = 6;
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        int startsubtok = subword.find("plus");
                        int subtoklen = 4;
                        if (startsubtok < 0) {
                            startsubtok = subword.find("minus");
                            subtoklen = 5;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("twice");
                            subtoklen = 4;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("halfup");
                            subtoklen = 6;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("halfdown");
                            subtoklen = 8;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thirdup");
                            subtoklen = 7;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thirddown");
                            subtoklen = 9;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("thrice");
                            subtoklen = 6;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("fourtimes");
                            subtoklen = 9;
                        }
                        if (starttok < 0) {
                            startsubtok = word.find("fivetimes");
                            subtoklen = 9;
                        }
                        if (startsubtok >= 0) {
                            subword = subword.substr(0, startsubtok);
                        }
                        else {
                            startsubtok = 0;
                            subtoklen = 0;
                        }
                        if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (!subword.empty() && subtoklen == 0 && (word.find("hascnt") == std::string::npos) &&
                            (word.find("hasability") == std::string::npos) && (word.find("cardcounttype") == std::string::npos) &&
                            (word.find("cardcountabil") == std::string::npos)) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("hascnt") != std::string::npos || word.find("totcnt") != std::string::npos || word.find("cardcounttype") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("hascnt");
                    int toklen = 6;
                    if (starttok < 0) {
                        starttok = word.find("totcntcre");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntpla");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntart");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntenc");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntlan");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntbat");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("totcntall");
                        toklen = 9;
                    }
                    if (starttok < 0) {
                        starttok = word.find("diffcardcounttype");
                        toklen = 17;
                    }
                    if (starttok < 0) {
                        starttok = word.find("cardcounttype");
                        toklen = 13;
                    }
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                    }
                }
                if (!found && !word.empty() && (word.find("hasability") != std::string::npos || word.find("cardcountabil") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("hasability");
                    int toklen = 10;
                    if (starttok < 0) {
                        starttok = word.find("diffcardcountabil");
                        toklen = 17;
                    }
                    if (starttok < 0) {
                        starttok = word.find("cardcountabil");
                        toklen = 13;
                    }
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        int startsubtok = subword.find("plus");
                        int subtoklen = 4;
                        if (startsubtok < 0) {
                            startsubtok = subword.find("minus");
                            subtoklen = 5;
                        }
                        if (startsubtok < 0) {
                            startsubtok = subword.find("math");
                            subtoklen = 4;
                        }
                        if (startsubtok > 0) {
                            subword = subword.substr(0, startsubtok);
                        }
                        else {
                            startsubtok = 0;
                            subtoklen = 0;
                        }
                        if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (!subword.empty() && subtoklen == 0) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("myhasdead") != std::string::npos || word.find("oppohasdead") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("myhasdead");
                    int toklen = 9;
                    if (starttok < 0) {
                        starttok = word.find("oppohasdead");
                        toklen = 11;
                    }
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        subword = word.substr(starttok + toklen, word.length() - toklen - starttok);
                        if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (!subword.empty()) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("math") != std::string::npos)) {
                    subfound = true;
                    int starttok = word.find("math");
                    int endtok = word.find("mathend");
                    if (endtok < 0)
                        endtok = word.length();
                    int toklen = 4;
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(0, starttok);
                        if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_COMMENTDOC);
                        }
                        else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_OPERATOR);
                        }
                        else if (!subword.empty() && (subword.find("plus") == std::string::npos) && 
                            (subword.find("minus") == std::string::npos) && (subword.find("hascnt") == std::string::npos) &&
                            (subword.find("stored") == std::string::npos) && (subword.find("hasability") == std::string::npos) &&
                            (subword.find("cardcounttype") == std::string::npos) && (subword.find("cardcountabil") == std::string::npos)) {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                        subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        if (endtok >= 0 && endtok >= starttok) {
                            subword = word.substr(starttok + toklen, endtok - (starttok + toklen));
                            int startsubtok = subword.find("plus");
                            int subtoklen = 4;
                            if (startsubtok < 0) {
                                startsubtok = subword.find("minus");
                                subtoklen = 5;
                            }
                            if (startsubtok > 0) {
                                subword = subword.substr(startsubtok, subtoklen);
                            }
                            else {
                                startsubtok = 0;
                                subtoklen = 0;
                            }
                            int startsubtok2 = subword.find("hascnt");
                            int subtoklen2 = 6;
                            if (startsubtok2 >= 0) {
                                subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("diffcardcounttype");
                                subtoklen2 = 17;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(startsubtok2, startsubtok2 - (subtoklen2 - 1));
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("cardcounttype");
                                subtoklen2 = 13;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(startsubtok2, startsubtok2 - (subtoklen2 - 1));
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("hasability");
                                subtoklen2 = 10;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                    if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (!subword.empty()) {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                        log("ERR", text);
                                    }
                                }
                                if (startsubtok2 < 0) {
                                    startsubtok2 = subword.find("diffcardcountabil");
                                    subtoklen2 = 17;
                                    if (startsubtok2 >= 0) {
                                        subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                        if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                        {
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                        }
                                        else if (!subword.empty()) {
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                            log("ERR", text);
                                        }
                                    }
                                    if (startsubtok2 < 0) {
                                        startsubtok2 = subword.find("cardcountabil");
                                        subtoklen2 = 13;
                                        if (startsubtok2 >= 0) {
                                            subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                            if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                            {
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                            }
                                            else if (!subword.empty()) {
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                                std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                                log("ERR", text);
                                            }
                                        }
                                        if (startsubtok2 < 0) {
                                            startsubtok2 = subword.find("stored");
                                            subtoklen2 = 6;
                                            if (startsubtok2 >= 0) {
                                                subword = subword.substr(startsubtok2 + subtoklen2, subword.length() - (startsubtok2 + subtoklen2));
                                                if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                                                {
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                                }
                                                else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                                                {
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                                }
                                                else if (!subword.empty()) {
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                                    std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                                    log("ERR", text);
                                                }
                                            }
                                            if (startsubtok2 < 0) {
                                                startsubtok2 = subword.find("mytarg");
                                                subtoklen2 = 6;
                                                if (startsubtok2 >= 0) {
                                                    subword = subword.substr(startsubtok2 + subtoklen2, subword.length() - (startsubtok2 + subtoklen2));
                                                    if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                                                    {
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                                    }
                                                    else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                                                    {
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                                    }
                                                    else if (!subword.empty()) {
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                                        log("ERR", text);
                                                    }
                                                }
                                                else {
                                                    startsubtok2 = 0;
                                                    subtoklen2 = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if(startsubtok2 < 0) {
                                startsubtok2 = subword.find("hasability");
                                subtoklen2 = 10;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                    if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (!subword.empty()) {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                        log("ERR", text);
                                    }
                                }
                                else {
                                    startsubtok2 = 0;
                                    subtoklen2 = 0;
                                }
                            }
                            if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_COMMENTDOC);
                            }
                            else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(types.begin(), types.end(), subword) != types.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_OPERATOR);
                            }
                            else if (!subword.empty() && subtoklen2 == 0) {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                log("ERR", text);
                            }
                            subword = word.substr(endtok, toklen + 3);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + endtok, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                    }
                }
                if (!found && !word.empty() && (word.find("minus") != std::string::npos || word.find("plus") != std::string::npos)) {
                    subfound = true; 
                    int starttok = word.find("plus");
                    int endtok = word.find("plusend");
                    int toklen = 4;
                    if(starttok < 0){
                        starttok = word.find("minus");
                        endtok = word.find("minusend");
                        toklen = 5;
                    }
                    if (endtok < 0)
                        endtok = word.length();
                    if (starttok < 0) {
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_ESCAPESEQUENCE);
                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                        log("ERR", text);
                    }
                    else {
                        std::string subword = word.substr(0, starttok);
                        int startsubtok = subword.find("math");
                        int subtoklen = 4;
                        if (startsubtok >= 0) {
                            subword = subword.substr(subtoklen, subword.length() - subtoklen);
                        }
                        else {
                            startsubtok = 0;
                            subtoklen = 0;
                        }
                        int startsubtok2 = subword.find("hascnt");
                        int subtoklen2 = 6;
                        if (startsubtok2 >= 0) {
                            subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        if (startsubtok2 < 0) {
                            startsubtok2 = subword.find("hasability");
                            subtoklen2 = 10;
                            if (startsubtok2 >= 0) {
                                subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                                else if (!subword.empty()) {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                    std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                    log("ERR", text);
                                }
                            }
                        }
                        if (startsubtok2 < 0) {
                            startsubtok2 = subword.find("stored");
                            subtoklen2 = 6;
                            if (startsubtok2 >= 0) {
                                subword = subword.substr(startsubtok2 + subtoklen2, subword.length() - (startsubtok2 + subtoklen2));
                                if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                                {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                                else if (std::find(types.begin(), types.end(), subword) != types.end())
                                {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                                else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                                {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                                else if (!subword.empty()) {
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                    std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                    log("ERR", text);
                                }
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("mytarg");
                                subtoklen2 = 6;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(startsubtok2 + subtoklen2, subword.length() - (startsubtok2 + subtoklen2));
                                    if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (std::find(types.begin(), types.end(), subword) != types.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (!subword.empty()) {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                        log("ERR", text);
                                    }
                                }
                                else {
                                    startsubtok2 = 0;
                                    subtoklen2 = 0;
                                }
                            }
                        }
                        if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_COMMENTDOC);
                        }
                        else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (std::find(types.begin(), types.end(), subword) != types.end())
                        {
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                        else if (!subword.empty() && subtoklen2 == 0 && (subword.find("cardcounttype") != std::string::npos) && (subword.find("cardcountabil") != std::string::npos)){
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                            log("ERR", text);
                        }
                        subword = word.substr(starttok, toklen);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        if (endtok >= 0 && endtok >= starttok) {
                            subword = word.substr(starttok + toklen, endtok - (starttok + toklen));
                            int startsubtok2 = subword.find("hascnt");
                            int subtoklen2 = 6;
                            if (startsubtok2 >= 0) {
                                subword = subword.substr(startsubtok2, startsubtok2 - (subtoklen2 - 1));
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("diffcardcounttype");
                                subtoklen2 = 17;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(startsubtok2, startsubtok2 - (subtoklen2 - 1));
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("cardcounttype");
                                subtoklen2 = 13;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(startsubtok2, startsubtok2 - (subtoklen2 - 1));
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                }
                            }
                            if (startsubtok2 < 0) {
                                startsubtok2 = subword.find("hasability");
                                subtoklen2 = 10;
                                if (startsubtok2 >= 0) {
                                    subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                    if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                    {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                    }
                                    else if (!subword.empty()) {
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                        std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                        log("ERR", text);
                                    }
                                }
                                if (startsubtok2 < 0) {
                                    startsubtok2 = subword.find("diffcardcountabil");
                                    subtoklen2 = 17;
                                    if (startsubtok2 >= 0) {
                                        subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                        if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                        {
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                        }
                                        else if (!subword.empty()) {
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                            std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                            log("ERR", text);
                                        }
                                    }
                                    if (startsubtok2 < 0) {
                                        startsubtok2 = subword.find("cardcountabil");
                                        subtoklen2 = 13;
                                        if (startsubtok2 >= 0) {
                                            subword = subword.substr(subtoklen2, startsubtok2 - (subtoklen2 - 1));
                                            if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                                            {
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen, 0x1f);
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                                            }
                                            else if (!subword.empty()) {
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok + subtoklen + startsubtok2 + subtoklen2, 0x1f);
                                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                                std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                                log("ERR", text);
                                            }
                                        }
                                        else {
                                            startsubtok2 = 0;
                                            subtoklen2 = 0;
                                        }
                                    }
                                }
                            }
                            if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(zones.begin(), zones.end(), subword) != zones.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_COMMENTDOC);
                            }
                            else if (std::find(constants.begin(), constants.end(), subword) != constants.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(basicabilities.begin(), basicabilities.end(), subword) != basicabilities.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (std::find(types.begin(), types.end(), subword) != types.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_OPERATOR);
                            }
                            else if (std::find(keywords.begin(), keywords.end(), subword) != keywords.end())
                            {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                            }
                            else if (!subword.empty() && subtoklen2 == 0 && (word.find("end") == std::string::npos)) {
                                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + starttok + toklen + startsubtok2 + subtoklen2, 0x1f);
                                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_ESCAPESEQUENCE);
                                std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + subword + "\"\n";
                                log("ERR", text);
                            }
                            subword = word.substr(endtok, toklen + 3);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos + endtok, 0x1f);
                            ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, subword.length(), SCE_C_PREPROCESSOR);
                        }
                    }
                }
                if(!found && !subfound && !word.empty() && !triggerWord  && !macroWord &&
                    ((startPos - offset - editor.PositionFromLine(i)) > 0 && lineText[startPos - offset - editor.PositionFromLine(i) - 1] != '_') &&
                    !containsWordBetween("all(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("target(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("token(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("create(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("cards(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("counteradded(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("counter{", "}", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("counter(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("counters(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("flip(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("doubleside(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("named!", "!", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("sacrifice(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("s(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("e(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
                    !containsWordBetween("c(", ")", lineText, word, startPos - offset - editor.PositionFromLine(i)) &&
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
                    std::string text = "unknown word in line (" + std::to_string(i) + "): \"" + word + "\"\n";
                    log("ERR", text);
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
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_TASKMARKER);
                    lastfound += trigger.length();
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
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, endPos - startPos, SCE_C_HASHQUOTEDSTRING);
                    lastfound += macro.length();
                }
            }
        }
        if (lineText.find('(') || lineText.find('[') || lineText.find('{') || lineText.find('<') || lineText.find('$') || lineText.find('!')) {
            // Check the all the unbalanced brackets
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
                    if ((currentChar == ')' && bracketsStack1.empty()) || (currentChar == ']' && bracketsStack2.empty()) || 
                        (currentChar == '}' && bracketsStack3.empty()) || (currentChar == '$' && bracketsStack4.empty())) {
                        // Unbalanced closing brackets
                        ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                        ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);
                    }
                    else if ((currentChar == ')' && !bracketsStack1.empty()) || (currentChar == ']' && !bracketsStack2.empty()) || 
                        (currentChar == '}' && !bracketsStack3.empty()) || (currentChar == '$' && !bracketsStack4.empty())) {
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
                            // Unbalanced opening brackets at the end of row
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
                // Unbalanced opening round brackets
                int startPos = bracketsStack1.top();
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

                bracketsStack1.pop();
            }
            // Check if there are '[' still remaining
            while (!bracketsStack2.empty()) {
                // Unbalanced opening square brackets
                int startPos = bracketsStack2.top();
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

                bracketsStack2.pop();
            }
            // Check if there are '{' still remaining
            while (!bracketsStack3.empty()) {
                // Unbalanced opening brace curly brackets
                int startPos = bracketsStack3.top();
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

                bracketsStack3.pop();
            }
            //Check if there are '$' still remaining
            while (!bracketsStack4.empty()) {
                // Unbalanced opening dollar char
                int startPos = bracketsStack4.top();
                ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);

                bracketsStack4.pop();
            }
            // Check if there are '!' still remaining
            int even = bracketsStack5.size() % 2;
            while (!bracketsStack5.empty()) {
                // Unbalanced opening mark char
                if (even != 0) {
                    int startPos = bracketsStack5.top();
                    ::SendMessage(nppData._scintillaMainHandle, SCI_STARTSTYLING, startPos, 0x1f);
                    ::SendMessage(nppData._scintillaMainHandle, SCI_SETSTYLING, 1, SCE_C_ESCAPESEQUENCE);
                }
                bracketsStack5.pop();
            }
        }
    }
}

static void SetCurrentEditor() {
    int which = -1;
    SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, SCI_UNUSED, (LPARAM)&which);
    editor = (which == 0) ? editor1 : editor2;
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
    // Check the current text to understand if it's a Wagic primitive file.
    currentText = editor.GetText();
    if ((currentText.find("[card]") != std::string::npos) || (currentText.find("[/card]") != std::string::npos) ||
        (currentText.find("grade=") != std::string::npos) || (currentText.find("#AUTO_DEFINE") != std::string::npos))
        SetStyles();
    else
        DisablePlugin();
}

LRESULT CALLBACK PluginWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // Check if the user pressed the HOTKEY
    if (active && message == WM_HOTKEY) {
        SCNotification* notification = new SCNotification();
        notification->modificationType = SC_MOD_INSERTTEXT;
        HandleScnModified(notification);
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
        if (active)
            CheckWagicVisibleLinesSyntax(notifyCode);
        break;
    case NPPN_FILEOPENED:
    case NPPN_READY:
    case SCN_FOCUSIN:
    case SCN_UPDATEUI:
    case SCN_ZOOM:
        if (active)
            CheckWagicVisibleLinesSyntax(notifyCode);
        break;
    case SCN_MODIFIED:
        if (active)
            HandleScnModified(notifyCode);
        break;
    case NPPN_SHUTDOWN:
        if(active)
            DisablePlugin();
        break;
    }
}

// Plugin initialization
extern "C" __declspec(dllexport) void setInfo(NppData nppData) {
    ::nppData = nppData;
    // Retrive the current editor
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
