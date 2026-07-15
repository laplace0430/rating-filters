# Rating Filter

<img src="logo.png" width="150" alt="Rating Filter logo" />

A Windows Geode mod that adds Unfeatured and exact Featured filters to
Geometry Dash's online level search.

## Features

- Adds an **Unfeatured** filter and makes **Featured** exclude Epic,
  Legendary, and Mythic levels.
- Keeps Unfeatured, Featured, Epic, Legendary, and Mythic checkboxes fully
  independent. Multiple selections use union semantics.
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
