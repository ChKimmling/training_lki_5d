#!/usr/bin/env python3

import ctypes
import mmap
import os
import sys

PAGE_SIZE = os.sysconf("SC_PAGE_SIZE")
PAGE_COUNT = 4
PFN_MASK = (1 << 55) - 1

memory = mmap.mmap(
    -1,
    PAGE_COUNT * PAGE_SIZE,
    flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
    prot=mmap.PROT_READ | mmap.PROT_WRITE,
)

base_address = ctypes.addressof(ctypes.c_char.from_buffer(memory))


def read_pagemap(address):
    page_number = address // PAGE_SIZE
    file_offset = page_number * 8

    with open("/proc/self/pagemap", "rb", buffering=0) as pagemap:
        pagemap.seek(file_offset)
        raw = pagemap.read(8)

    if len(raw) != 8:
        raise RuntimeError("pagemap-Eintrag konnte nicht gelesen werden")

    return int.from_bytes(raw, byteorder=sys.byteorder)


def show_pages(title):
    print(f"\n{title}")
    print("Seite  virtuelle Adresse   Eintrag             P S F X D PFN")

    for page in range(PAGE_COUNT):
        address = base_address + page * PAGE_SIZE
        entry = read_pagemap(address)

        present = (entry >> 63) & 1
        swapped = (entry >> 62) & 1
        file_page = (entry >> 61) & 1
        exclusive = (entry >> 56) & 1
        soft_dirty = (entry >> 55) & 1
        pfn = entry & PFN_MASK

        print(
            f"{page:5}  0x{address:016x} "
            f"0x{entry:016x}  "
            f"{present} {swapped} {file_page} "
            f"{exclusive} {soft_dirty} 0x{pfn:x}"
        )


print(f"PID:          {os.getpid()}")
print(f"Seitengröße:  {PAGE_SIZE} Byte")
print(f"Startadresse: 0x{base_address:x}")

show_pages("1. Direkt nach mmap()")

memory[0 * PAGE_SIZE] = 0x11
memory[2 * PAGE_SIZE] = 0x22

show_pages("2. Nach dem Schreiben auf Seite 0 und 2")

with open("/proc/self/clear_refs", "w") as clear_refs:
    clear_refs.write("4\n")

show_pages("3. Nach dem Löschen der Soft-Dirty-Bits")

memory[2 * PAGE_SIZE] = 0x33

show_pages("4. Nach erneutem Schreiben auf Seite 2")
