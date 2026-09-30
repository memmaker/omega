/*
Omega copyright (C) by Laurence Raphael Brothers, 1987,1988,1989
Modifications copyright (C) by Lyle Tafoya, 2019, 2021-2023

This file is part of Omega.

Omega is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Omega is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Omega. If not, see <https://www.gnu.org/licenses/>.
*/

// inv.cpp
// functions having to do with player item inventory

#include "glob.h"
#include "scr.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <span>
#include <string>
#include <vector>

int objequal(const object *, const object *);
int pack_item_cost(size_t);
bool merge_item_with_pack(const object *);

// returns some money from player back into "money" item.
// for giving and dropping money
std::unique_ptr<object> detach_money()
{
  long c = get_money(Player.cash);
  if(c == ABORT)
  {
    return nullptr;
  }
  else
  {
    Player.cash -= c;
    auto cash = std::make_unique<object>();
    make_cash(cash.get(), difficulty());
    cash->basevalue = c;
    return cash;
  }
}

// drops money, heh heh
void drop_money()
{

  std::unique_ptr<object> money = detach_money();
  if(money)
  {
    if(Current_Environment == E_CITY)
    {
      queue_message("As soon as the money leaves your hand,");
      queue_message("a horde of scrofulous beggars snatch it up and are gone!");
    }
    else
    {
      drop_at(Player.x, Player.y, std::move(money));
    }
  }
  else
  {
    setgamestatus(SKIP_MONSTERS, GameStatus);
  }
}

// gets a legal amount of money or ABORT
long get_money(long limit)
{
  queue_message("How much? ");
  long c = parsenum();
  if(c > limit || c <= 0)
  {
    queue_message("Forget it, buddy.");
    return ABORT;
  }
  else
  {
    return c;
  }
}

// pick up from some location x,y
void pickup_at(int x, int y)
{
  resetgamestatus(FAST_MOVE, GameStatus);

  std::vector<std::unique_ptr<object>> &items = Level->site[x][y].things;
  if(items.size() == 1)
  {
    std::unique_ptr<object> tmp = std::move(items.back());
    items.pop_back();
    gain_item(std::move(tmp));
  }
  else
  {
    // port: one checklist for the pile; Enter with nothing marked takes the highlighted item
    std::vector<std::string> names;
    for(auto &item : items)
    {
      names.push_back(itemid(item.get()));
    }
    std::vector<bool> marks(items.size());
    int sel = rl_choose_keys("Pick up what? (a key or Space marks, ',' all, Enter takes)", names, {}, 0, &marks);
    if(sel < 0)
    {
      setgamestatus(SKIP_MONSTERS, GameStatus);
      return;
    }
    if(std::find(marks.begin(), marks.end(), true) == marks.end())
    {
      marks[sel] = true;
    }
    for(auto i = items.size(); i-- > 0;)
    {
      if(marks[i])
      {
        std::unique_ptr<object> tmp = std::move(items[i]);
        items.erase(items.begin() + i);
        gain_item(std::move(tmp));
      }
    }
  }
}

// port: auto-pickup (PICKUP option) takes only money and items that stack with carried ones
void auto_pickup()
{
  if(Player.status[SHADOWFORM])
  {
    return;
  }
  std::vector<std::unique_ptr<object>> &items = Level->site[Player.x][Player.y].things;
  for(auto i = items.size(); i-- > 0;)
  {
    const object *o = items[i].get();
    bool stacks     = o->objchar == CASH;
    for(auto &p : Player.pack)
    {
      stacks = stacks || (o->objchar != STICK && objequal(o, p.get()));
    }
    for(auto &p : Player.possessions)
    {
      stacks = stacks || (o->objchar != STICK && objequal(o, p.get()));
    }
    if(stacks)
    {
      std::unique_ptr<object> tmp = std::move(items[i]);
      items.erase(items.begin() + i);
      gain_item(std::move(tmp));
    }
  }
}

// criteria for being able to put some item in some slot
int aux_slottable(const object *o, int slot)
{
  bool ok = true;
  if(!o)
  {
    ok = false;
  }
  else if(slot == O_ARMOR)
  {
    if(o->objchar != ARMOR)
    {
      ok = false;
    }
  }
  else if(slot == O_SHIELD)
  {
    if(o->objchar != SHIELD)
    {
      ok = false;
    }
  }
  else if(slot == O_BOOTS)
  {
    if(o->objchar != BOOTS)
    {
      ok = false;
    }
  }
  else if(slot == O_CLOAK)
  {
    if(o->objchar != CLOAK)
    {
      ok = false;
    }
  }
  else if(slot >= O_RING1)
  {
    if(o->objchar != RING)
    {
      ok = false;
    }
  }
  return ok;
}

