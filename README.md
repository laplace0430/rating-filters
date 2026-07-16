# Rating Filter

<img src="logo.png" width="150" alt="Rating Filter logo" />

Geometry Dash 온라인 레벨 검색에 **Unfeatured** 필터와 정확한
**Featured** 필터를 추가하는 Windows Geode 모드입니다.

이 모드는 Geode Index에 올라간 모드가 아니므로, Geode 앱 검색으로 설치할
수 없습니다. 아래의 수동 설치 방법으로 설치해야 합니다.

## 다운로드

최신 `.geode` 파일은 GitHub Releases에서 받을 수 있습니다.

- [Rating Filter Releases](https://github.com/laplace0430/rating-filters/releases)

## 기능

- **Unfeatured** 필터를 추가합니다.
- **Featured** 필터가 Epic, Legendary, Mythic 레벨을 포함하지 않게 합니다.
- Unfeatured, Featured, Epic, Legendary, Mythic 체크박스가 서로 영향을 주지
  않고 독립적으로 작동합니다.
- 여러 등급을 동시에 선택하면 선택한 등급들이 함께 검색됩니다.
- rated classic 레벨과 platformer 레벨을 지원합니다.

## 요구 사항

- Windows용 Geometry Dash 2.2081
- Geode 5.8.2 이상
- Node IDs 1.23.3 이상

## 수동 설치

1. Releases 페이지에서 `laplace0430.rating-filter.geode` 파일을 다운로드합니다.
2. Geometry Dash 설치 폴더 안의 `geode/mods` 폴더를 엽니다.
3. 다운로드한 `.geode` 파일을 `geode/mods` 폴더에 넣습니다.
4. Geometry Dash를 다시 실행합니다.

Steam 기본 설치 경로는 보통 아래와 비슷합니다.

```text
C:\Program Files (x86)\Steam\steamapps\common\Geometry Dash\geode\mods
```

다른 드라이브에 설치했다면 Steam에서 Geometry Dash를 우클릭한 뒤
`관리 > 로컬 파일 보기`를 눌러 설치 폴더를 찾으면 됩니다.

## 빌드

```powershell
$env:GEODE_SDK = 'path\to\GeodeSDK'
geode build
```
