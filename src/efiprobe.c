#include "efi.h"

static EFI_SYSTEM_TABLE* gST;
static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* gOut;
static CHAR16 buf[192];

static void p(CHAR16* s) {
    if (s && gOut) gOut->OutputString(gOut, s);
}

static void nl(void) {
    p(L"\r\n");
}

static void hex(UINT64 v, int digits) {
    static const CHAR16 h[] = L"0123456789ABCDEF";
    int i;
    for (i = 0; i < digits; i++)
        buf[i] = h[(v >> ((digits - 1 - i) * 4)) & 0xF];
    buf[digits] = 0;
    p(buf);
}

static void hx(UINT64 v, int digits) {
    p(L"0x");
    hex(v, digits);
}

static void dec(UINT64 v) {
    CHAR16 tmp[24];
    int n = 0, i;
    if (!v) { p(L"0"); return; }
    while (v) {
        tmp[n++] = (CHAR16)(L'0' + (v % 10));
        v /= 10;
    }
    for (i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
    p(buf);
}

static void pad(int n) {
    int i;
    for (i = 0; i < n; i++) p(L" ");
}

static int slen(CHAR16* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void pad_after(CHAR16* s, int width) {
    int n = width - slen(s);
    if (n > 0) pad(n);
}

static const CHAR16* mem_type_name(UINT32 t) {
    static const CHAR16* n[] = {
        L"reserved", L"loader code", L"loader data", L"bs code", L"bs data",
        L"rt code", L"rt data", L"conventional", L"unusable", L"acpi reclaim",
        L"acpi nvs", L"mmio", L"mmio port", L"pal code", L"persistent"
    };
    return t < EfiMaxMemoryType ? n[t] : L"?";
}

static const CHAR16* px_fmt(UINT32 f) {
    static const CHAR16* n[] = {
        L"rgba8", L"bgra8", L"bitmask", L"blt-only", L"?"
    };
    return f < 4 ? n[f] : n[4];
}

static void mem_attr(UINT64 a) {
    struct { UINT64 bit; CHAR16* s; } f[] = {
        { EFI_MEMORY_UC, L"UC " }, { EFI_MEMORY_WC, L"WC " },
        { EFI_MEMORY_WT, L"WT " }, { EFI_MEMORY_WB, L"WB " },
        { EFI_MEMORY_UCE, L"UCE " }, { EFI_MEMORY_WP, L"WP " },
        { EFI_MEMORY_RP, L"RP " }, { EFI_MEMORY_XP, L"XP " },
        { EFI_MEMORY_NV, L"NV " }, { EFI_MEMORY_RO, L"RO " },
        { EFI_MEMORY_SP, L"SP " }, { EFI_MEMORY_CPU_CRYPTO, L"CRYPTO " },
        { EFI_MEMORY_RUNTIME, L"RT " }
    };
    int i;
    for (i = 0; i < 13; i++)
        if (a & f[i].bit) p(f[i].s);
}

typedef struct { EFI_GUID g; CHAR16* n; } GUID_NAME;

static const GUID_NAME gGuids[] = {
    { { 0xEB9D2D30, 0x2D88, 0x11D3, { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } }, L"ACPI 1.0 RSDP" },
    { { 0x8868E871, 0xE4F1, 0x11D3, { 0xBC, 0x22, 0x00, 0x80, 0xC7, 0x3C, 0x88, 0x81 } }, L"ACPI 2.0 RSDP" },
    { { 0xEB9D2D31, 0x2D88, 0x11D3, { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } }, L"SMBIOS" },
    { { 0xF2FD1544, 0x9794, 0x4A2C, { 0x99, 0x2E, 0xE5, 0xBB, 0xCF, 0x20, 0xE3, 0x94 } }, L"SMBIOS3" },
    { { 0xEB9D2D2F, 0x2D88, 0x11D3, { 0x9A, 0x16, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D } }, L"MPS" },
    { { 0xB122A263, 0x3661, 0x4F68, { 0x99, 0x29, 0x78, 0xF8, 0xB0, 0xD6, 0x21, 0x80 } }, L"ESRT" },
    { { 0x5568E427, 0x68FC, 0x4F3D, { 0xAC, 0x74, 0xCA, 0x55, 0x52, 0x31, 0xCC, 0x68 } }, L"initrd media" },
    { { 0xB1B621D5, 0xF19C, 0x41A5, { 0x83, 0x0B, 0xD9, 0x15, 0x2C, 0x69, 0xAA, 0xE0 } }, L"device tree" },
    { { 0x4C19049F, 0x4137, 0x4DD3, { 0x9C, 0x10, 0x8B, 0x97, 0xA8, 0x3F, 0xFD, 0xFA } }, L"memory attributes" },
    { { 0xDCFA911D, 0x26DE, 0x476F, { 0x9A, 0xAC, 0x36, 0x51, 0x3F, 0x9F, 0x6A, 0x9B } }, L"SMM communication" }
};

static int guid_eq(EFI_GUID* a, EFI_GUID* b) {
    int i;
    if (a->Data1 != b->Data1 || a->Data2 != b->Data2 || a->Data3 != b->Data3)
        return 0;
    for (i = 0; i < 8; i++)
        if (a->Data4[i] != b->Data4[i]) return 0;
    return 1;
}

static void putguid(EFI_GUID* g) {
    int i;
    hex(g->Data1, 8); p(L"-"); hex(g->Data2, 4); p(L"-"); hex(g->Data3, 4);
    p(L"-");
    for (i = 0; i < 2; i++) hex(g->Data4[i], 2);
    p(L"-");
    for (i = 2; i < 8; i++) hex(g->Data4[i], 2);
}

static CHAR16* guid_name(EFI_GUID* g) {
    int i;
    for (i = 0; i < 10; i++)
        if (guid_eq(g, (EFI_GUID*)&gGuids[i].g)) return gGuids[i].n;
    return L"-";
}

static void banner(void) {
    p(L"efiprobe 0.2  |  ");
    p(gST->FirmwareVendor ? gST->FirmwareVendor : L"?");
    p(L"  rev ");
    dec((gST->FirmwareRevision >> 16) & 0xFFFF);
    p(L".");
    dec(gST->FirmwareRevision & 0xFFFF);
    p(L"  |  uefi spec ");
    dec(gST->Hdr.Revision >> 16);
    p(L".");
    dec(gST->Hdr.Revision & 0xFFFF);
    nl();
    p(L"------------------------------------------------------------");
    nl();
}

static void conout_info(void) {
    UINTN cols = 0, rows = 0;
    INT32 mode = gOut->Mode ? gOut->Mode->Mode : -1;
    EFI_STATUS s = gOut->QueryMode(gOut, (UINTN)mode, &cols, &rows);
    p(L"conout      mode "); dec(mode);
    p(L"  cols ");
    if (!EFI_ERROR(s)) { dec(cols); p(L" rows "); dec(rows); }
    else p(L"n/a");
    p(L"  conout @ "); hx((UINT64)gOut, 16);
    nl();
}

static void loaded_image(EFI_BOOT_SERVICES* bs, EFI_HANDLE ih) {
    EFI_LOADED_IMAGE_PROTOCOL* li = NULL;
    EFI_STATUS s = ((EFI_HANDLE_PROTOCOL)bs->HandleProtocol)(
        ih, (EFI_GUID*)&gEfiLoadedImageProtocolGuid, (void**)&li);
    if (EFI_ERROR(s) || !li) {
        p(L"loaded image  <not available "); hx(s, 16); p(L">"); nl();
        return;
    }
    p(L"loaded image  base "); hx((UINT64)li->ImageBase, 16);
    p(L"  size "); hx(li->ImageSize, 8);
    p(L"  code "); hex(li->ImageCodeType, 2);
    p(L"  opts "); dec(li->LoadOptionsSize);
    p(L"  device "); hx((UINT64)li->DeviceHandle, 16);
    nl();
}

static void config_tables(void) {
    UINTN i;
    p(L"config tables  "); dec(gST->NumberOfTableEntries); nl();
    for (i = 0; i < gST->NumberOfTableEntries && i < 32; i++) {
        EFI_CONFIGURATION_TABLE* t = &gST->ConfigurationTable[i];
        p(L"  ");
        putguid(&t->VendorGuid);
        p(L"  ");
        p(guid_name(&t->VendorGuid));
        p(L"  @ "); hx((UINT64)t->VendorTable, 16);
        nl();
    }
}

static void gop_modes(EFI_BOOT_SERVICES* bs) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL* g = NULL;
    UINT32 m;
    EFI_STATUS s = ((EFI_LOCATE_PROTOCOL)bs->LocateProtocol)(
        (EFI_GUID*)&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&g);
    if (EFI_ERROR(s) || !g || !g->Mode) {
        p(L"gop         <none "); hx(s, 16); p(L">"); nl();
        return;
    }
    p(L"gop         mode "); dec(g->Mode->Mode);
    p(L" / "); dec(g->Mode->MaxMode);
    p(L"  fb "); hx(g->Mode->FrameBufferBase, 16);
    p(L"  size "); hx(g->Mode->FrameBufferSize, 12);
    if (g->Mode->Info) {
        p(L"  cur "); dec(g->Mode->Info->HorizontalResolution);
        p(L"x"); dec(g->Mode->Info->VerticalResolution);
        p(L"  "); p((CHAR16*)px_fmt(g->Mode->Info->PixelFormat));
    }
    nl();
    for (m = 0; m < g->Mode->MaxMode && m < 24; m++) {
        UINTN sz = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
        s = ((EFI_GOP_QUERY_MODE)g->QueryMode)(g, m, &sz, &info);
        if (EFI_ERROR(s) || !info) {
            p(L"  "); dec(m); p(L": <query failed "); hx(s, 16);
            if (!info) p(L" null info");
            p(L">"); nl();
            if (info) ((EFI_FREE_POOL)bs->FreePool)(info);
            continue;
        }
        p(L"  "); pad(2); dec(m); p(L": ");
        pad(4 - (m >= 10 ? 2 : 1));
        dec(info->HorizontalResolution); p(L"x"); dec(info->VerticalResolution);
        p(L"  stride "); dec(info->PixelsPerScanLine);
        p(L"  "); p((CHAR16*)px_fmt(info->PixelFormat));
        if (m == g->Mode->Mode) p(L"  *");
        nl();
        ((EFI_FREE_POOL)bs->FreePool)(info);
    }
}

static void memory_map(EFI_BOOT_SERVICES* bs) {
    UINTN size = 0, key = 0, dsize = 0, used = 0;
    UINT32 dver = 0;
    UINT64 total[EfiMaxMemoryType];
    UINT64 pages = 0;
    int i, attempt;
    UINT8* cur;
    EFI_STATUS s;
    void* mem = NULL;

    for (i = 0; i < EfiMaxMemoryType; i++) total[i] = 0;

    s = ((EFI_GET_MEMORY_MAP)bs->GetMemoryMap)(&size, NULL, &key, &dsize, &dver);
    if (s != EFI_BUFFER_TOO_SMALL) {
        p(L"memmap      sizing failed "); hx(s, 16); nl();
        return;
    }

    for (attempt = 0; attempt < 8; attempt++) {
        if (mem) {
            ((EFI_FREE_POOL)bs->FreePool)(mem);
            mem = NULL;
        }
        size += dsize * 8;
        s = ((EFI_ALLOCATE_POOL)bs->AllocatePool)(EfiLoaderData, size, &mem);
        if (EFI_ERROR(s) || !mem) {
            p(L"memmap      alloc failed "); hx(s, 16); nl();
            return;
        }
        s = ((EFI_GET_MEMORY_MAP)bs->GetMemoryMap)(&size, mem, &key, &dsize, &dver);
        if (s == EFI_SUCCESS) break;
        if (s != EFI_BUFFER_TOO_SMALL) {
            p(L"memmap      read failed "); hx(s, 16); nl();
            ((EFI_FREE_POOL)bs->FreePool)(mem);
            return;
        }
        p(L"memmap      map grew, retrying (");
        dec(attempt + 1); p(L"/8)"); nl();
    }
    if (s != EFI_SUCCESS) {
        p(L"memmap      retries exhausted "); hx(s, 16); nl();
        if (mem) ((EFI_FREE_POOL)bs->FreePool)(mem);
        return;
    }

    p(L"memmap      "); dec(size / dsize);
    p(L" descriptors  desc "); dec(dsize);
    p(L" bytes  ver "); hex(dver, 8);
    p(L"  key "); hx(key, 16);
    nl();

    cur = (UINT8*)mem;
    while (used + dsize <= size) {
        EFI_MEMORY_DESCRIPTOR* d = (EFI_MEMORY_DESCRIPTOR*)cur;
        if (d->Type < EfiMaxMemoryType) total[d->Type] += d->NumberOfPages;
        pages += d->NumberOfPages;
        if (d->Attribute & EFI_MEMORY_RUNTIME) {
            p(L"  rt  "); hx(d->PhysicalStart, 16);
            p(L"  "); hx(d->NumberOfPages * EFI_PAGE_SIZE, 12);
            p(L"  "); p((CHAR16*)mem_type_name(d->Type));
            p(L"  "); mem_attr(d->Attribute);
            nl();
        }
        cur += dsize;
        used += dsize;
    }
    p(L"  address space "); dec(pages * EFI_PAGE_SIZE / (1024 * 1024));
    p(L" MiB (incl. reserved/MMIO)"); nl();
    for (i = 0; i < EfiMaxMemoryType; i++) {
        if (!total[i]) continue;
        p(L"  "); p((CHAR16*)mem_type_name(i));
        pad_after((CHAR16*)mem_type_name(i), 16);
        p(L" "); dec(total[i] * EFI_PAGE_SIZE / (1024 * 1024)); p(L" MiB");
        nl();
    }
    ((EFI_FREE_POOL)bs->FreePool)(mem);
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    gST = SystemTable;
    gOut = SystemTable->ConOut;
    if (!gOut) return EFI_UNSUPPORTED;

    gOut->ClearScreen(gOut);
    gOut->SetAttribute(gOut, 0x07);
    banner();
    conout_info();
    loaded_image(SystemTable->BootServices, ImageHandle);
    nl();
    config_tables();
    nl();
    gop_modes(SystemTable->BootServices);
    nl();
    memory_map(SystemTable->BootServices);

    return EFI_SUCCESS;
}
