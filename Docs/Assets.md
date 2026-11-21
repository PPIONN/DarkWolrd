# 에셋 출처·라이선스 목록

> 기준: 설치본 v0.2.0 (2026-11-13). 설치본에 들어가는 에셋은 빌드 컨테이너 목록에서 확인했습니다(UnrealPak `-ListContainer`). 새 에셋은 프로젝트에 넣을 때 이 표에 먼저 적습니다.

## 설치본에 들어가는 에셋

| 분류 | 에셋 | 위치 | 출처 | 라이선스·조건 | 출처 표기 |
| --- | --- | --- | --- | --- | --- |
| 캐릭터 | 토끼 메시 | `Content/Rabbit/Basic` | Meshy AI로 직접 생성 (유료 플랜) | 유료 플랜 생성물은 상업적 사용 가능 | 필요 없음 |
| 애니메이션 | 토끼 애니메이션 14종 | `Content/Rabbit/ain` | Mixamo (Adobe) | 게임에 포함해 배포 가능. 애니메이션 파일만 따로 재배포 금지 | 필요 없음 |
| 몬스터 | 박쥐 메시·리그·애니메이션 | `Content/Alice/Bat` | 리그와 애니메이션은 Blender에서 직접 제작 | 직접 제작 | 필요 없음 |
| 몬스터 | 슬라임 메시 | `Content/Alice/Slime` | Meshy AI로 직접 생성 (유료 플랜) | 유료 플랜 생성물은 상업적 사용 가능 | 필요 없음 |
| 지형 재질 | Forest Ground 01, Aerial Rocks 02 텍스처 | `Content/Art/Terrain` | [Poly Haven](https://polyhaven.com/a/forrest_ground_01) · [Aerial Rocks 02](https://polyhaven.com/a/aerial_rocks_02), 제작 Rob Tuytel | CC0 (퍼블릭 도메인) | 필요 없음 (감사 표기만) |
| 지형 재질 | M_TutorialGround (경사로 풀·바위 섞기) | `Content/Art/Terrain` | 직접 제작 | 직접 제작 | 필요 없음 |
| 효과음 | 효과음 11종 | `Content/Audio/SFX` | [make_sfx.py](../Scripts/make_sfx.py)로 직접 합성 | 직접 제작, 외부 음원 없음 | 필요 없음 |
| 이펙트 | 충격파 이펙트 (Easy Shockwaves VFX) | `Content/Vefects` | Vefects, Fab 마켓플레이스 | Fab 라이선스. 게임 안에 넣어 배포하는 조건은 확인 중. 원본 파일 재배포 금지라 저장소에는 넣지 않음 | 확인 중 |
| 마네킹 애니메이션 | Mannequins 애니메이션·리타깃 | `Content/Characters/Mannequins` | Epic Games (Third Person 템플릿) | 언리얼 엔진 EULA. 언리얼로 만든 게임에 포함 가능 | 필요 없음 |
| 기본 재질·소품 | StarterContent 일부 (재질 2, 텍스처 1, 소품 1, 블루프린트 1, 건축 1) | `Content/StarterContent` | Epic Games | 언리얼 엔진 EULA | 필요 없음 |
| 마법 구슬, 입력, UI | BP_MagicOrb, BP_CurseOrb, 입력 매핑, WBP_HUD, 코드로 그린 화면 | `Content/Magic`, `Content/Input`, `Content/UI`, `Source/` | 직접 제작 | 직접 제작 | 필요 없음 |
| 폰트 | Roboto, Droid Sans Fallback (한글 표시) | 엔진 기본 폰트 | Google, 언리얼 엔진에 포함 | Apache License 2.0 | 필요 없음 |
| 엔진 | Unreal Engine 5.8 | | Epic Games | 언리얼 엔진 EULA (매출 기준 로열티 조건) | 출시 때 엔진 로고 규정 확인 |

## 저장소에만 있고 설치본에는 없는 에셋

| 에셋 | 위치 | 출처 | 비고 |
| --- | --- | --- | --- |
| Haru (Dapper Bunny) | `Content/StarterContent/Haru` | Meshy AI로 직접 생성 (유료 플랜) | 예전 시험용 모델 |
| 해골 몬스터 (Mossy Little Bones) 메시·리그·애니메이션 7종 | `Content/Alice/Bones` | 메시는 Meshy AI로 직접 생성 (유료 플랜), 리그·애니메이션은 Blender에서 직접 제작 (2026-11-21) | 아직 맵에 배치 전이라 설치본에는 없음. 배치하면 위 표로 옮김 |
| 늑대 몬스터 (Fierce Timberwolf) 메시·리그·애니메이션 7종 | `Content/Alice/Wolf` | 메시는 Meshy AI로 직접 생성 (유료 플랜). 단순화하고 텍스처를 Blender에서 다시 구움. 리그·애니메이션은 Blender에서 직접 제작 (2026-11-21) | 아직 맵에 배치 전이라 설치본에는 없음. 배치하면 위 표로 옮김 |
| NPC 문포 (Moonpaw) 메시·리그·애니메이션 3종 | `Content/NPC/Moonpaw` | 메시는 Meshy AI로 직접 생성 (유료 플랜). 단순화하고 텍스처를 Blender에서 다시 구움. 리그·애니메이션은 Blender에서 직접 제작 (2026-11-21) | 아직 맵에 배치 전이라 설치본에는 없음. 배치하면 위 표로 옮김 |
| Mannequin_UE4, LevelPrototyping, ThirdPerson 맵 | `Content/Characters`, `Content/LevelPrototyping`, `Content/ThirdPerson` | Epic Games | 템플릿 기본 에셋 |

## AI 도구

| 도구 | 쓴 곳 | 비용 |
| --- | --- | --- |
| Meshy AI | 토끼, 슬라임, Haru 메시 생성 (2주차 이전) | 유료 플랜 |
| Claude Code | 코드 구현, 자동 점검, 헤드리스 에셋 작업 | 유료 구독 |

3주차의 아트 교체(지형)는 무료 경로로 했습니다. CC0 텍스처와 직접 만든 재질만 썼고 유료 AI 3D 툴은 쓰지 않았습니다.
