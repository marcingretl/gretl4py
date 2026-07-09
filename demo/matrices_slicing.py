import gretl

# 3 x 4 matrix with deterministic values
m = gretl.Matrix([
    [11, 12, 13, 14],
    [21, 22, 23, 24],
    [31, 32, 33, 34]
])

print("Matrix:")
print(m)

print("\n=== Scalar indexing ===")
for idx in [
    (0, 0),
    (2, 3),
    (-1, -1),
]:
    print(f"\nm{idx}")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Row indexing ===")
for idx in [
    (0, slice(None)),
    (1, slice(None)),
    (-1, slice(None)),
]:
    print(f"\nm{idx}")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Column indexing ===")
for idx in [
    (slice(None), 0),
    (slice(None), 2),
    (slice(None), -1),
]:
    print(f"\nm{idx}")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Row slices ===")
row_tests = [
    slice(None),
    slice(0, 3),
    slice(0, 2),
    slice(1, 3),
    slice(1, 2),
    slice(2, 3),
    slice(0, -1),
    slice(1, -1),
    slice(-2, None),
    slice(-1, None),
    slice(None, -1),
    slice(None, -2),
    slice(None, None, 1),
    slice(None, None, 2),
    slice(None, None, -1),
    slice(None, None, -2),
]

for s in row_tests:
    print(f"\nm[{s}, :]")
    try:
        print(m[s, :])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Column slices ===")
for s in row_tests:
    print(f"\nm[:, {s}]")
    try:
        print(m[:, s])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== 2D slices ===")
cases = [
    (slice(None), slice(None)),
    (slice(0, 2), slice(0, 2)),
    (slice(1, 3), slice(2, 4)),
    (slice(None, -1), slice(None, -1)),
    (slice(-2, None), slice(-3, None)),
    (slice(None, None, 1), slice(None, None, 1)),
    (slice(0, None), slice(1, None)),
    (slice(None, -1), slice(1, -1)),
    (slice(-2, -1), slice(-3, -1)),
    (slice(None, None, 2), slice(None, None, 2)),
    (slice(None, None, -1), slice(None, None, -1)),
]

for rs, cs in cases:
    print(f"\nm[{rs}, {cs}]")
    try:
        print(m[rs, cs])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== One-dimensional shorthand ===")
tests = [
    slice(None),
    slice(0, 2),
    slice(1, 3),
    slice(None, -1),
    slice(-2, None),
    slice(None, None, 1),
    slice(None, None, 2),
    slice(None, None, -1),
]

for s in tests:
    print(f"\nm[{s}]")
    try:
        print(m[s])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Whole-matrix shorthand ===")
cases = [
    slice(None),
    slice(None, None, 1),
    (slice(None), slice(None, None, 1)),
    (slice(None, None, 1), slice(None)),
    (slice(None, None, 1), slice(None, None, 1)),
]

for idx in cases:
    print(f"\nm[{idx}]")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Out-of-range indices ===")
for idx in [
    (3, 0),
    (0, 4),
    (-4, 0),
    (0, -5),
]:
    print(f"\nm{idx}")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)

print("\n=== Invalid types ===")
for idx in [
    ("a", 0),
    (0, "b"),
    (1.5, 0),
    (0, 2.5),
]:
    print(f"\nm{idx}")
    try:
        print(m[idx])
    except Exception as e:
        print(type(e).__name__, e)
