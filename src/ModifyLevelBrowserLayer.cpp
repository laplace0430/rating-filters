#include "CustomRateFilter.hpp"
#include "SearchFilterState.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;
using rating_filters::RatingSelection;

namespace {

bool hasCustomFilter(RatingSelection const& selection) {
    return selection.unfeatured || selection.featured;
}

std::string selectionName(RatingSelection const& selection) {
    std::string name;
    auto append = [&name](char const* tier) {
        if (!name.empty()) {
            name += "+";
        }
        name += tier;
    };
    if (selection.unfeatured) append("Unfeatured");
    if (selection.featured) append("Featured");
    if (selection.epic) append("Epic");
    if (selection.legendary) append("Legendary");
    if (selection.mythic) append("Mythic");
    if (name.empty()) {
        name = "None";
    }
    return name;
}

bool matchesSelection(GJGameLevel* level, RatingSelection const& selection) {
    if (!rating_filters::isRatedLevel(level)) {
        return false;
    }

    auto const epicRating = static_cast<int>(level->m_isEpic);
    if (epicRating == 1) return selection.epic;
    if (epicRating == 2) return selection.legendary;
    if (epicRating == 3) return selection.mythic;
    if (epicRating != 0) return false;
    if (level->m_featured > 0) return selection.featured;
    return selection.unfeatured;
}

CCArray* filterLevelArray(CCArray* results, RatingSelection const& selection, int page) {
    if (!results || !hasCustomFilter(selection)) {
        return results;
    }

    auto* filtered = CCArray::createWithCapacity(results->count());
    unsigned int unrated = 0;
    unsigned int unfeatured = 0;
    unsigned int featured = 0;
    unsigned int upperTier = 0;

    for (unsigned int index = 0; index < results->count(); ++index) {
        auto* level = typeinfo_cast<GJGameLevel*>(results->objectAtIndex(index));
        if (!level) {
            continue;
        }

        if (!rating_filters::isRatedLevel(level)) {
            ++unrated;
        } else if (level->m_isEpic > 0) {
            ++upperTier;
        } else if (level->m_featured > 0) {
            ++featured;
        } else {
            ++unfeatured;
        }

        if (matchesSelection(level, selection)) {
            filtered->addObject(level);
        }
    }

    log::info(
        "Server page {} for {}: total={}, unrated={}, unfeatured={}, featured={}, upper-tier={}, kept={}",
        page,
        selectionName(selection),
        results->count(),
        unrated,
        unfeatured,
        featured,
        upperTier,
        filtered->count()
    );
    return filtered;
}

} // namespace

class $modify(RatingFilterBrowserLayer, LevelBrowserLayer) {
    struct Fields {
        RatingSelection selection;
        Ref<CCArray> collected = CCArray::create();
        std::unordered_set<int> collectedIDs;
        std::unordered_map<std::string, int> requestPages;
        std::vector<Ref<GJSearchObject>> requestObjects;
        std::string primaryKey;
        int primaryType = 0;
        int nextServerPage = 0;
        int lastServerPage = 0;
        int pendingRequests = 0;
        int serverResponses = 0;
        bool gathering = false;
        bool batchFailed = false;
        int logicalPage = 0;
    };

    static constexpr unsigned int kTargetResults = 10;
    static constexpr int kBatchSize = 4;
    static constexpr int kMaxServerPages = 10;

    void resetCollection() {
        m_fields->collected = CCArray::create();
        m_fields->collectedIDs.clear();
        m_fields->requestPages.clear();
        m_fields->requestObjects.clear();
        m_fields->primaryKey.clear();
        m_fields->primaryType = 0;
        m_fields->nextServerPage = 0;
        m_fields->lastServerPage = 0;
        m_fields->pendingRequests = 0;
        m_fields->serverResponses = 0;
        m_fields->gathering = false;
        m_fields->batchFailed = false;
    }

    void addMatches(CCArray* filtered) {
        if (!filtered) {
            return;
        }
        for (unsigned int index = 0; index < filtered->count(); ++index) {
            auto* level = typeinfo_cast<GJGameLevel*>(filtered->objectAtIndex(index));
            if (!level) {
                continue;
            }
            auto const levelID = static_cast<int>(level->m_levelID);
            if (m_fields->collectedIDs.insert(levelID).second) {
                m_fields->collected->addObject(level);
            }
        }
    }

    void finishGathering() {
        auto* display = CCArray::createWithCapacity(kTargetResults);
        for (
            unsigned int index = 0;
            index < m_fields->collected->count() && index < kTargetResults;
            ++index
        ) {
            display->addObject(m_fields->collected->objectAtIndex(index));
        }

        log::info(
            "Finished {} page: {} result(s) from {} server response(s)",
            selectionName(m_fields->selection),
            display->count(),
            m_fields->serverResponses
        );
        m_fields->gathering = false;
        m_fields->requestPages.clear();
        m_fields->requestObjects.clear();
        LevelBrowserLayer::loadLevelsFinished(
            display,
            m_fields->primaryKey.c_str(),
            m_fields->primaryType
        );
        if (m_searchObject) {
            // Restore the user-facing logical page. Each logical page consumes
            // a disjoint block of server pages, so Next never repeats results.
            m_searchObject->m_page = m_fields->logicalPage;
            updatePageLabel();
        }
    }

