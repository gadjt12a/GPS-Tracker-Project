"""
Reconstruct .ninja_log for VALTRACK-V4-ESP32-C3 build.

The build log can be overwritten by failed build attempts. Without log entries,
ninja considers all ~1100 framework .obj files dirty and tries to recompile them.
This fails on mbedtls/ssl.h (a pre-existing framework bug on full recompile).

Fix: populate .ninja_log with entries for all existing .obj files using:
  - MurmurHash64A(command_string, seed=0xDECAFBADDECAFBAD) for command_hash
  - Windows FILETIME - 126227704000000000 for mtime

GCC compilation rules have no ninja rspfile, so 'ninja -t commands' output
is exactly the string that gets hashed.

Seed confirmed from ninja 1.12.1 binary offset 48262: MOV RDX, 0xDECAFBADDECAFBAD
"""

import subprocess
import struct
import ctypes
import os
import re
import sys

NINJA = r"C:\Kris\Espressif\tools\ninja\1.12.1\ninja.exe"
BUILD = r"C:\Kris\Projects\Valtrack_v4_Project\VALTRACK-V4-ESP32-C3\build"
FILETIME_OFFSET = 126227704000000000  # log_mtime = FILETIME - FILETIME_OFFSET

GCC = r"C:\Users\Kris.Pawson\.platformio\packages\toolchain-riscv32-esp\bin\riscv32-esp-elf-gcc.exe"
ASM_GCC = GCC  # ASM also uses gcc


def murmurhash64a(data, seed=0xDECAFBADDECAFBAD):
    """MurmurHash2 64-bit (MurmurHash64A) — used by ninja 1.x build log."""
    m = 0xc6a4a7935bd1e995
    r = 47
    mask = 0xFFFFFFFFFFFFFFFF

    if isinstance(data, str):
        data = data.encode('utf-8')

    length = len(data)
    h = (seed ^ (length * m)) & mask

    # Process 8-byte blocks (little-endian)
    num_blocks = length // 8
    for i in range(num_blocks):
        off = i * 8
        k = struct.unpack_from('<Q', data, off)[0]
        k = (k * m) & mask
        k ^= k >> r
        k = (k * m) & mask
        h ^= k
        h = (h * m) & mask

    # Process remaining bytes (fall-through switch in C, data2[0] is lowest addr)
    remaining = length % 8
    if remaining > 0:
        tail = data[num_blocks * 8:]
        k = 0
        if remaining >= 7: k ^= tail[6] << 48
        if remaining >= 6: k ^= tail[5] << 40
        if remaining >= 5: k ^= tail[4] << 32
        if remaining >= 4: k ^= tail[3] << 24
        if remaining >= 3: k ^= tail[2] << 16
        if remaining >= 2: k ^= tail[1] << 8
        k ^= tail[0]
        h ^= k & mask
        h = (h * m) & mask

    h ^= h >> r
    h = (h * m) & mask
    h ^= h >> r

    return h


def get_filetime(path):
    """Get Windows FILETIME (last-write) for path. Returns None on error."""
    kernel32 = ctypes.windll.kernel32
    FILE_SHARE_ALL = 0x00000007
    OPEN_EXISTING = 3
    FILE_FLAG_BACKUP_SEMANTICS = 0x02000000
    GENERIC_READ = 0x80000000
    INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

    h = kernel32.CreateFileW(
        path, GENERIC_READ, FILE_SHARE_ALL, None,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, None
    )
    if h == INVALID_HANDLE_VALUE or h == -1:
        return None
    ft = ctypes.c_uint64(0)
    ok = kernel32.GetFileTime(h, None, None, ctypes.byref(ft))
    kernel32.CloseHandle(h)
    return ft.value if ok else None


# ---------------------------------------------------------------------------
# Step 1: Verify hash algorithm against known log entry
# ---------------------------------------------------------------------------
KNOWN_CMD = (
    r'C:\windows\system32\cmd.exe /C "cd /D C:\Kris\Projects\Valtrack_v4_Project'
    r'\VALTRACK-V4-ESP32-C3\build && C:\Users\Kris.Pawson\.platformio\packages'
    r'\tool-cmake\bin\cmake.exe -E touch C:/Kris/Projects/Valtrack_v4_Project'
    r'/VALTRACK-V4-ESP32-C3/build/project_elf_src_esp32c3.c"'
)
KNOWN_HASH = 0xd0a559e484b2a92f

# Get exact command bytes from ninja (not from a Python literal, avoiding any quoting issues)
import subprocess as _sp
_r = _sp.run([NINJA, '-C', BUILD, '-t', 'commands', 'project_elf_src_esp32c3.c'], capture_output=True)
_lines = _r.stdout.split(b'\n')
KNOWN_CMD_BYTES = b''
for _l in _lines:
    _l = _l.rstrip(b'\r\n')
    if _l:
        KNOWN_CMD_BYTES = _l
        break

