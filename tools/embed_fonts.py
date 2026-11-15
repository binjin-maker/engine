import sys, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
for src, name in (("DejaVuSans.ttf", "reg"), ("DejaVuSans-Bold.ttf", "bold")):
    d = open(os.path.join(root, "fonts", src), "rb").read()
    with open(os.path.join(root, f"font_{name}.h"), "w") as o:
        o.write(f"static const unsigned char {name}_ttf[]={{" + ",".join(map(str, d)) + "};\n")
        o.write(f"static const int {name}_ttf_len={len(d)};\n")
