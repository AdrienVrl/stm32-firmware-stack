import numpy as np
import tensorflow as tf

mel_matrix = np.array(
    tf.signal.linear_to_mel_weight_matrix(
        num_mel_bins=40,
        num_spectrogram_bins=513,
        sample_rate=16000,
        lower_edge_hertz=20.0,
        upper_edge_hertz=4000.0,
    )
)
assert mel_matrix.shape == (513, 40)


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


with open("mel_matrix_data.h", "w") as f:
    f.write("#ifndef MEL_MATRIX_DATA_H\n#define MEL_MATRIX_DATA_H\n\n")
    f.write(format_c_array_2d("MEL_WEIGHT_MATRIX", mel_matrix))
    f.write("\n\n#endif\n")