computed = murmurhash64a(KNOWN_CMD_BYTES)
print(f"Hash verify: expected={KNOWN_HASH:016x}, got={computed:016x}, match={computed == KNOWN_HASH}")
if computed != KNOWN_HASH:
    print("ERROR: Hash mismatch -- algorithm or command string is wrong. Aborting.")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Step 2: Get all commands from ninja (covers the default target = the elf)
# ---------------------------------------------------------------------------
print(f"\nRunning 'ninja -t commands' to get all build commands...")
result = subprocess.run(
    [NINJA, '-C', BUILD, '-t', 'commands'],
    capture_output=True  # raw bytes -- preserve exactly what ninja outputs
)
if result.returncode != 0:
    print(f"ninja -t commands failed (rc={result.returncode}):\n{result.stderr[:500]}")
    sys.exit(1)

# Split on newlines, keep as bytes
all_commands = [l.rstrip(b'\r\n') for l in result.stdout.split(b'\n') if l.rstrip(b'\r\n')]
print(f"Got {len(all_commands)} commands total")

# ---------------------------------------------------------------------------
# Step 3: Parse GCC compilation commands -> {abs_output_path: command_string}
# ---------------------------------------------------------------------------
# GCC command pattern: ends with -o RELPATH.obj -c SOURCE.c
# The -o flag appears exactly twice in a compile command:
#   -MT out.obj (for deps) and -o out.obj -c src.c
# We want the last -o (before -c). Match on bytes.
OBJ_CMD_PATTERN = re.compile(
    rb'\s-o\s+(\S+\.obj)\s+-c\s+\S'
)

target_commands = {}  # abs_path -> command_bytes
skipped = 0

for cmd in all_commands:
    # Only process GCC/G++ compilation commands (produces .obj)
    m = OBJ_CMD_PATTERN.search(cmd)
    if not m:
        skipped += 1
        continue

    rel_out = m.group(1).decode('utf-8', errors='replace')
    # Normalize to absolute Windows path
    if not os.path.isabs(rel_out):
        abs_out = os.path.normpath(os.path.join(BUILD, rel_out))
    else:
        abs_out = os.path.normpath(rel_out)

    target_commands[abs_out] = cmd  # bytes

print(f"Found {len(target_commands)} .obj compilation commands ({skipped} non-obj commands skipped)")

# ---------------------------------------------------------------------------
# Step 4: Walk build dir for existing .obj files, compute entries
# ---------------------------------------------------------------------------
log_entries = []  # list of (rel_path, abs_path, cmd_hash, log_mtime)
missing = []
no_cmd = []

for dirpath, dirnames, filenames in os.walk(BUILD):
    # Skip the CMakeFiles/rules.ninja dir and similar non-output dirs
    dirnames[:] = [d for d in dirnames if d not in {'.cmake', 'Testing'}]
    for fname in filenames:
        if not fname.endswith('.obj'):
            continue
        abs_path = os.path.join(dirpath, fname)
        if abs_path not in target_commands:
            no_cmd.append(abs_path)
            continue
        cmd = target_commands[abs_path]
        ft = get_filetime(abs_path)
        if ft is None:
            missing.append(abs_path)
            continue
        log_mtime = ft - FILETIME_OFFSET
        cmd_hash = murmurhash64a(cmd)
        rel_path = os.path.relpath(abs_path, BUILD).replace('\\', '/')
        abs_fwd = abs_path.replace('\\', '/')
        log_entries.append((rel_path, abs_fwd, cmd_hash, log_mtime))

print(f"\nResults:")
print(f"  .obj files with log entries to write: {len(log_entries)}")
print(f"  .obj files with no matching command:  {len(no_cmd)}")
print(f"  .obj files unreadable/missing:        {len(missing)}")

if no_cmd:
    print(f"\n  First few with no command:")
    for p in no_cmd[:5]:
        print(f"    {p}")

# ---------------------------------------------------------------------------
# Step 5: Write new .ninja_log
# ---------------------------------------------------------------------------
log_path = os.path.join(BUILD, '.ninja_log')
backup_path = log_path + '.bak'

# Back up existing log
if os.path.exists(log_path):
    import shutil
    shutil.copy2(log_path, backup_path)
    print(f"\nBacked up existing .ninja_log -> .ninja_log.bak")

with open(log_path, 'w', newline='\n') as f:
    f.write('# ninja log v6\n')
    for rel_path, abs_fwd, cmd_hash, log_mtime in log_entries:
        # Ninja stores both relative and absolute path entries
        f.write(f'0\t1\t{log_mtime}\t{rel_path}\t{cmd_hash:016x}\n')
        f.write(f'0\t1\t{log_mtime}\t{abs_fwd}\t{cmd_hash:016x}\n')

total_lines = len(log_entries) * 2
print(f"Wrote {total_lines} entries ({len(log_entries)} .obj files x 2) to {log_path}")
print(f"\nDone. Next: run ninja to build (should recompile only main.c + link, skip framework .obj files).")
