#include "SearchFilterState.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/modify/MoreSearchLayer.hpp>

using namespace geode::prelude;

namespace {

constexpr auto kFeaturedKey = "featured_filter";
constexpr auto kEpicKey = "epic_filter";
constexpr auto kLegendaryKey = "legendary_filter";
constexpr auto kMythicKey = "mythic_filter";

void setGameFilter(char const* key, bool value) {
    GameLevelManager::get()->setBoolForKey(value, key);
}

void clearGameRatingFilters() {
    setGameFilter(kFeaturedKey, false);
    setGameFilter(kEpicKey, false);
    setGameFilter(kLegendaryKey, false);
    setGameFilter(kMythicKey, false);
}

void persistRatingSelection(rating_filters::RatingSelection const& selection) {
    setGameFilter(kFeaturedKey, selection.featured);
    setGameFilter(kEpicKey, selection.epic);
    setGameFilter(kLegendaryKey, selection.legendary);
    setGameFilter(kMythicKey, selection.mythic);
}

CCMenuItemToggler* findToggle(CCNode* root, char const* id) {
    return root ? typeinfo_cast<CCMenuItemToggler*>(root->getChildByIDRecursive(id)) : nullptr;
}

// MoreSearchLayer intentionally constructs its stock togglers with the On
// sprite as the "off" item and the Off sprite as the "on" item. Their
// m_toggled value is therefore the inverse of what the player sees.
void setStockToggle(CCNode* root, char const* id, bool checked) {
    if (auto* toggle = findToggle(root, id)) {
        toggle->toggle(!checked);
    }
}

} // namespace

class $modify(RatingFilterSearchLayer, LevelSearchLayer) {
    bool init(int type) {
        rating_filters::resetRatingSelection();
        clearGameRatingFilters();
        return LevelSearchLayer::init(type);
    }

    void clearFilters() {
        LevelSearchLayer::clearFilters();
        rating_filters::resetRatingSelection();
        clearGameRatingFilters();
    }

    GJSearchObject* getSearchObject(SearchType type, gd::string query) {
        auto* object = LevelSearchLayer::getSearchObject(type, query);
        auto const selection = rating_filters::ratingSelection();

        if (object && object->isLevelSearchObject() && selection.managed) {
            // The visible rating toggles are authoritative. This avoids relying
            // on private saved-value keys and prevents programmatic UI changes
            // from desynchronizing the request.
            auto const hasCustomFilter = selection.unfeatured || selection.featured;
            if (hasCustomFilter) {
                // Request the broad rated feed and apply the exact union of
                // selected tiers locally. The server has no Unfeatured flag,
                // and its Featured flag also includes higher tiers.
                object->m_featuredFilter = false;
                object->m_epicFilter = false;
                object->m_legendaryFilter = false;
                object->m_mythicFilter = false;
                object->m_starFilter = true;
                object->m_noStarFilter = false;
                // An empty generic search is effectively the all-time popular
                // list, whose early pages contain virtually no Unfeatured
                // rated levels. Awarded is the server's chronological rating
                // feed and contains both featured and unfeatured awards.
                if (
                    object->m_searchType == SearchType::Search &&
                    object->m_searchQuery.empty()
                ) {
                    object->m_searchType = SearchType::Awarded;
                    object->m_page = 0;
                }
                rating_filters::markPendingSearch(object, selection);
            } else {
                object->m_featuredFilter = false;
                object->m_epicFilter = selection.epic;
                object->m_legendaryFilter = selection.legendary;
                object->m_mythicFilter = selection.mythic;
            }
            log::info(
                "Search object: ptr={}, type={}, selected=[U={}, F={}, E={}, L={}, M={}], request=[star={}, featured={}, epic={}, legendary={}, mythic={}]",
                reinterpret_cast<uintptr_t>(object),
                static_cast<int>(object->m_searchType),
                selection.unfeatured,
                selection.featured,
                selection.epic,
                selection.legendary,
                selection.mythic,
                object->m_starFilter,
                object->m_featuredFilter,
                object->m_epicFilter,
                object->m_legendaryFilter,
                object->m_mythicFilter
            );
        }
        return object;
    }
};

class $modify(RatingFilterMoreSearchLayer, MoreSearchLayer) {
    struct Fields {
        CCMenuItemToggler* unfeaturedToggle = nullptr;
        CCLabelBMFont* unfeaturedLabel = nullptr;
    };

    static void onModify(auto& self) {
        if (!self.setHookPriorityAfterPost("MoreSearchLayer::init", "geode.node-ids")) {
            log::warn("Unable to place MoreSearchLayer::init after geode.node-ids");
        }
    }

    bool init() {
        // The in-memory selection is the single source of truth. Old builds
        // could leave several saved filters enabled at once, so seed the
        // game's values before it constructs the stock toggles.
        auto selection = rating_filters::ratingSelection();
        if (!selection.managed) {
            rating_filters::setRatingSelection(false, false, false, false, false);
            selection = rating_filters::ratingSelection();
        }
        persistRatingSelection(selection);

        if (!MoreSearchLayer::init()) {
            return false;
        }

        auto* menu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByIDRecursive("main-menu"));
        auto* featured = findToggle(this, "featured-filter-toggler");
        auto* epic = findToggle(this, "epic-filter-toggler");
        auto* legendary = findToggle(this, "legendary-filter-toggler");
        auto* mythic = findToggle(this, "mythic-filter-toggler");
        auto* epicLabel = typeinfo_cast<CCLabelBMFont*>(
            this->getChildByIDRecursive("epic-filter-label")
        );
        auto* mythicLabel = typeinfo_cast<CCLabelBMFont*>(
            this->getChildByIDRecursive("mythic-filter-label")
        );