    void launchBatch() {
        if (!m_searchObject) {
            finishGathering();
            return;
        }

        std::vector<Ref<GJSearchObject>> batch;
        for (
            int index = 0;
            index < kBatchSize && m_fields->nextServerPage <= m_fields->lastServerPage;
            ++index, ++m_fields->nextServerPage
        ) {
            auto* object = m_searchObject->getPageObject(m_fields->nextServerPage);
            if (!object) {
                continue;
            }
            auto const key = std::string(object->getKey());
            m_fields->requestPages.emplace(key, m_fields->nextServerPage);
            m_fields->requestObjects.emplace_back(object);
            batch.emplace_back(object);
            ++m_fields->pendingRequests;
        }

        if (batch.empty()) {
            finishGathering();
            return;
        }

        log::info(
            "Loading {} {} server pages in parallel (next={}, collected={})",
            batch.size(),
            selectionName(m_fields->selection),
            m_fields->nextServerPage,
            m_fields->collected->count()
        );
        for (auto const& object : batch) {
            GameLevelManager::get()->getOnlineLevels(object.data());
        }
    }

    void continueOrFinishBatch() {
        if (m_fields->pendingRequests > 0) {
            return;
        }
        if (
            m_fields->batchFailed ||
            m_fields->collected->count() >= kTargetResults ||
            m_fields->nextServerPage > m_fields->lastServerPage
        ) {
            finishGathering();
        } else {
            launchBatch();
        }
    }

    bool init(GJSearchObject* object) {
        // Set this before the original init so cached results loaded during init
        // pass through updateResultArray with the correct request-local state.
        m_fields->selection = rating_filters::consumePendingSearch(object);
        resetCollection();
        log::info(
            "Browser init: ptr={}, selection={}",
            reinterpret_cast<uintptr_t>(object),
            selectionName(m_fields->selection)
        );
        return LevelBrowserLayer::init(object);
    }

    void loadPage(GJSearchObject* object) {
        // A user-initiated Next, Previous, refresh, or page jump starts a new
        // logical custom page and invalidates any prior collection state.
        resetCollection();
        if (object && hasCustomFilter(m_fields->selection)) {
            m_fields->logicalPage = object->m_page;
            auto* physical = object->getPageObject(
                m_fields->logicalPage * kMaxServerPages
            );
            LevelBrowserLayer::loadPage(physical ? physical : object);
        } else {
            LevelBrowserLayer::loadPage(object);
        }
    }

    void loadLevelsFinished(CCArray* levels, char const* key, int type) {
        // Online results bypass updateResultArray in GD 2.2081 and are sent
        // straight to setupLevelBrowser. Filter the array at the delegate
        // boundary, before any LevelCell objects are created.
        auto const selection = m_fields->selection;
        auto const keyText = std::string(key ? key : "");
        auto requestIt = m_fields->requestPages.find(keyText);
        auto const isBatchResponse = requestIt != m_fields->requestPages.end();
        auto const page = isBatchResponse
            ? requestIt->second
            : (m_searchObject ? m_searchObject->m_page : -1);
        auto* filtered = filterLevelArray(
            levels,
            selection,
            page
        );

        if (!hasCustomFilter(selection)) {
            LevelBrowserLayer::loadLevelsFinished(filtered, key, type);
            return;
        }

        if (isBatchResponse) {
            addMatches(filtered);
            ++m_fields->serverResponses;
            m_fields->requestPages.erase(requestIt);
            --m_fields->pendingRequests;
            continueOrFinishBatch();
            return;
        }

        // The normal page response becomes the first part of a logical page.
        resetCollection();
        m_fields->gathering = true;
        m_fields->primaryKey = keyText;
        m_fields->primaryType = type;
        m_fields->serverResponses = 1;
        auto const startPage = m_searchObject ? m_searchObject->m_page : 0;
        m_fields->nextServerPage = startPage + 1;
        m_fields->lastServerPage = startPage + kMaxServerPages - 1;
        addMatches(filtered);

        if (
            m_fields->collected->count() >= kTargetResults ||
            !levels || levels->count() < 10
        ) {
            finishGathering();
        } else {
            launchBatch();
        }
    }

    void loadLevelsFailed(char const* key, int type) {
        auto const keyText = std::string(key ? key : "");
        auto requestIt = m_fields->requestPages.find(keyText);
        if (
            hasCustomFilter(m_fields->selection) &&
            requestIt != m_fields->requestPages.end()
        ) {
            log::warn("Rating-filter batch request failed for server page {}", requestIt->second);
            m_fields->requestPages.erase(requestIt);
            --m_fields->pendingRequests;
            m_fields->batchFailed = true;
            continueOrFinishBatch();
            return;
        }
        LevelBrowserLayer::loadLevelsFailed(key, type);
    }

    CCArray* updateResultArray(CCArray* results) {
        auto* baseResults = LevelBrowserLayer::updateResultArray(results);
        return filterLevelArray(
            baseResults,
            m_fields->selection,
            m_searchObject ? m_searchObject->m_page : -1
        );
    }
};
