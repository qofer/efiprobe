#ifndef EFI_H
#define EFI_H

#include <stddef.h>

typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef int                 INT32;
typedef unsigned long long  UINT64;
typedef long long           INT64;
typedef unsigned long long  UINTN;
typedef unsigned char       BOOLEAN;
typedef wchar_t             CHAR16;
typedef void*               EFI_HANDLE;
typedef void*               EFI_EVENT;
typedef UINTN               EFI_STATUS;
typedef UINT64              EFI_PHYSICAL_ADDRESS;
typedef UINT64              EFI_VIRTUAL_ADDRESS;

#define EFIAPI
#define TRUE  1
#define FALSE 0

#define EFI_SUCCESS             0
#define EFI_LOAD_ERROR          0x8000000000000001ULL
#define EFI_INVALID_PARAMETER   0x8000000000000002ULL
#define EFI_UNSUPPORTED         0x8000000000000003ULL
#define EFI_BAD_BUFFER_SIZE     0x8000000000000004ULL
#define EFI_BUFFER_TOO_SMALL    0x8000000000000005ULL
#define EFI_NOT_READY           0x8000000000000006ULL
#define EFI_OUT_OF_RESOURCES    0x8000000000000009ULL
#define EFI_NOT_FOUND           0x800000000000000EULL
#define EFI_ACCESS_DENIED       0x800000000000000FULL
#define EFI_END_OF_FILE         0x8000000000000012ULL
#define EFI_ERROR_BIT           0x8000000000000000ULL
#define EFI_ERROR(s)            (((INT64)(s)) < 0)

#define EFI_PAGE_SIZE  4096
#define EFI_SIZE_TO_PAGES(a) (((a) >> 12) + (((a) & 0xfff) ? 1 : 0))

#define EFI_MEMORY_UC          0x0000000000000001ULL
#define EFI_MEMORY_WC          0x0000000000000002ULL
#define EFI_MEMORY_WT          0x0000000000000004ULL
#define EFI_MEMORY_WB          0x0000000000000008ULL
#define EFI_MEMORY_UCE         0x0000000000000010ULL
#define EFI_MEMORY_WP          0x0000000000001000ULL
#define EFI_MEMORY_RP          0x0000000000002000ULL
#define EFI_MEMORY_XP          0x0000000000004000ULL
#define EFI_MEMORY_NV          0x0000000000008000ULL
#define EFI_MEMORY_MORE_RELIABLE 0x0000000000010000ULL
#define EFI_MEMORY_RO          0x0000000000020000ULL
#define EFI_MEMORY_SP          0x0000000000040000ULL
#define EFI_MEMORY_CPU_CRYPTO  0x0000000000080000ULL
#define EFI_MEMORY_RUNTIME     0x8000000000000000ULL

#define EfiReservedMemoryType      0
#define EfiLoaderCode              1
#define EfiLoaderData              2
#define EfiBootServicesCode        3
#define EfiBootServicesData        4
#define EfiRuntimeServicesCode     5
#define EfiRuntimeServicesData     6
#define EfiConventionalMemory      7
#define EfiUnusableMemory          8
#define EfiACPIReclaimMemory       9
#define EfiACPIMemoryNVS           10
#define EfiMemoryMappedIO          11
#define EfiMemoryMappedIOPortSpace 12
#define EfiPalCode                 13
#define EfiPersistentMemory        14
#define EfiMaxMemoryType           15

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    UINT32 Type;
    UINT32 Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS  VirtualStart;
    UINT64 NumberOfPages;
    UINT64 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    EFI_GUID VendorGuid;
    void*    VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct {
    INT32   MaxMode;
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL  EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_STATUS (EFIAPI *Reset)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL*, BOOLEAN);
    EFI_STATUS (EFIAPI *ReadKeyStroke)(EFI_SIMPLE_TEXT_INPUT_PROTOCOL*, EFI_INPUT_KEY*);
    void* WaitForKey;
};

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_STATUS (EFIAPI *Reset)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, BOOLEAN);
    EFI_STATUS (EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
    EFI_STATUS (EFIAPI *TestString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
    EFI_STATUS (EFIAPI *QueryMode)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, UINTN, UINTN*, UINTN*);
    EFI_STATUS (EFIAPI *SetMode)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, UINTN);
    EFI_STATUS (EFIAPI *SetAttribute)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, UINTN);
    EFI_STATUS (EFIAPI *ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*);
    EFI_STATUS (EFIAPI *SetCursorPosition)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, UINTN, UINTN);
    EFI_STATUS (EFIAPI *EnableCursor)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, BOOLEAN);
    EFI_SIMPLE_TEXT_OUTPUT_MODE* Mode;
};

typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(UINTN*, EFI_MEMORY_DESCRIPTOR*, UINTN*, UINTN*, UINT32*);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(UINTN, UINTN, void**);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void*);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(EFI_HANDLE, EFI_GUID*, void**);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(EFI_GUID*, void*, void**);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN);
typedef EFI_STATUS (EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(UINT64*);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void* RaiseTPL;
    void* RestoreTPL;
    void* AllocatePages;
    void* FreePages;
    void* GetMemoryMap;
    void* AllocatePool;
    void* FreePool;
    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    void* HandleProtocol;
    void* Reserved;
    void* RegisterProtocolNotify;
    void* LocateHandle;
    void* LocateDevicePath;
    void* InstallConfigurationTable;
    void* LoadImage;
    void* StartImage;
    void* Exit;
    void* UnloadImage;
    void* ExitBootServices;
    void* GetNextMonotonicCount;
    void* Stall;
    void* SetWatchdogTimer;
    void* ConnectController;
    void* DisconnectController;
    void* OpenProtocol;
    void* CloseProtocol;
    void* OpenProtocolInformation;
    void* ProtocolsPerHandle;
    void* LocateHandleBuffer;
    void* LocateProtocol;
    void* InstallMultipleProtocolInterfaces;
    void* UninstallMultipleProtocolInterfaces;
    void* CalculateCrc32;
    void* CopyMem;
    void* SetMem;
    void* CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_TABLE_HEADER                 Hdr;
    CHAR16*                          FirmwareVendor;
    UINT32                           FirmwareRevision;
    UINT32                           Pad0;
    EFI_HANDLE                       ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL*  ConIn;
    EFI_HANDLE                       ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_HANDLE                       StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* StdErr;
    void*                            RuntimeServices;
    EFI_BOOT_SERVICES*               BootServices;
    UINTN                            NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE*         ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    UINT32 PixelFormat;
    UINT32 PixelInformation[4];
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    UINTN  SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN                FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_GOP_QUERY_MODE)(EFI_GRAPHICS_OUTPUT_PROTOCOL*, UINT32, UINTN*, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION**);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
};

typedef struct {
    UINT32 Revision;
    UINT32 Pad0;
    EFI_HANDLE        ParentHandle;
    EFI_SYSTEM_TABLE* SystemTable;
    EFI_HANDLE        DeviceHandle;
    void*             FilePath;
    void*             Reserved;
    UINT32            LoadOptionsSize;
    UINT32            Pad1;
    void*             LoadOptions;
    void*             ImageBase;
    UINT64            ImageSize;
    UINT32            ImageCodeType;
    UINT32            ImageDataType;
    void*             Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

static const EFI_GUID gEfiLoadedImageProtocolGuid = {
    0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
};
static const EFI_GUID gEfiGraphicsOutputProtocolGuid = {
    0x9042A9DE, 0x23DC, 0x4A38, { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A }
};

#endif
