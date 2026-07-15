#include "CustomRateFilter.hpp"

#include <Geode/Bindings.hpp>

namespace rating_filters {

bool isRatedLevel(GJGameLevel* level) {
    // In GD 2.2081 the reward is stored in m_stars for both classic
    // levels (stars) and platformer levels (moons).
    return level && static_cast<int>(level->m_stars) > 0;
}

} // namespace rating_filters
