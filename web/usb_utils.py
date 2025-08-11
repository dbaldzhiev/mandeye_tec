import os
import re
import subprocess
from pathlib import Path
from typing import List, Optional

def find_usb_mount(mount_roots: Optional[List[str]] = None) -> Optional[Path]:
    """Return the first writable ``<root>/usbN`` mount point.

    The search strategy mirrors ``tecscanner``: probe ``mount`` and ``lsblk``
    before scanning the filesystem directly.  ``mount_roots`` defaults to the
    ``LIVOX_MOUNT_ROOTS`` environment variable or ``"/media:/run/media"``.
    """
    if mount_roots is None:
        roots_env = os.getenv("LIVOX_MOUNT_ROOTS", "/media:/run/media")
        mount_roots = [r for r in roots_env.split(os.pathsep) if r]
    prefixes = [f"{r.rstrip('/')}/usb" for r in mount_roots]

    # Inspect current mounts via ``mount``
    try:
        result = subprocess.run(["mount"], capture_output=True, text=True, check=False)
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3:
                mp = parts[2]
                if any(mp.startswith(p) for p in prefixes) and os.access(mp, os.W_OK):
                    return Path(mp)
    except OSError:
        pass

    # Fallback to ``lsblk`` output
    try:
        result = subprocess.run(
            ["lsblk", "-nr", "-o", "MOUNTPOINT"], capture_output=True, text=True, check=False
        )
        for mp in result.stdout.splitlines():
            if any(mp.startswith(p) for p in prefixes) and os.access(mp, os.W_OK):
                return Path(mp)
    except OSError:
        pass

    # Last resort: scan the filesystem
    for root in mount_roots:
        base = Path(root.rstrip("/"))
        try:
            candidates = [
                p for p in base.glob("usb*") if os.path.ismount(p) and os.access(p, os.W_OK)
            ]
        except OSError:
            continue
        if candidates:
            def sort_key(p: Path) -> int:
                m = re.search(r"usb(\d+)$", p.name)
                return int(m.group(1)) if m else float("inf")
            candidates.sort(key=sort_key)
            return candidates[0]

    return None
