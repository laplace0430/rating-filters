#pragma once

class GJSearchObject;

namespace rating_filters {

struct RatingSelection {
    bool managed = false;
    bool unfeatured = false;
    bool featured = false;
    bool epic = false;
    bool legendary = false;
    bool mythic = false;
};

void resetRatingSelection();

RatingSelection ratingSelection();
void setRatingSelection(
    bool unfeatured,
    bool featured,
    bool epic,
    bool legendary,
    bool mythic
);

void markPendingSearch(GJSearchObject* object, RatingSelection selection);
RatingSelection consumePendingSearch(GJSearchObject* object);

} // namespace rating_filters
