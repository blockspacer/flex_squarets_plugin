#include <string>
#include <iostream>
#include <vector>

#define GEN_CAT(a, b) GEN_CAT_I(a, b)
#define GEN_CAT_I(a, b) GEN_CAT_II(~, a ## b)
#define GEN_CAT_II(p, res) res

#define GEN_UNIQUE_NAME(base) GEN_CAT(base, __COUNTER__)

#define $typeclass(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
  __attribute__((annotate("{gen};{funccall};typeclass" ))) \
  GEN_UNIQUE_NAME(__gen_tmp__typeclass) \
  : __VA_ARGS__ \
  {};

#define $typeclass_impl(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__typeclass_impl) \
    ;

#define $typeclass_combo(...) \
  /* generate definition required to use __attribute__ */ \
  struct \
    __attribute__((annotate("{gen};{funccall};" #__VA_ARGS__))) \
    GEN_UNIQUE_NAME(__gen_tmp__typeclass_combo) \
    ;

struct FireSpell {
  std::string title = "FireSpell";
  std::string description = "FireSpell";
};

// like impl for trait
struct WaterSpell {
  std::string title = "WaterSpell";
  std::string description = "WaterSpell";
};

// like `trait`
struct Printable {
    virtual void print() const noexcept = 0;
};

// like `trait`
//template<typename S, typename A>
struct Spell {
  virtual void cast(const char* spellname, const int spellpower,
                    const char* target) const noexcept = 0;

  virtual void has_spell(const char* spellname) const noexcept = 0;

  virtual void add_spell(const char* spellname) const noexcept = 0;

  virtual void remove_spell(const char* spellname) const noexcept = 0;

  virtual void set_spell_power(const char* spellname,
                               const int spellpower) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};

struct
MagicItem {
  virtual void has_enough_mana(const char* spellname) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};

template<typename T1>
struct
ParentTemplated_1 {
  virtual void has_P1(T1 name1) const noexcept = 0;
};

template<typename T1>
struct
ParentTemplated_2 {
  virtual void has_P2(T1 name1) const noexcept = 0;
};

template<typename T1, typename T2>
struct
MagicTemplated {
  virtual void has_T(const T1& name1, const T2& name2) const noexcept = 0;

  /// \note same for all types
  // @gen(inject_to_all)
  //S interface_data;
};

// like `trait`
$typeclass(public MagicItem)

// like `trait`
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass(
    public MagicTemplated<std::string, int>
    , public ParentTemplated_1<const char *>
    , public ParentTemplated_2<const int &>)

// like `trait`
$typeclass(public Printable)


// like `trait`
$typeclass(public Spell)


// like impl for trait
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "FireSpell", "Spell", "MagicItem")
)

// like impl for trait
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "FireSpell", "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass_impl(
  typeclass_instance(
    target = "FireSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)


// like impl for trait
/// \note example of combined typeclasses, see `typeclass_combo`
$typeclass_impl(
  typeclass_instance(target = "WaterSpell",
    "Spell", "MagicItem");
  typeclass_instance(target = "WaterSpell",
    "Printable")
)

// like impl for trait
/// \note example of merged typeclasses
/// \note in most cases prefer combined typeclasses to merged
$typeclass_impl(
  typeclass_instance(
    target = "WaterSpell",
    "MagicTemplated<std::string, int>,"
    "ParentTemplated_1<const char *>,"
    "ParentTemplated_2<const int &>")
)

// just wraps multiple `traits`, forwards calls
/// \note example of combined typeclasses
$typeclass_combo(
  typeclass_combo(Spell, MagicItem)
)

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/FireSpell_MagicItem.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void has_enough_mana<MagicItem, FireSpell>
    (const FireSpell& data, const char* spellname) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) has_enough_mana " << " by "
      << data.title << " " << spellname << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/WaterSpell_MagicItem.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void has_enough_mana<MagicItem, WaterSpell>
    (const WaterSpell& data, const char* spellname) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) has_enough_mana " << " by "
      << data.title << " " << spellname << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass.generated.hpp"

#include "generated/FireSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void has_T<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const std::string &name1, const int &name2) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(Fire) has_T on " << name1
              << " by " << name2 << " "
              << std::endl;
}

template<>
void has_P1<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const char *name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(FireSpell) has_P1 on " << name1
              << std::endl;
}

template<>
void has_P2<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , FireSpell >
(const FireSpell& data, const int& name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(FireSpell) has_P2 on " << name1
              << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass.generated.hpp"

#include "generated/WaterSpell_MagicTemplated_std__string__int__ParentTemplated_1_const_char____ParentTemplated_2_const_int___.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void has_T<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const std::string &name1, const int &name2) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_T on " << name1
              << " by " << name2 << " "
              << std::endl;
}