// are two objects equal except for their number field?
// returns false if either object is null
int objequal(const object *o, const object *p)
{
  if(!o || !p)
  {
    return false;
  }
  else
  {
    return o->id == p->id && o->weight == p->weight && o->plus == p->plus &&
      o->charge == p->charge && o->dmg == p->dmg && o->hit == p->hit &&
      o->aux == p->aux && o->fragility == p->fragility &&
      o->basevalue == p->basevalue&& o->known == p->known &&
      o->blessing == p->blessing && o->on_use == p->on_use &&
      o->on_equip == p->on_equip && o->on_unequip == p->on_unequip;
  }
}

bool merge_item(std::span<std::unique_ptr<object>> items, const object *o, int n)
{
  for(std::unique_ptr<object> &item : items)
  {
    if(!item)
    {
      continue;
    }
    if(o->objchar == CASH && item->objchar == CASH)
    {
      item->basevalue += o->basevalue;
      return true;
    }
    else if(objequal(item.get(), o) && item->objchar != STICK)
    {
      item->number += n;
      return true;
    }
  }
  return false;
}

// put all of o on objlist at x,y on Level->depth
// Not necessarily dropped by character; just dropped...
void drop_at(int x, int y, std::unique_ptr<object> o)
{
  if(Current_Environment != E_COUNTRYSIDE)
  {
    if((Level->site[x][y].locchar != VOID_CHAR) && (Level->site[x][y].locchar != ABYSS))
    {
      if(merge_item(Level->site[x][y].things, o.get(), o->number))
      {
        return;
      }
      o->used = false;
      Level->site[x][y].things.emplace_back(std::move(o));
    }
    else if(Level->site[x][y].p_locf == L_VOID_STATION)
    {
      setgamestatus(PREPARED_VOID, GameStatus);
    }
  }
}

// put n of o on objlist at x,y on Level->depth
void p_drop_at(int x, int y, int n, object *o)
{
  if(Current_Environment != E_COUNTRYSIDE)
  {
    if((Level->site[x][y].locchar != VOID_CHAR) && (Level->site[x][y].locchar != ABYSS))
    {
      queue_message(std::format("Dropped {}", itemid(o)));
      if(merge_item(Level->site[x][y].things, o, n))
      {
        return;
      }
      auto cpy = std::make_unique<object>(*o);
      cpy->used   = false;
      cpy->number = n;
      Level->site[x][y].things.emplace_back(std::move(cpy));
    }
    else if(Level->site[x][y].p_locf == L_VOID_STATION)
    {
      setgamestatus(PREPARED_VOID, GameStatus);
    }
  }
}

// return an object's plus as a string
std::string getplusstr(const object *obj)
{
  if(obj->plus < 0)
  {
    return std::to_string(obj->plus);
  }
  else
  {
    return "+" + std::to_string(obj->plus);
  }
}

// return object with charges
std::string getchargestr(const object *obj)
{
  std::string chargestr = " [";
  if(obj->charge < 0)
  {
    chargestr += "dead]";
  }
  else
  {
    chargestr += std::to_string(obj->charge) + "]";
  }
  return chargestr;
}

// return an object's number as a string
std::string getnumstr(const object *obj)
{
  std::string numstr;
  if(obj->number > 1 && obj->number < 41)
  {
    numstr += std::to_string(obj->number) + "x ";
  }
  else if(obj->number > 40)
  {
    numstr = "lots of ";
  }
  return numstr;
}

