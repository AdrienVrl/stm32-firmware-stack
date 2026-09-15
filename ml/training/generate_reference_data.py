import pathlib
import re

import numpy as np
import scipy.io.wavfile
import tensorflow as tf


def extract_mfcc(waveform):  # waveform: 16000 float32 samples in [-1, 1]
    stft = tf.signal.stft(
        waveform,
        frame_length=640,  # 40ms * 16kHz
        frame_step=320,  # 20ms * 16kHz
        fft_length=1024,
    )
    spectrogram = tf.abs(stft)

    mel_weight_matrix = tf.signal.linear_to_mel_weight_matrix(
        num_mel_bins=40,  # ARM's recipe computes 40 mel bins, then DCTs down to 10
        num_spectrogram_bins=stft.shape[-1],
        sample_rate=16000,
        lower_edge_hertz=20.0,
        upper_edge_hertz=4000.0,
    )
    mel_spectrogram = tf.matmul(spectrogram, mel_weight_matrix)
    log_mel = tf.math.log(mel_spectrogram + 1e-6)

    mfcc = tf.signal.mfccs_from_log_mel_spectrograms(log_mel)
    return mfcc[..., :10]  # keep first 10 cepstral coefficients


def format_c_array_1d(name, arr, dtype="int16_t", per_line=20):
    lines = [f"static const {dtype} {name}[{len(arr)}] = {{"]
    for i in range(0, len(arr), per_line):
        chunk = arr[i : i + per_line]
        vals = ", ".join(str(int(v)) for v in chunk)
        lines.append(f"    {vals},")
    lines.append("};")
    return "\n".join(lines)


def format_c_array_2d(name, arr):
    rows, cols = arr.shape
    lines = [f"static const float {name}[{rows}][{cols}] = {{"]
    for r in range(rows):
        vals = ", ".join(format_float(v) for v in arr[r])
        lines.append(f"    {{{vals}}},")
    lines.append("};")
    return "\n".join(lines)


def format_float(v):
    s = f"{v:.9g}"
    if "." not in s and "e" not in s and "E" not in s:
        s += ".0"
    return s + "f"


wav_path = "data/speech_commands_v0.02/yes/004ae714_nohash_0.wav"
samples, pcm_int16 = scipy.io.wavfile.read(wav_path)
pad = lambda a, i: a[0:i] if len(a) > i else a + [0] * (i - len(a))
pad(pcm_int16, 16000)
waveform = pcm_int16.astype(np.float32) / 32768.0
mfcc = extract_mfcc(waveform).numpy()


def parse_network_header_macro(header_path, macro_name):
    text = pathlib.Path(header_path).read_text()
    pattern = rf"#define\s+{macro_name}\s*\(\s*([-+0-9.eEfF]+)\s*\)"
    match = re.search(pattern, text)
    if not match:
        raise ValueError(f"Could not find macro {macro_name} in {header_path}")
    value_str = match.group(1).rstrip(
        "fF"
    )  # strip the C float suffix, e.g. "0.604...f" -> "0.604..."
    return float(value_str)


NETWORK_HEADER = "ml/include/network.h"

SCALE = parse_network_header_macro(NETWORK_HEADER, "STAI_NETWORK_IN_1_SCALE")
ZERO_POINT = int(
    parse_network_header_macro(NETWORK_HEADER, "STAI_NETWORK_IN_1_ZERO_POINT")
)

q = np.round(mfcc / SCALE) + ZERO_POINT
q = np.clip(q, -128, 127).astype(np.int8)

a = np.zeros((10, 40))
for i in range(10):
    for j in range(40):
        a[i, j] = np.cos(np.pi / 40 * (j + 0.5) * i)

with open("reference_quantized.h", "w") as f:
    f.write("#ifndef REFERENCE_QUANTIZED_H\n#define REFERENCE_QUANTIZED_H\n\n")
    f.write(format_c_array_2d("REFERENCE_QUANTIZED", mfcc))
    f.write("\n\n#endif\n")

with open("reference_pcm.h", "w") as f:
    f.write("#ifndef REFERENCE_PCM_H\n#define REFERENCE_PCM_H\n\n")
    f.write(format_c_array_1d("REFERENCE_PCM", pcm_int16))
    f.write("\n\n#endif\n")

with open("reference_mfcc.h", "w") as f:
    f.write("#ifndef REFERENCE_MFCC_H\n#define REFERENCE_MFCC_H\n\n")
    f.write(format_c_array_2d("REFERENCE_MFCC", q))
    f.write("\n\n#endif\n")

with open("cos_table.h", "w") as f:
    f.write("#ifndef COS_TABLEH\n#define COS_TABLE_H\n\n")
    f.write(format_c_array_2d("COS_TABLE", a))
    f.write("\n\n#endif\n")