template<>
void has_P1<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const char *name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_P1 on " << name1
              << std::endl;
}

template<>
void has_P2<
  MagicTemplated<std::string, int>,
  ParentTemplated_1<const char *>,
  ParentTemplated_2<const int &>
  , WaterSpell >
(const WaterSpell& data, const int& name1) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(WaterSpell) has_P2 on " << name1
              << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Printable.typeclass.generated.hpp"
#include "generated/FireSpell_Printable.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void print<Printable, FireSpell>
    (const FireSpell& data) noexcept {
    std::cout << "(lib1) print for FireSpell "
      << data.title << " " << data.description << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Printable.typeclass.generated.hpp"
#include "generated/WaterSpell_Printable.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void print<Printable, WaterSpell>
    (const WaterSpell& data) noexcept {
    std::cout << "(lib1) print for WaterSpell "
      << data.title << " " << data.description << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/FireSpell_Spell.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void cast<Spell, FireSpell>
    (const FireSpell& data, const char* spellname, const int spellpower,
     const char* target) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) cast on " << target << " by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

template<>
void has_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) has_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void add_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) add_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void remove_spell<Spell, FireSpell>(const FireSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) remove_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void set_spell_power<Spell, FireSpell>
    (const FireSpell& data, const char *spellname, const int spellpower) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) set_spell_power by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

} // namespace cxxctp
} // namespace generated

#include "generated/Spell.typeclass.generated.hpp"
#include "generated/WaterSpell_Spell.typeclass_instance.generated.hpp"

namespace cxxctp {
namespace generated {

template<>
void cast<Spell, WaterSpell>
    (const WaterSpell& data, const char* spellname, const int spellpower,
     const char* target) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) cast on " << target << " by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

template<>
void has_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) has_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void add_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) add_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void remove_spell<Spell, WaterSpell>(const WaterSpell& data, const char *spellname ) noexcept {
    std::cout << "(lib1) remove_spell by " << data.title << " " << spellname
              << " with " << spellname << std::endl;
}

template<>
void set_spell_power<Spell, WaterSpell>
    (const WaterSpell& data, const char *spellname, const int spellpower) noexcept {
    /// \note don`t use get_concrete<type> here, it may be get_concrete<ref_type>
    std::cout << "(lib1) set_spell_power by " << data.title << " " << spellname
              << " with " << spellpower << std::endl;
}

} // namespace cxxctp
} // namespace generated

