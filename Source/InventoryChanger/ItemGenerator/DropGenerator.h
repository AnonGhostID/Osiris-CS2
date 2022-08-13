#pragma once

#include "../../Memory.h"
#include <SDK/WeaponId.h>

#include <InventoryChanger/GameItems/Lookup.h>
#include <InventoryChanger/Inventory/Item.h>
#include <InventoryChanger/Inventory/Structs.h>

#include <SDK/ItemSchema.h>

namespace inventory_changer::item_generator
{

[[nodiscard]] inline std::uint8_t getNumberOfSupportedStickerSlots(WeaponId weaponID) noexcept
{
    if (const auto def = memory->itemSystem()->getItemSchema()->getItemDefinitionInterface(weaponID))
        return static_cast<std::uint8_t>(std::clamp(def->getNumberOfSupportedStickerSlots(), 0, 5));
    return 0;
}

template <typename AttributeGenerator>
class DropGenerator {
public:
    explicit DropGenerator(const game_items::Lookup& gameItemLookup, AttributeGenerator attributeGenerator)
        : gameItemLookup{ gameItemLookup }, attributeGenerator{ attributeGenerator } {}

    [[nodiscard]] inventory::ItemData generateItemData(const game_items::Item& unlockedItem, const inventory::Item& caseItem, bool willProduceStatTrak) const
    {
        if (willProduceStatTrak && unlockedItem.isMusic()) {
            return inventory::Music{ .statTrak = 0 };
        } else if (unlockedItem.isSkin()) {
            return generateSkin(unlockedItem, caseItem);
        } else if (unlockedItem.isGloves()) {
            return generateGloves(unlockedItem);
        }
        return {};
    }

private:
    [[nodiscard]] inventory::Skin generateSkin(const game_items::Item& unlockedItem, const inventory::Item& caseItem) const
    {
        inventory::Skin skin;
        const auto& paintKit = gameItemLookup.getStorage().getPaintKit(unlockedItem);
        skin.wear = std::lerp(paintKit.wearRemapMin, paintKit.wearRemapMax, attributeGenerator.generatePaintKitWear(attributeGenerator.generatePaintKitCondition()));
        skin.seed = attributeGenerator.generatePaintKitSeed();

        if (const auto souvenirPackage = caseItem.get<inventory::SouvenirPackage>()) {
            skin.tournamentID = gameItemLookup.getStorage().getTournamentEventID(caseItem.gameItem());
            skin.tournamentStage = souvenirPackage->tournamentStage;
            skin.tournamentTeam1 = souvenirPackage->tournamentTeam1;
            skin.tournamentTeam2 = souvenirPackage->tournamentTeam2;
            skin.proPlayer = souvenirPackage->proPlayer;
            skin.stickers = generateSouvenirStickers(unlockedItem.getWeaponID(), gameItemLookup.getStorage().getTournamentEventID(caseItem.gameItem()), gameItemLookup.getStorage().getTournamentMap(caseItem.gameItem()), skin.tournamentTeam1, skin.tournamentTeam2, skin.proPlayer);
        } else if (attributeGenerator.generateStatTrak()) {
            skin.statTrak = 0;
        }

        return skin;
    }

    [[nodiscard]] inventory::SkinStickers generateSouvenirStickers(WeaponId weaponID, std::uint32_t tournamentID, TournamentMap map, TournamentTeam team1, TournamentTeam team2, csgo::ProPlayer player) const
    {
        inventory::SkinStickers stickers;

        stickers[0].stickerID = gameItemLookup.findTournamentEventStickerID(static_cast<csgo::Tournament>(tournamentID));

        if (tournamentID != 1) {
            stickers[1].stickerID = gameItemLookup.findTournamentTeamGoldStickerID(static_cast<csgo::Tournament>(tournamentID), team1);
            stickers[2].stickerID = gameItemLookup.findTournamentTeamGoldStickerID(static_cast<csgo::Tournament>(tournamentID), team2);

            if (player)
                stickers[3].stickerID = gameItemLookup.findTournamentPlayerGoldStickerID(static_cast<csgo::Tournament>(tournamentID), static_cast<int>(player));
            else if (tournamentID >= 18) // starting with PGL Stockholm 2021
                stickers[3].stickerID = game_integration::getTournamentMapGoldStickerID(map);
        }

        attributeGenerator.shuffleStickers(getNumberOfSupportedStickerSlots(weaponID), stickers);
        return stickers;
    }

    [[nodiscard]] inventory::Gloves generateGloves(const game_items::Item& unlockedItem) const
    {
        const auto& paintKit = gameItemLookup.getStorage().getPaintKit(unlockedItem);

        return inventory::Gloves{
            .wear = std::lerp(paintKit.wearRemapMin, paintKit.wearRemapMax, attributeGenerator.generatePaintKitWear(attributeGenerator.generatePaintKitCondition())),
            .seed = attributeGenerator.generatePaintKitSeed()
        };
    }

    const game_items::Lookup& gameItemLookup;
    AttributeGenerator attributeGenerator;
};

}