// returns a string for identified items
std::string itemid(object *obj, int known)
{
  std::string item_name;
  if(obj->objchar == CASH)
  {
    return obj->truename;
  }
  else
  {
    if(Objects[obj->id].known > obj->known)
    {
      obj->known = Objects[obj->id].known;
    }
    if(known < 0)
    {
      known = obj->known;
    }

    item_name += getnumstr(obj);
    if(known == 0)
    {
      item_name += obj->objstr;
    }
    else if(known == 1)
    {
      if(obj->id == ARTIFACTID + 8 || obj->id == ARTIFACTID + 20 || obj->id == ARTIFACTID + 21)
      {
        item_name += obj->objstr;
      }
      item_name += obj->truename;
    }
    else
    {
      if(obj->id == ARTIFACTID + 8 || obj->id == ARTIFACTID + 20 || obj->id == ARTIFACTID + 21)
      {
        item_name += "the ";
      }
      if(obj->on_use == I_NOTHING && Objects[obj->id].on_use != I_NOTHING &&
         obj->on_equip == I_NOTHING && Objects[obj->id].on_equip != I_NOTHING &&
         obj->on_unequip == I_NOTHING && Objects[obj->id].on_unequip != I_NOTHING)
      {
        item_name += "disenchanted ";
      }
      if(obj->blessing < 0)
      {
        item_name += "cursed " + obj->cursestr;
      }
      else if(obj->blessing > 0)
      {
        item_name += "blessed " + obj->truename;
      }
      else
      {
        item_name += obj->truename;
      }
      if(obj->number > 1)
      {
        item_name += "s";
      }
      switch(obj->objchar)
      {
        case STICK:
          item_name += getchargestr(obj);
          break;
        case MISSILEWEAPON:
        case ARMOR:
        case RING:
        case SHIELD:
        case WEAPON:
          item_name += " " + getplusstr(obj);
          break;
        default:
          break;
      }
      if(obj->objchar == WEAPON)
      {
        item_name += std::format(" ({},{})", obj->hit + obj->plus, obj->dmg + obj->plus);
      }
      else if(obj->objchar == ARMOR)
      {
        item_name += std::format(" [{},{}]", obj->plus - obj->aux, obj->dmg);
      }
      else if(obj->objchar == SHIELD)
      {
        item_name += std::format(" [{},0]", obj->plus + obj->aux);
      }
    }
    return item_name;
  }
}

const std::string cashstr()
{
  if(difficulty() < 3)
  {
    return "copper pieces";
  }
  else if(difficulty() < 5)
  {
    return "silver pieces";
  }
  else if(difficulty() < 7)
  {
    return "gold pieces";
  }
  else if(difficulty() < 8)
  {
    return "semiprecious gems";
  }
  else if(difficulty() < 9)
  {
    return "mithril pieces";
  }
  else if(difficulty() < 10)
  {
    return "precious gems";
  }
  else
  {
    return "orichalc pieces";
  }
}

void give_money(monster *m)
{
  std::unique_ptr<object> cash = detach_money();
  if(!cash)
  {
    setgamestatus(SKIP_MONSTERS, GameStatus);
  }
  else
  {
    givemonster(m, std::move(cash));
  }
}

void givemonster(monster *m, std::unique_ptr<object> o)
{
  // special case -- give gem to LawBringer
  if((m->id == LAWBRINGER) && (o->id == ARTIFACTID + 21))
  {
    queue_message("The LawBringer accepts the gem reverently.");
    queue_message("He raises it above his head, where it bursts into lambent flame!");
    queue_message("You are bathed in a shimmering golden light.");
    queue_message("You feel embedded in an infinite matrix of ordered energy.");
    if(Imprisonment > 0)
    {
      Imprisonment = 0;
    }
    if(Player.rank[ORDER] == -1)
    {
      queue_message("You have been forgiven. You feel like a Paladin....");
      Player.rank[ORDER] = 1;
    }
    Player.alignment += 200;
    Player.pow = Player.maxpow = Player.pow * 2;
    gain_experience(2000);
    setgamestatus(GAVE_STARGEM, GameStatus);
  }
  else
  {
    std::string monster_name;
    if(m->uniqueness == COMMON)
    {
      monster_name = std::format("The {}", m->monstring);
    }
    else
    {
      monster_name = m->monstring;
    }

    if(m_statusp(*m, GREEDY) || m_statusp(*m, NEEDY))
    {
      long value = true_item_value(o.get());
      m_pickup(m, std::move(o));
      queue_message(std::format("{} takes your gift.", monster_name));
      Player.alignment++;
      if(m_statusp(*m, GREEDY) && (value < (long)m->level * 100))
      {
        append_message("...but does not appear satisfied.");
      }
      else if(m_statusp(*m, NEEDY) && (value < (long)Level->depth * Level->depth))
      {
        append_message("...and looks chasteningly at you.");
      }
      else
      {
        append_message("...and seems happy with it.");
        m_status_reset(*m, HOSTILE);
        m_status_reset(*m, GREEDY);
        m_status_reset(*m, NEEDY);
      }
    }
    else if(m_statusp(*m, HUNGRY))
    {
      if(((m->id == HORSE) && (o->id == FOODID + 15)) || // grain
         ((m->id != HORSE) && ((o->on_use == I_FOOD) || (o->on_use == I_POISON_FOOD))))
      {
        queue_message(std::format("{} wolfs dwn your food ...", monster_name));
        m_status_reset(*m, HUNGRY);
        m_status_reset(*m, HOSTILE);
        if(o->on_use == I_POISON_FOOD)
        {
          Player.alignment -= 2;
          append_message("...and chokes on the poisoned ration!");
          m_status_set(*m, HOSTILE);
          m_damage(m, 100, POISON);
        }
        else
        {
          append_message("...and now seems satiated.");
        }
      }
      else
      {
        queue_message(std::format("{} spurns your offering and leaves it on the ground.", monster_name));
        drop_at(m->x, m->y, std::move(o));
      }
    }
    else
    {
      queue_message(std::format("{} doesn't care for your offering and drops it.", monster_name));
      drop_at(m->x, m->y, std::move(o));
    }
  }
}

