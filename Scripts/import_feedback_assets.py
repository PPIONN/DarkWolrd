"""효과음을 가져오고 토끼·마법 구슬·박쥐의 사운드/이펙트 슬롯을 채웁니다.

에디터를 끈 상태에서 헤드리스로 실행합니다:
  UnrealEditor-Cmd.exe DarkWolrd.uproject -run=pythonscript -script="import_feedback_assets.py <WAV 폴더>"

이펙트는 로컬에만 있는 Fab 팩(Vefects)을 쓰므로, 저장소를 새로 받은 PC에서는 슬롯이 비어 보일 수 있습니다.
명상 키도 여기서 우클릭 → R로 옮깁니다 (우클릭이 조준과 겹쳐 조준할 때마다 토끼가 멈추던 문제).
"""
import os
import sys

import unreal

SFX_PATH = "/Game/Audio/SFX"
VFX_ROOT = "/Game/Vefects/Easy_Shockwaves_VFX/VFX/Shockwaves/Particles/Color_Variation"
assets = unreal.EditorAssetLibrary


def import_sounds(wav_dir):
    tasks = []
    for file_name in sorted(os.listdir(wav_dir)):
        if file_name.endswith(".wav"):
            task = unreal.AssetImportTask()
            task.filename = os.path.join(wav_dir, file_name)
            task.destination_path = SFX_PATH
            task.automated = True
            task.replace_existing = True
            task.save = True
            tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    if not tasks:
        raise RuntimeError("no wav files in " + wav_dir)


def sound(name):
    asset = unreal.load_asset(SFX_PATH + "/" + name)
    if not asset:
        raise RuntimeError("missing sound " + name)
    return asset


def vfx(name):
    asset = unreal.load_asset(VFX_ROOT + "/" + name)
    if not asset:
        raise RuntimeError("missing vfx " + name)
    return asset


def blueprint_cdo(path):
    return unreal.get_default_object(assets.load_blueprint_class(path))


def set_properties(target, values):
    for name, value in values.items():
        target.set_editor_property(name, value)


def make_key(name):
    key = unreal.Key()
    key.set_editor_property("key_name", name)
    return key


def main():
    import_sounds(sys.argv[1])

    rabbit = blueprint_cdo("/Game/Rabbit/MyRabbit")
    set_properties(rabbit, {"HurtSound": sound("SFX_Hurt"), "GuideStepSound": sound("SFX_GuideStep")})
    set_properties(rabbit.get_editor_property("CompanionComponent"), {
        "SummonSound": sound("SFX_Summon"),
        "SummonVFX": vfx("VFX_Shockwave_01_Teal_Big_1s"),
        "DespawnSound": sound("SFX_Despawn"),
        "DespawnVFX": vfx("VFX_Shockwave_01_Teal_Small_1s"),
        "CardPickupSound": sound("SFX_CardPickup"),
        "CardPickupVFX": vfx("VFX_Shockwave_01_Yellow_1s"),
    })
    set_properties(rabbit.get_editor_property("GameFeelComponent"), {"LandSound": sound("SFX_Land")})
    assets.save_asset("/Game/Rabbit/MyRabbit", only_if_is_dirty=False)

    set_properties(blueprint_cdo("/Game/Magic/BP_MagicOrb"), {
        "LaunchSound": sound("SFX_OrbLaunch"),
        "ImpactSound": sound("SFX_OrbImpact"),
        "ImpactVFX": vfx("VFX_Shockwave_01_Lavender_Small_1s"),
    })
    assets.save_asset("/Game/Magic/BP_MagicOrb", only_if_is_dirty=False)

    set_properties(blueprint_cdo("/Game/Alice/Bat/BP_Bat"), {
        "HitSound": sound("SFX_Hit"),
        "DeathSound": sound("SFX_Kill"),
        "DeathVFX": vfx("VFX_Shockwave_01_Purple_1s"),
        "CardDropSound": sound("SFX_CardDrop"),
    })
    assets.save_asset("/Game/Alice/Bat/BP_Bat", only_if_is_dirty=False)

    context = unreal.load_asset("/Game/Input/IMC_Default")
    meditate = unreal.load_asset("/Game/Input/IA_Meditate")
    context.unmap_key(meditate, make_key("RightMouseButton"))
    context.map_key(meditate, make_key("R"))
    assets.save_asset("/Game/Input/IMC_Default", only_if_is_dirty=False)


main()
