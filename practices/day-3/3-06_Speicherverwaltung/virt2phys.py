import os
import struct
import sys

pid = int(sys.argv[1])
vaddr = int(sys.argv[2], 16)

page_size = os.sysconf("SC_PAGE_SIZE")
page_index = vaddr // page_size
page_offset = vaddr % page_size

with open(f"/proc/{pid}/pagemap", "rb") as f:
    f.seek(page_index * 8)
    entry = struct.unpack("Q", f.read(8))[0]

present = bool(entry & (1 << 63))
swapped = bool(entry & (1 << 62))
pfn = entry & ((1 << 55) - 1)

if not present:
    print("Seite nicht resident.",
          "Im Swap." if swapped else "Kein PFN vorhanden.")
    sys.exit(1)

if pfn == 0:
    print("PFN ist 0: möglicherweise fehlende Berechtigung.")
    sys.exit(1)

physical = pfn * page_size + page_offset

print(f"Virtuelle Adresse: 0x{vaddr:x}")
print(f"Page Frame Number: 0x{pfn:x}")
print(f"Physische Adresse: 0x{physical:x}")