        if (!menu || !featured || !epic || !legendary || !mythic ||
            !epicLabel || !mythicLabel) {
            log::warn("Unable to find MoreSearchLayer rating controls; custom toggle was not added");
            return true;
        }

        auto* toggle = CCMenuItemToggler::createWithStandardSprites(
            this,
            menu_selector(RatingFilterMoreSearchLayer::onUnfeatured),
            .8f
        );
        if (!toggle) {
            log::warn("Unable to create Unfeatured toggle");
            return true;
        }
        // Row four has Legendary, Mythic, then an empty third column. These
        // are local coordinates because the toggle is added to the same menu.
        toggle->setPosition(epic->getPositionX(), mythic->getPositionY());
        toggle->toggle(selection.unfeatured);
        toggle->setID("unfeatured-filter-toggle");
        menu->addChild(toggle);
        m_fields->unfeaturedToggle = toggle;

        auto* label = CCLabelBMFont::create("Unfeatured", "bigFont.fnt");
        if (!label) {
            toggle->removeFromParent();
            m_fields->unfeaturedToggle = nullptr;
            log::warn("Unable to create Unfeatured label");
            return true;
        }
        label->setScale(epicLabel->getScale());
        label->setAnchorPoint(epicLabel->getAnchorPoint());
        auto const epicLeft = epicLabel->getPositionX() -
            epicLabel->getContentSize().width * epicLabel->getScaleX() *
            epicLabel->getAnchorPoint().x;
        auto const labelX = epicLeft +
            label->getContentSize().width * label->getScaleX() *
            label->getAnchorPoint().x;
        label->setPosition(labelX, mythicLabel->getPositionY());
        label->setID("unfeatured-filter-label");
        m_mainLayer->addChild(label);
        m_fields->unfeaturedLabel = label;

        // Explicitly mirror the canonical state after Node IDs has finished
        // naming the stock controls. Never import stale saved values here.
        selection = rating_filters::ratingSelection();
        setStockToggle(this, "featured-filter-toggler", selection.featured);
        setStockToggle(this, "epic-filter-toggler", selection.epic);
        setStockToggle(this, "legendary-filter-toggler", selection.legendary);
        setStockToggle(this, "mythic-filter-toggler", selection.mythic);
        toggle->toggle(selection.unfeatured);
        log::info(
            "Created Unfeatured UI: toggle=({}, {}), label=({}, {}), scale={}",
            toggle->getPositionX(),
            toggle->getPositionY(),
            label->getPositionX(),
            label->getPositionY(),
            toggle->getScale()
        );
        // Preserve the stock Featured node ID for compatibility with Node IDs
        // and other mods. It is the user-facing Featured-only switch.
        return true;
    }

    void onUnfeatured(CCObject* sender) {
        auto* toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
        if (!toggle) {
            return;
        }

        // CCMenuItemToggler callbacks run before the visual state flips.
        auto const enabled = !toggle->isToggled();
        auto const selection = rating_filters::ratingSelection();
        rating_filters::setRatingSelection(
            enabled,
            selection.featured,
            selection.epic,
            selection.legendary,
            selection.mythic
        );
        persistRatingSelection(rating_filters::ratingSelection());
        log::info("Unfeatured clicked: enabled={}", enabled);
    }

    void onFeatured(CCObject* sender) {
        auto* toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
        // Stock MoreSearch togglers render the inverse of m_toggled.
        auto const enabled = toggle && toggle->isToggled();

        auto const selection = rating_filters::ratingSelection();
        rating_filters::setRatingSelection(
            selection.unfeatured,
            enabled,
            selection.epic,
            selection.legendary,
            selection.mythic
        );
        persistRatingSelection(rating_filters::ratingSelection());
        log::info("Featured clicked: enabled={}", enabled);
    }

    void onEpic(CCObject* sender) {
        auto* toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
        auto const enabled = toggle && toggle->isToggled();
        auto const selection = rating_filters::ratingSelection();
        rating_filters::setRatingSelection(
            selection.unfeatured,
            selection.featured,
            enabled,
            selection.legendary,
            selection.mythic
        );
        persistRatingSelection(rating_filters::ratingSelection());
        log::info("Epic clicked: enabled={}", enabled);
    }

    void onLegendary(CCObject* sender) {
        auto* toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
        auto const enabled = toggle && toggle->isToggled();
        auto const selection = rating_filters::ratingSelection();
        rating_filters::setRatingSelection(
            selection.unfeatured,
            selection.featured,
            selection.epic,
            enabled,
            selection.mythic
        );
        persistRatingSelection(rating_filters::ratingSelection());
        log::info("Legendary clicked: enabled={}", enabled);
    }

    void onMythic(CCObject* sender) {
        auto* toggle = typeinfo_cast<CCMenuItemToggler*>(sender);
        auto const enabled = toggle && toggle->isToggled();
        auto const selection = rating_filters::ratingSelection();
        rating_filters::setRatingSelection(
            selection.unfeatured,
            selection.featured,
            selection.epic,
            selection.legendary,
            enabled
        );
        persistRatingSelection(rating_filters::ratingSelection());
        log::info("Mythic clicked: enabled={}", enabled);
    }
};
