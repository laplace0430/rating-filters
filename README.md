# Rating Filters

<img src="logo.png" width="150" alt="Rating Filters logo" />

A Windows Geode mod that adds exact rating-tier filters to Geometry Dash's
online level search.

## Features

- Filter rated levels by **Unfeatured**, **Featured**, **Epic**,
  **Legendary**, and **Mythic**.
- Select any combination of tiers. Every checkbox is independent and the
  results use union semantics.
- Distinguish regular Featured levels from Epic, Legendary, and Mythic levels.
- Fill custom result pages with up to 10 unique levels while keeping adjacent
  pages on separate server-page ranges.
- Support rated classic and platformer levels.

## Requirements

- Geometry Dash 2.2081 for Windows
- Geode 5.8.2 or newer within the same major version
- Node IDs 1.23.3 or newer within the same major version

## Manual installation

Copy `jeond.rating-filters.geode` into the Geometry Dash `geode/mods` folder,
then restart the game.

## Build

```powershell
$env:GEODE_SDK = 'path\to\GeodeSDK'
geode build
```
