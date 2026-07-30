#include <stdio.h>
#include <stddef.h>
#include "efi.h"

static int bad = 0;

#define CK(t, f, want) do {                                                   \
    size_t got = offsetof(t, f);                                              \
    if (got != (size_t)(want)) {                                              \
        printf("  BAD  %-18s %-22s got 0x%zX want 0x%zX\n", #t, #f, got,   \
               (size_t)(want));                                              \
        bad++;                                                                \
    } else {                                                                  \
        printf("  ok   %-18s %-22s 0x%zX\n", #t, #f, got);                  \
    }                                                                         \
} while (0)

#define SZ(t, want) do {                                                      \
    size_t got = sizeof(t);                                                   \
    if (got != (size_t)(want)) {                                              \
        printf("  BAD  sizeof(%-18s) got 0x%zX want 0x%zX\n", #t, got,      \
               (size_t)(want));                                              \
        bad++;                                                                \
    }                                                                         \
} while (0)

int main(void) {
    printf("efi struct layout (x64, msvc)\n");
    SZ(EFI_GUID, 0x10);
    SZ(EFI_TABLE_HEADER, 0x18);
    SZ(EFI_MEMORY_DESCRIPTOR, 0x28);
    SZ(EFI_CONFIGURATION_TABLE, 0x18);
    SZ(EFI_SYSTEM_TABLE, 0x78);
    SZ(EFI_BOOT_SERVICES, 0x178);
    SZ(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION, 0x24);
    SZ(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE, 0x28);
    SZ(EFI_LOADED_IMAGE_PROTOCOL, 0x60);

    CK(EFI_MEMORY_DESCRIPTOR, Type, 0x00);
    CK(EFI_MEMORY_DESCRIPTOR, PhysicalStart, 0x08);
    CK(EFI_MEMORY_DESCRIPTOR, VirtualStart, 0x10);
    CK(EFI_MEMORY_DESCRIPTOR, NumberOfPages, 0x18);
    CK(EFI_MEMORY_DESCRIPTOR, Attribute, 0x20);

    CK(EFI_SYSTEM_TABLE, FirmwareVendor, 0x18);
    CK(EFI_SYSTEM_TABLE, FirmwareRevision, 0x20);
    CK(EFI_SYSTEM_TABLE, ConsoleInHandle, 0x28);
    CK(EFI_SYSTEM_TABLE, ConIn, 0x30);
    CK(EFI_SYSTEM_TABLE, ConsoleOutHandle, 0x38);
    CK(EFI_SYSTEM_TABLE, ConOut, 0x40);
    CK(EFI_SYSTEM_TABLE, StandardErrorHandle, 0x48);
    CK(EFI_SYSTEM_TABLE, StdErr, 0x50);
    CK(EFI_SYSTEM_TABLE, RuntimeServices, 0x58);
    CK(EFI_SYSTEM_TABLE, BootServices, 0x60);
    CK(EFI_SYSTEM_TABLE, NumberOfTableEntries, 0x68);
    CK(EFI_SYSTEM_TABLE, ConfigurationTable, 0x70);

    CK(EFI_BOOT_SERVICES, GetMemoryMap, 0x38);
    CK(EFI_BOOT_SERVICES, AllocatePool, 0x40);
    CK(EFI_BOOT_SERVICES, FreePool, 0x48);
    CK(EFI_BOOT_SERVICES, HandleProtocol, 0x98);
    CK(EFI_BOOT_SERVICES, LocateProtocol, 0x140);
    CK(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL, OutputString, 0x08);
    CK(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL, QueryMode, 0x18);
    CK(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL, Mode, 0x48);
    CK(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE, Info, 0x08);
    CK(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE, FrameBufferBase, 0x18);
    CK(EFI_LOADED_IMAGE_PROTOCOL, ImageBase, 0x40);
    CK(EFI_LOADED_IMAGE_PROTOCOL, Unload, 0x58);

    if (bad) {
        printf("%d mismatch(es)\n", bad);
        return 1;
    }
    printf("all offsets match uefi 2.x x64\n");
    return 0;
}
