# DarkWolrd (가제)

**"쓰러뜨린 적이, 동료가 된다."**

쓰러뜨린 몬스터를 카드로 모아 최대 3마리와 함께 싸우는 3인칭 소환 액션 RPG입니다. 몽환적이면서 어두운 분위기의 맵에서, 토끼 마법사가 사냥으로 모은 소환수와 함께 혼자서는 못 이기는 보스를 쓰러뜨립니다.

LAUNCH:ER Unreal 3기 1인 프로젝트이며, PC(STOVE) 출시를 목표로 개발 중입니다.

## 코어 루프

```mermaid
flowchart LR
    A[몬스터 처치] --> B[카드 획득·등급 상승]
    B --> C[소환 슬롯 3칸 편성]
    C --> D[더 강한 사냥터·보스 도전]
    D --> A
```

필드 사냥 → 필드 보스 → 던전(방 3개, 마지막 방 보스)으로 이어지며, 데모 첫 클리어는 약 25분입니다.

## 개발 환경

| 항목 | 내용 |
| --- | --- |
| 엔진 | Unreal Engine 5.8 (C++) |
| 플랫폼 | Windows 64비트, 키보드+마우스 |
| IDE | Visual Studio 2022 |
| 기본 맵 | `/Game/Map/Alice` |

## 실행 방법

1. Unreal Engine 5.8을 설치합니다.
2. `DarkWolrd.uproject`를 우클릭해 **Generate Visual Studio project files**를 실행합니다.
3. 생성된 솔루션에서 `DarkWolrdEditor` / `Development Editor` / `Win64`로 빌드합니다.
4. `DarkWolrd.uproject`를 열고 Play를 누릅니다.

## 폴더 구조

| 경로 | 내용 |
| --- | --- |
| `Source/DarkWolrd` | 게임 C++ 코드 (Character, Combat, Summon, Monster, AI, Dungeon, Arena, SaveGame, UI 등) |
| `Content/Rabbit` | 플레이어(토끼) 메시·애니메이션·ABP |
| `Content/Alice` | 몬스터·소환수 에셋 |
| `Content/Map` | 레벨 |
| `Docs` | 기획서, 마일스톤, 주간 기록, TIL |

## 문서

- [1페이지 기획서](Docs/01_OnePagePlan.md)
- [8주 마일스톤](Docs/02_Milestones.md)
- [팀 구성](Docs/03_Team.md)
- [주간 기록](Docs/WeeklyLog/)
- [TIL](Docs/TIL/)

## 참고

- 마켓플레이스(Fab) 에셋은 라이선스상 저장소에 포함하지 않습니다.
- 정식 게임 제목은 스토어 자료 준비 단계(5주차)에 확정합니다.