// removes n of object from inventory; frees object if appropriate
void dispose_lost_objects(int n, int slot)
{
  std::unique_ptr<object> &item = Player.possessions[slot];
  if(!item)
  {
    return;
  }
  item->number -= n;
  if(item->number < 1)
  {
    conform_unused_object(item);
    item.reset();
  }
}

// clears unused possession
void conform_unused_object(std::unique_ptr<object> &obj)
{
  if(obj->used)
  {
    item_unequip(obj);
  }
  calc_melee();
}

// Item identifiers, in this case the letters of the alphabet minus
// any letters already used for commands.  Yes, there are more here
// than could be needed, but I don't want to short myself for later.
char inventory_keymap[] = "-abcfghimnoqruvwyz";

// WDT -- convert from a char (keypress) to an item index in player inventory
int key_to_index(signed char key)
{
  assert(MAXITEMS > 0); // must have room for an item, or this loop will die!

  for(int i = 0; i < MAXITEMS; ++i)
  {
    if(key == inventory_keymap[i])
    {
      return (signed char)i;
    }
  }
  return -1;
}

char index_to_key(signed int index)
{
  if(index < MAXITEMS)
  {
    return inventory_keymap[index];
  }
  else
  {
    return '-';
  }
}

// select an item from inventory
// if itype is NULL_ITEM, any kind of item is acceptable.
// if itype is CASH, any kind of item or '$' (cash) is acceptable.
// if itype is FOOD, CORPSE or FOOD is acceptable, but only FOOD is
// listed in the possibilities.
// if itype is any other object type (eg SCROLL, POTION, etc.), only
// that type of item is acceptable or is listed

// port: the slots and then the pack in one list (rl_choose_keys). Slots have
// their inventory letters, pack items the upper-case pack letter (A = top of
// the pack). A pack item is taken out for the command: it waits in slot 0
// (O_UP_IN_AIR, unused since rebirth) and rl_return_held() puts what is left
// back after the command; reaching into the pack costs pack_item_cost().
int Rl_rummage;
static size_t held_at;

static int pack_key(size_t i)
{
  return 'A' + static_cast<int>(Player.pack.size() - 1 - i);
}

static std::string pack_line(size_t i)
{
  int cost = pack_item_cost(i);
  return std::format("pack {:<9} {}", cost > 10 ? "**" : cost > 5 ? "*" : "", itemid(Player.pack[i].get()));
}

void rl_return_held()
{
  std::unique_ptr<object> &o = Player.possessions[O_UP_IN_AIR];
  if(!o || merge_item_with_pack(o.get()))
  {
    o.reset();
    return;
  }
  if(Player.pack.size() >= MAXPACK)
  {
    queue_message("Your pack is full. The item drops to the ground.");
    drop_at(Player.x, Player.y, std::move(o));
    return;
  }
  Player.pack.insert(Player.pack.begin() + std::min(held_at, Player.pack.size()), std::move(o));
}

