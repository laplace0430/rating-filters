#include "SearchFilterState.hpp"

namespace rating_filters {
namespace {

RatingSelection s_ratingSelection;
GJSearchObject* s_pendingObject = nullptr;
RatingSelection s_pendingSelection;

} // namespace

void resetRatingSelection() {
    s_ratingSelection = {};
    s_pendingObject = nullptr;
    s_pendingSelection = {};
}

RatingSelection ratingSelection() {
    return s_ratingSelection;
}

void setRatingSelection(
    bool unfeatured,
    bool featured,
    bool epic,
    bool legendary,
    bool mythic
) {
    s_ratingSelection = {
        .managed = true,
        .unfeatured = unfeatured,
        .featured = featured,
        .epic = epic,
        .legendary = legendary,
        .mythic = mythic,
    };
}

void markPendingSearch(GJSearchObject* object, RatingSelection selection) {
    s_pendingObject = object;
    s_pendingSelection = object ? selection : RatingSelection {};
}

RatingSelection consumePendingSearch(GJSearchObject* object) {
    if (!object || object != s_pendingObject) {
        return {};
    }

    auto const selection = s_pendingSelection;
    s_pendingObject = nullptr;
    s_pendingSelection = {};
    return selection;
}

} // namespace rating_filters
