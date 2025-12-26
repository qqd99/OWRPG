// Copyright Legion. All Rights Reserved.

#include "System/OWRPGGameplayTags.h"

namespace OWRPGGameplayTags
{
	// STATUSES
	UE_DEFINE_GAMEPLAY_TAG(Status_Draining_Stamina, "OWRPG.Status.Draining.Stamina");
	UE_DEFINE_GAMEPLAY_TAG(Status_Draining_Mana, "OWRPG.Status.Draining.Mana");
	UE_DEFINE_GAMEPLAY_TAG(Status_Immunity_Draining, "OWRPG.Status.Immunity.Draining");

	// EVENTS
	UE_DEFINE_GAMEPLAY_TAG(Event_OWRPG_FellOutOfWorld, "OWRPG.Event.FellOutOfWorld");

	// INPUT
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Interact, "InputTag.OWRPG.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_ToggleInventory, "InputTag.OWRPG.ToggleInventory");

	// CHEATS
	UE_DEFINE_GAMEPLAY_TAG(Cheat_UnlimitedMana, "OWRPG.Cheat.UnlimitedMana");
	UE_DEFINE_GAMEPLAY_TAG(Cheat_UnlimitedStamina, "OWRPG.Cheat.UnlimitedStamina");
	UE_DEFINE_GAMEPLAY_TAG(Cheat_GodMode, "OWRPG.Cheat.GodMode");

	// --- ITEM SYSTEM ---

	// CATEGORIES
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Weapon, "OWRPG.Item.Category.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Armor, "OWRPG.Item.Category.Armor");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Consumable, "OWRPG.Item.Category.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Resource, "OWRPG.Item.Category.Resource");
	UE_DEFINE_GAMEPLAY_TAG(Item_Category_Tool, "OWRPG.Item.Category.Tool");

	// TRAITS - USAGE
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Equippable, "OWRPG.Item.Trait.Equippable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Consumable, "OWRPG.Item.Trait.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Throwable, "OWRPG.Item.Trait.Throwable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Burnable, "OWRPG.Item.Trait.Burnable");

	// TRAITS - MATERIAL
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Material_Metal, "OWRPG.Item.Trait.Material.Metal");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Material_Wood, "OWRPG.Item.Trait.Material.Wood");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Material_Stone, "OWRPG.Item.Trait.Material.Stone");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Material_Organic, "OWRPG.Item.Trait.Material.Organic");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Material_Magical, "OWRPG.Item.Trait.Material.Magical");

	// TRAITS - DAMAGE
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Damage_Blunt, "OWRPG.Item.Trait.Damage.Blunt");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Damage_Slash, "OWRPG.Item.Trait.Damage.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Item_Trait_Damage_Pierce, "OWRPG.Item.Trait.Damage.Pierce");

	// ATTRIBUTES
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Strength, "OWRPG.Attribute.Strength");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Agility, "OWRPG.Attribute.Agility");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Intelligence, "OWRPG.Attribute.Intelligence");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Endurance, "OWRPG.Attribute.Endurance");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Luck, "OWRPG.Attribute.Luck");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Willpower, "OWRPG.Attribute.Willpower");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_Defense, "OWRPG.Attribute.Defense");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_CriticalChance, "OWRPG.Attribute.CriticalChance");

	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxStamina, "OWRPG.Attribute.MaxStamina");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxMana, "OWRPG.Attribute.MaxMana");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxHunger, "OWRPG.Attribute.MaxHunger");
	UE_DEFINE_GAMEPLAY_TAG(Attribute_MaxThirst, "OWRPG.Attribute.MaxThirst");

	UE_DEFINE_GAMEPLAY_TAG(Race_Human, "OWRPG.Race.Human");
	UE_DEFINE_GAMEPLAY_TAG(Race_RockPerson, "OWRPG.Race.RockPerson");
	UE_DEFINE_GAMEPLAY_TAG(Race_Elf, "OWRPG.Race.Elf");

	// --- INVENTORY TAGS ---
	UE_DEFINE_GAMEPLAY_TAG(OWRPG_Inventory_Stack, "OWRPG.Inventory.Stack");
}