static int hold_pack_item(size_t i)
{
  rl_return_held();
  int cost = pack_item_cost(i);
  if(cost > 5)
  {
    queue_message("You rummage through your pack for the item.");
  }
  Rl_rummage += cost;
  held_at                          = i;
  Player.possessions[O_UP_IN_AIR] = std::move(Player.pack[i]);
  Player.pack.erase(Player.pack.begin() + i);
  return O_UP_IN_AIR;
}

int getitem(chtype itype)
{
  auto fits = [itype](const object *o) {
    return o && (itype == NULL_ITEM || itype == CASH || o->objchar == itype || (itype == FOOD && o->objchar == CORPSE));
  };
  std::vector<std::string> names;
  std::vector<int> keys, refs;
  if(itype == CASH && Player.cash > 0)
  {
    names.push_back(std::format("{:<14} {} in cash", "", Player.cash));
    keys.push_back('$');
    refs.push_back(CASHVALUE);
  }
  for(int i = 1; i < MAXITEMS; ++i)
  {
    if(fits(Player.possessions[i].get()))
    {
      names.push_back(std::format("{:<14} {}", rl_slot_name(i), itemid(Player.possessions[i].get())));
      keys.push_back(index_to_key(i));
      refs.push_back(i);
    }
  }
  for(size_t i = Player.pack.size(); i-- > 0;)
  {
    if(fits(Player.pack[i].get()))
    {
      names.push_back(pack_line(i));
      keys.push_back(pack_key(i));
      refs.push_back(MAXITEMS + static_cast<int>(i));
    }
  }
  if(names.empty())
  {
    queue_message("Nothing appropriate.");
    return ABORT;
  }
  int sel = rl_choose_keys("Select an item (*: takes a while to find in the pack)", names, keys);
  if(sel < 0)
  {
    return ABORT;
  }
  if(refs[sel] >= MAXITEMS)
  {
    return hold_pack_item(refs[sel] - MAXITEMS);
  }
  return refs[sel];
}

bool merge_item_with_pack(const object *o)
{
  if(!o || o->objchar == STICK)
  {
    return false;
  }
  for(std::unique_ptr<object> &item : Player.pack)
  {
    if(item && objequal(o, item.get()))
    {
      item->number += o->number;
      return true;
    }
  }
  return false;
}

bool merge_item_with_inventory(const object *o)
{
  if(!o || o->objchar == STICK)
  {
    return false;
  }
  const std::array slots{O_LEFT_SHOULDER, O_RIGHT_SHOULDER, O_BELT1, O_BELT2, O_BELT3};
  for(int slot : slots)
  {
    object *pack_item = Player.possessions[slot].get();
    if(pack_item && objequal(o, pack_item))
    {
      pack_item->number += o->number;
      return true;
    }
  }
  return false;
}

// inserts the item at the end of the pack array
void push_pack(std::unique_ptr<object> o)
{
  Player.pack.emplace_back(std::move(o));
}

void add_to_pack(std::unique_ptr<object> o)
{
  if(merge_item_with_pack(o.get()))
  {
    return;
  }
  if(Player.pack.size() >= MAXPACK)
  {
    queue_message("Your pack is full. The item drops to the ground.");
    drop_at(Player.x, Player.y, std::move(o));
  }
  else
  {
    push_pack(std::move(o));
    queue_message("Putting item in pack.");
  }
}

void get_to_pack(std::unique_ptr<object> o)
{
  if(merge_item_with_pack(o.get()))
  {
    return;
  }
  if(Player.pack.size() >= MAXPACK)
  {
    queue_message("Your pack is full.");
    p_drop_at(Player.x, Player.y, o->number, o.get());
  }
  else
  {
    queue_message("Putting item in pack.");
    push_pack(std::move(o));
  }
}

void gain_item(std::unique_ptr<object> o)
{
  if(o->uniqueness == UNIQUE_MADE)
  {
    Objects[o->id].uniqueness = UNIQUE_TAKEN;
  }
  if(o->objchar == CASH)
  {
    queue_message("You gained some cash.");
    Player.cash += o->basevalue;
    dataprint();
  }
  else if(merge_item_with_inventory(o.get()))
  {
    queue_message("You add it to the stack in your inventory");
  }
  else
  {
    get_to_pack(std::move(o));
  }
  calc_melee();
}

int pack_item_cost(size_t index)
{
  index = Player.pack.size() - 1 - index;
  if(index > 20)
  {
    return 17;
  }
  else if(index > 15)
  {
    return 7;
  }
  else
  {
    return 2;
  }
}

