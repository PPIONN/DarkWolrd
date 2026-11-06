"""게임 효과음을 코드로 합성해 WAV로 저장합니다 (외부 음원을 쓰지 않아 라이선스 문제가 없음).

사용법: python make_sfx.py <출력 폴더>
만든 WAV는 import_feedback_assets.py가 /Game/Audio/SFX로 가져옵니다.
"""
import math
import os
import random
import struct
import sys
import wave

RATE = 44100
random.seed(7)


def silence(duration):
    return [0.0] * int(RATE * duration)


def mix(base, layer, start=0.0, gain=1.0):
    offset = int(RATE * start)
    needed = offset + len(layer)
    if needed > len(base):
        base.extend([0.0] * (needed - len(base)))
    for i, sample in enumerate(layer):
        base[offset + i] += sample * gain
    return base


def tone(freq_start, freq_end, duration, attack=0.005, decay=None):
    """주파수가 freq_start에서 freq_end로 변하는 사인파. decay는 지수 감쇠의 시간 상수."""
    samples = []
    phase = 0.0
    count = int(RATE * duration)
    for i in range(count):
        t = i / RATE
        freq = freq_start + (freq_end - freq_start) * (i / count)
        phase += 2 * math.pi * freq / RATE
        envelope = min(1.0, t / attack) if attack > 0 else 1.0
        envelope *= math.exp(-t / decay) if decay else 1.0 - i / count
        samples.append(math.sin(phase) * envelope)
    return samples


def noise(duration, cutoff_start, cutoff_end, attack=0.002, decay=None):
    """저역 통과 필터를 지난 잡음. 차단 주파수를 움직여 휙 하는 소리를 냄."""
    samples = []
    previous = 0.0
    count = int(RATE * duration)
    for i in range(count):
        t = i / RATE
        cutoff = cutoff_start + (cutoff_end - cutoff_start) * (i / count)
        alpha = min(1.0, 2 * math.pi * cutoff / RATE)
        previous += alpha * (random.uniform(-1.0, 1.0) - previous)
        envelope = min(1.0, t / attack) if attack > 0 else 1.0
        envelope *= math.exp(-t / decay) if decay else 1.0 - i / count
        samples.append(previous * envelope)
    return samples


def bell(freq, duration, decay):
    layer = tone(freq, freq, duration, 0.002, decay)
    return mix(layer, tone(freq * 2.01, freq * 2.01, duration, 0.002, decay * 0.5), 0.0, 0.35)


def normalize(samples, peak):
    loudest = max(abs(s) for s in samples) or 1.0
    # 끝이 툭 끊기지 않도록 마지막 5ms를 줄임
    fade = int(RATE * 0.005)
    for i in range(1, fade + 1):
        samples[-i] *= (i - 1) / fade
    return [s / loudest * peak for s in samples]


def orb_launch():
    return mix(noise(0.16, 900, 4200, 0.01), tone(500, 1100, 0.16, 0.01), 0.0, 0.35)


def orb_impact():
    return mix(tone(190, 70, 0.16, 0.002, 0.05), noise(0.12, 3000, 500, 0.001, 0.03), 0.0, 0.5)


def hit():
    return mix(noise(0.1, 6000, 1500, 0.001, 0.025), tone(260, 120, 0.1, 0.001, 0.035), 0.0, 0.8)


def kill():
    sound = tone(620, 90, 0.2, 0.002, 0.07)
    sound = mix(sound, noise(0.14, 5000, 700, 0.001, 0.04), 0.0, 0.5)
    mix(sound, bell(880, 0.32, 0.09), 0.1, 0.5)
    return mix(sound, bell(1319, 0.3, 0.08), 0.15, 0.4)


def card_drop():
    return mix(bell(1047, 0.35, 0.1), bell(1568, 0.28, 0.08), 0.06, 0.7)


def card_pickup():
    sound = bell(784, 0.25, 0.07)
    mix(sound, bell(1047, 0.25, 0.07), 0.07)
    return mix(sound, bell(1319, 0.34, 0.1), 0.14)


def summon():
    duration = 0.85
    sound = silence(duration)
    for freq in (220.0, 277.2, 329.6, 440.0):
        layer = []
        phase = 0.0
        count = int(RATE * duration)
        for i in range(count):
            t = i / RATE
            phase += 2 * math.pi * freq * (1 + 0.006 * math.sin(2 * math.pi * 6 * t)) / RATE
            swell = min(1.0, t / 0.25) * (1.0 - max(0.0, t - 0.45) / 0.4)
            layer.append(math.sin(phase) * swell)
        mix(sound, layer)
    mix(sound, noise(0.5, 1200, 7000, 0.2), 0.0, 1.0)
    return mix(sound, bell(1760, 0.4, 0.13), 0.3, 1.6)


def despawn():
    return mix(tone(700, 200, 0.35, 0.01), noise(0.3, 3000, 400, 0.01), 0.0, 0.3)


def hurt():
    return mix(tone(150, 60, 0.2, 0.002, 0.07), noise(0.12, 2500, 400, 0.001, 0.04), 0.0, 0.6)


def land():
    return mix(tone(110, 60, 0.1, 0.002, 0.035), noise(0.07, 900, 200, 0.001, 0.02), 0.0, 0.4)


def guide_step():
    sound = bell(659, 0.3, 0.1)
    mix(sound, bell(880, 0.3, 0.1), 0.1)
    return mix(sound, bell(1175, 0.45, 0.14), 0.2)


SOUNDS = {
    "SFX_OrbLaunch": (orb_launch, 0.30),
    "SFX_OrbImpact": (orb_impact, 0.35),
    "SFX_Hit": (hit, 0.55),
    "SFX_Kill": (kill, 0.60),
    "SFX_CardDrop": (card_drop, 0.40),
    "SFX_CardPickup": (card_pickup, 0.50),
    "SFX_Summon": (summon, 0.60),
    "SFX_Despawn": (despawn, 0.35),
    "SFX_Hurt": (hurt, 0.60),
    "SFX_Land": (land, 0.30),
    "SFX_GuideStep": (guide_step, 0.40),
}


def main():
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    for name, (make, peak) in SOUNDS.items():
        samples = normalize(make(), peak)
        with wave.open(os.path.join(out_dir, name + ".wav"), "wb") as f:
            f.setnchannels(1)
            f.setsampwidth(2)
            f.setframerate(RATE)
            f.writeframes(b"".join(struct.pack("<h", int(s * 32767)) for s in samples))
        print(name, round(len(samples) / RATE, 2), "s")


if __name__ == "__main__":
    main()