int main(int argc, char** argv)
{
  using namespace cxxctp::generated;

  // TODO: better example https://blog.rust-lang.org/2015/05/11/traits.html

  _tc_combined_t<Spell> myspell{FireSpell{"title1", "description1"}};

  myspell.set_spell_power(/*spellname*/ "spellname1", /*spellpower*/ 3);
  myspell.cast(/*spellname*/ "spellname1", /*spellpower*/ 3, /*target*/ "target1");

  _tc_combined_t<Spell> myspellcopy = myspell;

  //_tc_impl_t<FireSpell, Spell> someFireSpell{FireSpell{"title1", "description1"}};

  //_tc_combined_t<Spell> someFireSpellmove = std::move(someFireSpell);

  //FireSpell& data = myspell.ref_model()->ref_concrete<FireSpell>();
  //FireSpell a = data;

  /*_tc_impl_t<FireSpell, Spell> myFireSpell{FireSpell{"title1", "description1"}};
  _tc_combined_t<Spell> myFireSpellref = std::ref(myFireSpell);*/

  _tc_combined_t<Spell> someFireSpell{FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}};

  std::vector<_tc_combined_t<Spell>> spells;
  spells.push_back(myspell);
  spells.push_back(someFireSpell);

  for(const _tc_combined_t<Spell>& it : spells) {
    it.cast("", 1, "");
#if defined(ENABLE_TYPECLASS_GUID)
    std::cout << "spells: get_GUID "
      << it.get_GUID() << std::endl;
#endif // ENABLE_TYPECLASS_GUID
  }

  std::vector<_tc_combined_t<MagicItem>> magicItems;
  magicItems.push_back(FireSpell{"FireSpelltitle1", "description1"});
  magicItems.push_back(WaterSpell{"WaterSpelltitle1", "description1"});

  for(const _tc_combined_t<MagicItem>& it : magicItems) {
    it.has_enough_mana("");
  }

  std::vector<_tc_combined_t<Spell, MagicItem>> spellMagicItems;
  {
    _tc_combined_t<Spell, MagicItem> pushed{};
    pushed = magicItems.at(0); // copy
    spellMagicItems.push_back(std::move(pushed));
  }
  {
    _tc_combined_t<Spell, MagicItem> pushed{};
    _tc_combined_t<Spell> someTmpSpell{
      FireSpell{"someTmpSpell", "someTmpSpell"}};
    pushed = std::move(someTmpSpell); // move
    spellMagicItems.push_back(std::move(pushed));
  }
  //spellMagicItems.push_back(someFireSpell.raw_model());
  //spellMagicItems.push_back(
  //  someFireSpell.ref_model()); // shared data
  //spellMagicItems.push_back(someFireSpell.clone_model());

  for(const _tc_combined_t<Spell, MagicItem>& it : spellMagicItems) {
    if(it.has_model<Spell>()) {
      it.cast("", 1, "");
    }
    if(it.has_model<MagicItem>()) {
      it.has_enough_mana("");
    }
  }

  /*_tc_combined_t<Spell, MagicItem> combined1 {
      _tc_combined_t<Spell>{FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}}
  };*/

  _tc_combined_t<Spell, MagicItem> combined1 {
      FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  //combined1 = WaterSpell{"WaterSpell", "WaterSpell"};

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined1.has_model<Spell>()) {
    combined1.add_spell("");
  }

  combined1 = magicItems.at(0);

  if(combined1.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined1.has_model<Spell>()) {
    combined1.add_spell("");
  }

  /*combined1 = _tc_combined_t<Spell>{
    FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };*/

  _tc_combined_t<Spell, MagicItem> combined2 {
      FireSpell{"someFireSpellTitle", "someFireSpelldescription1"}
  };

  std::cout << "combined2: can_convert to MagicItem: "
    << combined2.can_convert<Spell>() << std::endl;

  std::cout << "combined2: can_convert to MagicItem: "
    << combined2.can_convert<MagicItem>() << std::endl;

  std::cout << "combined2: can_convert to int: "
    << combined2.can_convert<int>() << std::endl;

  if(combined2.has_model<MagicItem>()) {
    combined1.has_enough_mana("");
  }

  if(combined2.has_model<Spell>()) {
    combined1.add_spell("");
  }

  std::vector<_tc_combined_t<Printable>> printables;
  printables.push_back(FireSpell{"someFireSpellTitle", "someFireSpelldescription1"});
  printables.push_back(WaterSpell{"WaterSpell", "WaterSpell"});

  for(const _tc_combined_t<Printable>& it : printables) {
    it.print();
  }

  std::vector<_tc_combined_t<
    MagicTemplated<std::string, int>,ParentTemplated_1<const char *>,ParentTemplated_2<const int &>
  >> tpls;
  tpls.push_back({
      WaterSpell{"WaterSpell", "WaterSpell"}
  });
  tpls.push_back({
      FireSpell{"FireSpell", "FireSpell"}
  });

  int idx = 0;
  for(const _tc_combined_t<
    MagicTemplated<std::string, int>,ParentTemplated_1<const char *>,ParentTemplated_2<const int &>
    >& it : tpls) {
    it.has_T("name1", idx++);
    it.has_P1("name~");
    it.has_P2(idx);
#if defined(ENABLE_TYPECLASS_GUID)
    std::cout << "tpls: get_GUID "
      << it.get_GUID() << std::endl;
#endif // ENABLE_TYPECLASS_GUID
  }

  /// \note Uses std::reference_wrapper
  FireSpell fs{"FireSpellRef", "FireSpellRef!"};
  _tc_combined_t<Spell, MagicItem> combinedRef1 {
      std::ref(fs)
  };

  _tc_combined_t<Spell, MagicItem> combinedRef2;
  combinedRef2.create_model<Spell>
    (std::ref(fs));

  fs.title = "NewSharedFireSpellRefTitle0";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }

  /// \note Uses std::shared_ptr
  combinedRef2.ref_model<Spell>()
    = combinedRef1.ref_model<Spell>();

  fs.title = "NewSharedFireSpellRefTitle1";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }

  /// \note Uses std::unique_ptr, data copyed!
  combinedRef2 = combinedRef1;

  fs.title = "New__NOT_SHARED__FireSpellRefTitle!!!";
  if(combinedRef1.has_model<Spell>()) {
    combinedRef1.cast("", 0, "");
  }
  if(combinedRef2.has_model<Spell>()) {
    combinedRef2.cast("", 0, "");
  }

  return 0;
}