bool is_two_handed(const object *o)
{
  if(o && twohandedp(o->id))
  {
    return true;
  }
  else
  {
    return false;
  }
}

// whether or not an item o can be used in a slot. Assumes o can in fact be placed in the slot.
bool item_useable(const object *o, int slot)
{
  if(slot == O_ARMOR || slot == O_CLOAK || slot == O_SHIELD || slot == O_BOOTS || slot >= O_RING1)
  {
    return true;
  }
  else if(o->objchar == WEAPON || o->objchar == MISSILEWEAPON)
  {
    if(is_two_handed(o) && (slot == O_WEAPON_HAND || slot == O_READY_HAND))
    {
      if(is_two_handed(Player.possessions[O_WEAPON_HAND].get()) && !Player.possessions[O_READY_HAND])
      {
        queue_message("You heft the weapon and find you must use both hands.");
        return true;
      }
      else
      {
        queue_message("This weapon is two-handed, so at the moment, ");
        queue_message("you are just lugging it around....");
        return false;
      }
    }
    else
    {
      return slot == O_WEAPON_HAND;
    }
  }
  else if((slot == O_READY_HAND || slot == O_WEAPON_HAND) && o->id == 8)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// prevents people from wielding 3 short swords, etc.
void pack_extra_items(object *item)
{
  auto extra = std::make_unique<object>(*item);
  extra->number = item->number - 1;
  extra->used   = false;
  item->number  = 1;
  if(Player.pack.size() < MAXPACK)
  {
    queue_message("Putting extra items back in pack.");
    push_pack(std::move(extra));
  }
  else
  {
    queue_message("No room for extra copies of item -- dropping them.");
    drop_at(Player.x, Player.y, std::move(extra));
  }
  calc_melee();
}

// WDT -- 'response' must be an index into the pack
void use_pack_item(size_t response, int slot)
{
  int duration = pack_item_cost(response);
  if(duration > 10)
  {
    queue_message("You begin to rummage through your pack.");
  }
  if(duration > 5)
  {
    queue_message("You search your pack for the item.");
  }
  queue_message("You take the item from your pack.");
  Command_Duration += duration;

  Player.possessions[slot] = std::move(Player.pack[response]);
  Player.pack.erase(Player.pack.begin() + response);

  if(item_useable(Player.possessions[slot].get(), slot))
  {
    item_equip(Player.possessions[slot]);
    if(Player.possessions[slot]->number > 1)
    {
      pack_extra_items(Player.possessions[slot].get());
    }
  }
}

// WDT HACK!  This ought to be in scr.c, along with its companion.  However,
// right now it's only used in the function directly below.
// takes something from pack, puts to slot
void take_from_pack(int slot)
{
  if(Player.pack.empty())
  {
    queue_message("Pack is empty!");
  }
  else
  {
    // port: the pack items that fit this slot, as a choice menu (the pack's letters)
    std::vector<std::string> items;
    std::vector<int> keys;
    std::vector<size_t> index;
    for(size_t i = Player.pack.size(); i-- > 0;)
    {
      if(aux_slottable(Player.pack[i].get(), slot))
      {
        std::string depth = pack_item_cost(i) > 10 ? "** " : pack_item_cost(i) > 5 ? "*  " : "   ";
        items.push_back(depth + itemid(Player.pack[i].get()));
        keys.push_back('a' + static_cast<int>(Player.pack.size() - 1 - i));
        index.push_back(i);
      }
    }
    if(items.empty())
    {
      queue_message("You see nothing useful for that slot in the pack.");
    }
    else
    {
      for(int sel = 0;;)
      {
        sel = rl_choose_keys("Take which item from your pack?\n(*: takes some time to reach; **: buried very deeply)", items, keys, sel);
        if(sel < 0)
        {
          break;
        }
        if(slottable(Player.pack[index[sel]].get(), slot))
        {
          use_pack_item(index[sel], slot);
          break;
        }
      }
    }
  }
}

// port: take a slot's item back to the pack (W); false if it is cursed
static bool unequip_slot(int slot)
{
  std::unique_ptr<object> &o = Player.possessions[slot];
  if(!o)
  {
    return true;
  }
  if(o->blessing < 0 && o->used)
  {
    queue_message("Item is cursed!");
    return false;
  }
  queue_message(std::format("You take off {}.", itemid(o.get())));
  conform_unused_object(o);
  add_to_pack(std::move(o));
  Command_Duration += 2;
  calc_melee();
  return true;
}

// port: put pack item i into the slot its type takes (w): armour, shield,
// boots, cloak to theirs, a ring to the first free finger, a weapon to the
// weapon hand, anything else to the first free hand, belt or shoulder
static void equip_pack_item(size_t i)
{
  const object *o = Player.pack[i].get();
  std::vector<int> slots;
  switch(o->objchar)
  {
    case ARMOR:
      slots = {O_ARMOR};
      break;
    case SHIELD:
      slots = {O_SHIELD};
      break;
    case BOOTS:
      slots = {O_BOOTS};
      break;
    case CLOAK:
      slots = {O_CLOAK};
      break;
    case RING:
      slots = {O_RING1, O_RING2, O_RING3, O_RING4};
      break;
    case WEAPON:
    case MISSILEWEAPON:
      slots = {O_WEAPON_HAND};
      break;
    default:
      slots = {O_READY_HAND, O_BELT1, O_BELT2, O_BELT3, O_LEFT_SHOULDER, O_RIGHT_SHOULDER};
      break;
  }
  int slot = slots[0];
  for(int s : slots)
  {
    if(!Player.possessions[s])
    {
      slot = s;
      break;
    }
  }
  // make room: the slot's item (and for a two-handed weapon the ready hand) go to the pack
  std::unique_ptr<object> &old = Player.possessions[slot];
  if(old && old->blessing < 0 && old->used)
  {
    queue_message("Item is cursed!");
    return;
  }
  if(is_two_handed(o) && !unequip_slot(O_READY_HAND))
  {
    return;
  }
  std::unique_ptr<object> keep;
  if(old)
  {
    conform_unused_object(old);
    keep = std::move(old);
  }
  use_pack_item(i, slot);
  Command_Duration += 5;
  if(keep)
  {
    queue_message(std::format("You put {} in your pack.", itemid(keep.get())));
    add_to_pack(std::move(keep));
  }
  calc_melee();
}

void equip_item()
{
  std::vector<std::string> names;
  std::vector<int> keys;
  std::vector<size_t> index;
  for(size_t i = Player.pack.size(); i-- > 0;)
  {
    names.push_back(pack_line(i));
    keys.push_back(pack_key(i));
    index.push_back(i);
  }
  if(names.empty())
  {
    queue_message("Your pack is empty.");
  }
  int sel = names.empty() ? -1 : rl_choose_keys("Equip which item?", names, keys);
  if(sel < 0)
  {
    setgamestatus(SKIP_MONSTERS, GameStatus);
    return;
  }
  equip_pack_item(index[sel]);
}

void unequip_item()
{
  std::vector<std::string> names;
  std::vector<int> keys, slots;
  for(int i = 1; i < MAXITEMS; ++i)
  {
    if(Player.possessions[i])
    {
      names.push_back(std::format("{:<14} {}", rl_slot_name(i), itemid(Player.possessions[i].get())));
      keys.push_back(index_to_key(i));
      slots.push_back(i);
    }
  }
  if(names.empty())
  {
    queue_message("You have nothing equipped.");
  }
  int sel = names.empty() ? -1 : rl_choose_keys("Take off which item?", names, keys);
  if(sel < 0 || !unequip_slot(slots[sel]))
  {
    setgamestatus(SKIP_MONSTERS, GameStatus);
  }
}

// port: one list, the slots and then the pack; a key or Enter opens the item
// menu (rl_item_menu). Its use commands (quaff, drop ...) leave the list, run,
// and reopen it (Rl_reopen).
void do_inventory_control()
{
  static int sel = 0;
  for(;;)
  {
    std::vector<std::string> names;
    std::vector<int> keys, refs;
    for(int i = 1; i < MAXITEMS; ++i)
    {
      object *o = Player.possessions[i].get();
      names.push_back(std::format("{:<14} {}", rl_slot_name(i), o ? itemid(o) : ""));
      keys.push_back(index_to_key(i));
      refs.push_back(i);
    }
    for(size_t i = Player.pack.size(); i-- > 0;)
    {
      names.push_back(pack_line(i));
      keys.push_back(pack_key(i));
      refs.push_back(MAXITEMS + static_cast<int>(i));
    }
    sel = rl_choose_keys(std::format("Inventory (pack {}/{}): a key or Enter for actions", Player.pack.size(), MAXPACK),
                         names, keys, sel);
    if(sel < 0)
    {
      sel = 0;
      break;
    }
    int ref = refs[sel];
    int act = rl_item_menu(ref);
    if(act == 0)
    {
      break; // a game command was queued
    }
    if(act == 'w')
    {
      equip_pack_item(ref - MAXITEMS);
    }
    else if(act == 'W')
    {
      unequip_slot(ref);
    }
    else if(act == 'p')
    {
      if(ref == O_READY_HAND && is_two_handed(Player.possessions[O_WEAPON_HAND].get()))
      {
        unequip_slot(O_WEAPON_HAND);
      }
      else
      {
        take_from_pack(ref);
        Command_Duration += 5;
      }
    }
    calc_melee();
  }
  xredraw();
}

// splits num off of item to make newitem which is returned
// something else (dispose_lost_objects) has to reduce the actual
// number value of item and Player.itemweight
std::unique_ptr<object> split_item(int num, const object *item)
{
  if(item)
  {
    auto o = std::make_unique<object>(*item);
    if(num <= item->number)
    {
      o->number = num;
    }
    o->used = false;
    return o;
  }
  else
  {
    return nullptr;
  }
}

// criteria for being able to put some item in some slot
bool slottable(const object *o, int slot)
{
  bool ok = true;
  if(!o)
  {
    ok = false;
  }
  else if(slot == O_ARMOR)
  {
    if(o->objchar != ARMOR)
    {
      queue_message("Only armor can go in the armor slot!");
      ok = false;
    }
  }
  else if(slot == O_SHIELD)
  {
    if(o->objchar != SHIELD)
    {
      queue_message("Only a shield can go in the shield slot!");
      ok = false;
    }
  }
  else if(slot == O_BOOTS)
  {
    if(o->objchar != BOOTS)
    {
      queue_message("Only boots can go in the boots slot!");
      ok = false;
    }
  }
  else if(slot == O_CLOAK)
  {
    if(o->objchar != CLOAK)
    {
      queue_message("Only a cloak can go in the cloak slot!");
      ok = false;
    }
  }
  else if(slot >= O_RING1)
  {
    if(o->objchar != RING)
    {
      queue_message("Only a ring can go in a ring slot!");
      ok = false;
    }
  }
  return ok;
}

bool cursed(const object *obj)
{
  if(!obj)
  {
    return false;
  }
  else
  {
    return obj->blessing < 0;
  }
}

// returns true if item with id and charge is found in pack or in
// inventory slot. charge is used to differentiate
// corpses instead of aux, which is their food value.
bool find_item(object *&o, int id, int chargeval)
{
  o = nullptr;
  for(std::unique_ptr<object> &item : Player.possessions)
  {
    if(item)
    {
      if(item->id == id && (chargeval == -1 || item->charge == chargeval))
      {
        o = item.get();
        return true;
      }
    }
  }
  for(std::unique_ptr<object> &item : Player.pack)
  {
    if(item)
    {
      if(item->id == id && (chargeval == -1 || item->charge == chargeval))
      {
        o = item.get();
        return true;
      }
    }
  }
  return false;
}

// returns true if item with id and charge is found in pack or in
// inventory slot. Destroys item. charge is used to differentiate
// corpses instead of aux, which is their food value.
bool find_and_remove_item(int id, int chargeval)
{
  for(int i = 1; i < MAXITEMS; ++i)
  {
    std::unique_ptr<object> &item = Player.possessions[i];
    if(item)
    {
      if(item->id == id && (chargeval == -1 || item->charge == chargeval))
      {
        dispose_lost_objects(1, i);
        return true;
      }
    }
  }
  for(auto it = Player.pack.begin(); it != Player.pack.end(); ++it)
  {
    std::unique_ptr<object> &item = *it;
    if(item)
    {
      if(item->id == id && (chargeval == -1 || item->charge == chargeval))
      {
        if(--item->number == 0)
        {
          Player.pack.erase(it);
        }
        return true;
      }
    }
  }
  return false;
}

void lose_all_items()
{
  queue_message("You notice that you are completely devoid of all possessions.");
  for(int i = 0; i < MAXITEMS; ++i)
  {
    if(Player.possessions[i])
    {
      dispose_lost_objects(Player.possessions[i]->number, i);
    }
  }
  Player.pack.clear();
  calc_melee();
}
