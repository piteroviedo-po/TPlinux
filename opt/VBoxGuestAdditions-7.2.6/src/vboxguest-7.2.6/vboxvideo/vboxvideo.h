/* $Id: vboxvideo.h 170187 2025-08-11 17:18:47Z klaus $ */
/*
 * Copyright (C) 2006-2025 Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOX_INCLUDED_Graphics_VBoxVideo_h
#define VBOX_INCLUDED_Graphics_VBoxVideo_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "vbox_err.h"

/* this should be in sync with monitorCount <xsd:maxInclusive value="64"/> in src/VBox/Main/xml/VirtualBox-settings-common.xsd */
#define VBOX_VIDEO_MAX_SCREENS 64

/*
 * The last 4096 bytes of the guest VRAM contains the generic info for all
 * DualView chunks: sizes and offsets of chunks. This is filled by miniport.
 *
 * Last 4096 bytes of each chunk contain chunk specific data: framebuffer info,
 * etc. This is used exclusively by the corresponding instance of a display driver.
 *
 * The VRAM layout:
 *     Last 4096 bytes - Adapter information area.
 *     4096 bytes aligned miniport heap (value specified in the config rouded up).
 *     Slack - what left after dividing the VRAM.
 *     4096 bytes aligned framebuffers:
 *       last 4096 bytes of each framebuffer is the display information area.
 *
 * The Virtual Graphics Adapter information in the guest VRAM is stored by the
 * guest video driver using structures prepended by VBOXVIDEOINFOHDR.
 *
 * When the guest driver writes dword 0 to the VBE_DISPI_INDEX_VBOX_VIDEO
 * the host starts to process the info. The first element at the start of
 * the 4096 bytes region should be normally be a LINK that points to
 * actual information chain. That way the guest driver can have some
 * fixed layout of the information memory block and just rewrite
 * the link to point to relevant memory chain.
 *
 * The processing stops at the END element.
 *
 * The host can access the memory only when the port IO is processed.
 * All data that will be needed later must be copied from these 4096 bytes.
 * But other VRAM can be used by host until the mode is disabled.
 *
 * The guest driver writes dword 0xffffffff to the VBE_DISPI_INDEX_VBOX_VIDEO
 * to disable the mode.
 *
 * VBE_DISPI_INDEX_VBOX_VIDEO is used to read the configuration information
 * from the host and issue commands to the host.
 *
 * The guest writes the VBE_DISPI_INDEX_VBOX_VIDEO index register, the the
 * following operations with the VBE data register can be performed:
 *
 * Operation            Result
 * write 16 bit value   NOP
 * read 16 bit value    count of monitors
 * write 32 bit value   sets the vbox command value and the command processed by the host
 * read 32 bit value    result of the last vbox command is returned
 */

#define VBOX_VIDEO_PRIMARY_SCREEN 0
#define VBOX_VIDEO_NO_SCREEN ~0

/**
 * VBVA command header.
 *
 * @todo Where does this fit in?
 */
typedef struct VBVACMDHDR {
   /** Coordinates of affected rectangle. */
   int16_t x;
   int16_t y;
   u16 w;
   u16 h;
} VBVACMDHDR;
assert_compile_size(VBVACMDHDR, 8);

/** @name VBVA ring defines.
 *
 * The VBVA ring buffer is suitable for transferring large (< 2GB) amount of
 * data. For example big bitmaps which do not fit to the buffer.
 *
 * Guest starts writing to the buffer by initializing a record entry in the
 * records queue. VBVA_F_RECORD_PARTIAL indicates that the record is being
 * written. As data is written to the ring buffer, the guest increases off32End
 * for the record.
 *
 * The host reads the records on flushes and processes all completed records.
 * When host encounters situation when only a partial record presents and
 * len_and_flags & ~VBVA_F_RECORD_PARTIAL >= VBVA_RING_BUFFER_SIZE -
 * VBVA_RING_BUFFER_THRESHOLD, the host fetched all record data and updates
 * off32Head. After that on each flush the host continues fetching the data
 * until the record is completed.
 *
 */
#define VBVA_RING_BUFFER_SIZE        (4*1024*1024 - 1024)
#define VBVA_RING_BUFFER_THRESHOLD   (4 * 1024)

#define VBVA_MAX_RECORDS (64)

#define VBVA_F_MODE_ENABLED         0x00000001u
#define VBVA_F_MODE_VRDP            0x00000002u
#define VBVA_F_MODE_VRDP_RESET      0x00000004u
#define VBVA_F_MODE_VRDP_ORDER_MASK 0x00000008u

#define VBVA_F_STATE_PROCESSING     0x00010000u

#define VBVA_F_RECORD_PARTIAL       0x80000000u

/**
 * VBVA record.
 */
typedef struct VBVARECORD {
	/** The length of the record. Changed by guest. */
	u32 len_and_flags;
} VBVARECORD;
assert_compile_size(VBVARECORD, 4);

/* The size of the information. */
/*
 * The minimum HGSMI heap size is PAGE_SIZE (4096 bytes) and is a restriction of the
 * runtime heapsimple API. Use minimum 2 pages here, because the info area also may
 * contain other data (for example struct hgsmi_host_flags structure).
 */
#ifndef VBOX_XPDM_MINIPORT
# define VBVA_ADAPTER_INFORMATION_SIZE (64*1024)
#else
#define VBVA_ADAPTER_INFORMATION_SIZE  (16*1024)
#define VBVA_DISPLAY_INFORMATION_SIZE  (64*1024)
#endif
#define VBVA_MIN_BUFFER_SIZE           (64*1024)


/* The value for port IO to let the adapter to interpret the adapter memory. */
#define VBOX_VIDEO_DISABLE_ADAPTER_MEMORY        0xFFFFFFFF

/* The value for port IO to let the adapter to interpret the adapter memory. */
#define VBOX_VIDEO_INTERPRET_ADAPTER_MEMORY      0x00000000

/* The value for port IO to let the adapter to interpret the display memory.
 * The display number is encoded in low 16 bits.
 */
#define VBOX_VIDEO_INTERPRET_DISPLAY_MEMORY_BASE 0x00010000


/* The end of the information. */
#define VBOX_VIDEO_INFO_TYPE_END          0
/* Instructs the host to fetch the next VBOXVIDEOINFOHDR at the given offset of VRAM. */
#define VBOX_VIDEO_INFO_TYPE_LINK         1
/* Information about a display memory position. */
#define VBOX_VIDEO_INFO_TYPE_DISPLAY      2
/* Information about a screen. */
#define VBOX_VIDEO_INFO_TYPE_SCREEN       3
/* Information about host notifications for the driver. */
#define VBOX_VIDEO_INFO_TYPE_HOST_EVENTS  4
/* Information about non-volatile guest VRAM heap. */
#define VBOX_VIDEO_INFO_TYPE_NV_HEAP      5
/* VBVA enable/disable. */
#define VBOX_VIDEO_INFO_TYPE_VBVA_STATUS  6
/* VBVA flush. */
#define VBOX_VIDEO_INFO_TYPE_VBVA_FLUSH   7
/* Query configuration value. */
#define VBOX_VIDEO_INFO_TYPE_QUERY_CONF32 8


#pragma pack(1)
typedef struct VBOXVIDEOINFOHDR {
	u8 u8Type;
	u8 u8Reserved;
	u16 u16Length;
} VBOXVIDEOINFOHDR;


typedef struct VBOXVIDEOINFOLINK {
	/* Relative offset in VRAM */
	s32 i32Offset;
} VBOXVIDEOINFOLINK;


/* Resides in adapter info memory. Describes a display VRAM chunk. */
typedef struct VBOXVIDEOINFODISPLAY {
	/* Index of the framebuffer assigned by guest. */
	u32 index;

	/* Absolute offset in VRAM of the framebuffer to be displayed on the monitor. */
	u32 offset;

	/* The size of the memory that can be used for the screen. */
	u32 u32FramebufferSize;

	/* The size of the memory that is used for the Display information.
	 * The information is at offset + u32FramebufferSize
	 */
	u32 u32InformationSize;

} VBOXVIDEOINFODISPLAY;


/* Resides in display info area, describes the current video mode. */
#define VBOX_VIDEO_INFO_SCREEN_F_NONE   0x00
#define VBOX_VIDEO_INFO_SCREEN_F_ACTIVE 0x01

typedef struct VBOXVIDEOINFOSCREEN {
	/* Physical X origin relative to the primary screen. */
	s32 xOrigin;

	/* Physical Y origin relative to the primary screen. */
	s32 yOrigin;

	/* The scan line size in bytes. */
	u32 line_size;

	/* Width of the screen. */
	u16 u16Width;

	/* Height of the screen. */
	u16 u16Height;

	/* Color depth. */
	u8 bitsPerPixel;

	/* VBOX_VIDEO_INFO_SCREEN_F_* */
	u8 u8Flags;
} VBOXVIDEOINFOSCREEN;

/* The guest initializes the structure to 0. The positions of the structure in the
 * display info area must not be changed, host will update the structure. Guest checks
 * the events and modifies the structure as a response to host.
 */
#define VBOX_VIDEO_INFO_HOST_EVENTS_F_NONE        0x00000000
#define VBOX_VIDEO_INFO_HOST_EVENTS_F_VRDP_RESET  0x00000080

typedef struct VBOXVIDEOINFOHOSTEVENTS {
	/* Host events. */
	u32 fu32Events;
} VBOXVIDEOINFOHOSTEVENTS;

/* Resides in adapter info memory. Describes the non-volatile VRAM heap. */
typedef struct VBOXVIDEOINFONVHEAP {
	/* Absolute offset in VRAM of the start of the heap. */
	u32 u32HeapOffset;

	/* The size of the heap. */
	u32 u32HeapSize;

} VBOXVIDEOINFONVHEAP;

/* Display information area. */
typedef struct VBOXVIDEOINFOVBVASTATUS {
	/* Absolute offset in VRAM of the start of the VBVA QUEUE. 0 to disable VBVA. */
	u32 u32QueueOffset;

	/* The size of the VBVA QUEUE. 0 to disable VBVA. */
	u32 u32QueueSize;

} VBOXVIDEOINFOVBVASTATUS;

typedef struct VBOXVIDEOINFOVBVAFLUSH {
	u32 u32DataStart;

	u32 u32DataEnd;

} VBOXVIDEOINFOVBVAFLUSH;

#define VBOX_VIDEO_QCI32_MONITOR_COUNT       0
#define VBOX_VIDEO_QCI32_OFFSCREEN_HEAP_SIZE 1

typedef struct VBOXVIDEOINFOQUERYCONF32 {
	u32 index;

	u32 value;

} VBOXVIDEOINFOQUERYCONF32;
#pragma pack()

/* All structures are without alignment. */
#pragma pack(1)

typedef struct VBVAHOSTFLAGS {
	u32 host_events;
	u32 supported_orders;
} VBVAHOSTFLAGS;

typedef struct VBVABUFFER {
	VBVAHOSTFLAGS host_flags;

	/* The offset where the data start in the buffer. */
	u32 data_offset;
	/* The offset where next data must be placed in the buffer. */
	u32 free_offset;

	/* The queue of record descriptions. */
	VBVARECORD records[VBVA_MAX_RECORDS];
	u32 first_record_index;
	u32 free_record_index;

	/* Space to leave free in the buffer when large partial records are transferred. */
	u32 partial_write_tresh;

	u32 data_len;
	u8  data[1]; /* variable size for the rest of the VBVABUFFER area in VRAM. */
} VBVABUFFER;

#define VBVA_MAX_RECORD_SIZE (128*_1M)

/* guest->host commands */
#define VBVA_QUERY_CONF32 1
#define VBVA_SET_CONF32   2
#define VBVA_INFO_VIEW    3
#define VBVA_INFO_HEAP    4
#define VBVA_FLUSH        5
#define VBVA_INFO_SCREEN  6
/** Enables or disables VBVA.  Enabling VBVA without disabling it before
 * causes a complete screen update. */
#define VBVA_ENABLE       7
#define VBVA_MOUSE_POINTER_SHAPE 8
#ifdef VBOX_WITH_VDMA
# define VBVA_VDMA_CTL   10 /* setup G<->H DMA channel info */
# define VBVA_VDMA_CMD    11 /* G->H DMA command             */
#endif
#define VBVA_INFO_CAPS   12 /* informs host about HGSMI caps. see struct vbva_caps below */
#define VBVA_SCANLINE_CFG    13 /* configures scanline, see VBVASCANLINECFG below */
#define VBVA_SCANLINE_INFO   14 /* requests scanline info, see VBVASCANLINEINFO below */
#define VBVA_CMDVBVA_SUBMIT  16 /* inform host about VBVA Command submission */
#define VBVA_CMDVBVA_FLUSH   17 /* inform host about VBVA Command submission */
#define VBVA_CMDVBVA_CTL     18 /* G->H DMA command             */
#define VBVA_QUERY_MODE_HINTS 19 /* Query most recent mode hints sent. */
/** Report the guest virtual desktop position and size for mapping host and
 * guest pointer positions. */
#define VBVA_REPORT_INPUT_MAPPING 20
/** Report the guest cursor position and query the host position. */
#define VBVA_CURSOR_POSITION 21

/* host->guest commands */
#define VBVAHG_EVENT              1
#define VBVAHG_DISPLAY_CUSTOM     2
#ifdef VBOX_WITH_VDMA
#define VBVAHG_SHGSMI_COMPLETION  3
#endif

#pragma pack(1)
typedef enum {
	VBVAHOSTCMD_OP_EVENT = 1,
	VBVAHOSTCMD_OP_CUSTOM
}VBVAHOSTCMD_OP_TYPE;

typedef struct VBVAHOSTCMDEVENT {
	uint64_t pEvent;
}VBVAHOSTCMDEVENT;


typedef struct VBVAHOSTCMD {
	/* destination ID if >=0 specifies display index, otherwize the command is directed to the miniport */
	s32 iDstID;
	s32 customOpCode;
	union {
		struct VBVAHOSTCMD *pNext;
		u32             offNext;
		uint64_t Data; /* the body is 64-bit aligned */
	} u;
	char body[1];
} VBVAHOSTCMD;

#define VBVAHOSTCMD_SIZE(a_cb)                  (sizeof(VBVAHOSTCMD) + (a_cb))
#define VBVAHOSTCMD_BODY(a_pCmd, a_TypeBody)    ((a_TypeBody  *)&(a_pCmd)->body[0])
#define VBVAHOSTCMD_HDR(a_pBody) \
	( (VBVAHOSTCMD  *)( (u8 *)(a_pBody) - RT_OFFSETOF(VBVAHOSTCMD, body)) )
#define VBVAHOSTCMD_HDRSIZE                     (RT_OFFSETOF(VBVAHOSTCMD, body))

#pragma pack()

/* struct vbva_conf32::index */
#define VBOX_VBVA_CONF32_MONITOR_COUNT  0
#define VBOX_VBVA_CONF32_HOST_HEAP_SIZE 1
/** Returns VINF_SUCCESS if the host can report mode hints via VBVA.
 * Set value to VERR_NOT_SUPPORTED before calling. */
#define VBOX_VBVA_CONF32_MODE_HINT_REPORTING  2
/** Returns VINF_SUCCESS if the host can report guest cursor enabled status via
 * VBVA.  Set value to VERR_NOT_SUPPORTED before calling. */
#define VBOX_VBVA_CONF32_GUEST_CURSOR_REPORTING  3
/** Returns the currently available host cursor capabilities.  Available if
 * struct vbva_conf32::VBOX_VBVA_CONF32_GUEST_CURSOR_REPORTING returns success.
 * @see VMMDevReqMouseStatus::mouseFeatures. */
#define VBOX_VBVA_CONF32_CURSOR_CAPABILITIES  4
/** Returns the supported flags in VBVAINFOSCREEN::u8Flags. */
#define VBOX_VBVA_CONF32_SCREEN_FLAGS 5
/** Returns the max size of VBVA record. */
#define VBOX_VBVA_CONF32_MAX_RECORD_SIZE 6

typedef struct vbva_conf32 {
	u32 index;
	u32 value;
} vbva_conf32;

/** Reserved for historical reasons. */
#define VBOX_VBVA_CURSOR_CAPABILITY_RESERVED0  BIT(0)
/** Guest cursor capability: can the host show a hardware cursor at the host
 * pointer location? */
#define VBOX_VBVA_CURSOR_CAPABILITY_HARDWARE   BIT(1)
/** Reserved for historical reasons. */
#define VBOX_VBVA_CURSOR_CAPABILITY_RESERVED2  BIT(2)
/** Reserved for historical reasons.  Must always be unset. */
#define VBOX_VBVA_CURSOR_CAPABILITY_RESERVED3  BIT(3)
/** Reserved for historical reasons. */
#define VBOX_VBVA_CURSOR_CAPABILITY_RESERVED4  BIT(4)
/** Reserved for historical reasons. */
#define VBOX_VBVA_CURSOR_CAPABILITY_RESERVED5  BIT(5)

typedef struct VBVAINFOVIEW {
	/* Index of the screen, assigned by the guest. */
	u32 view_index;

	/* The screen offset in VRAM, the framebuffer starts here. */
	u32 u32ViewOffset;

	/* The size of the VRAM memory that can be used for the view. */
	u32 u32ViewSize;

	/* The recommended maximum size of the VRAM memory for the screen. */
	u32 u32MaxScreenSize;
} VBVAINFOVIEW;

typedef struct VBVAINFOHEAP {
	/* Absolute offset in VRAM of the start of the heap. */
	u32 u32HeapOffset;

	/* The size of the heap. */
	u32 u32HeapSize;

} VBVAINFOHEAP;

typedef struct VBVAFLUSH {
	u32 reserved;

} VBVAFLUSH;

typedef struct VBVACMDVBVASUBMIT {
	u32 reserved;
} VBVACMDVBVASUBMIT;

/* flush is requested because due to guest command buffer overflow */
#define VBVACMDVBVAFLUSH_F_GUEST_BUFFER_OVERFLOW 1

typedef struct VBVACMDVBVAFLUSH {
	u32 flags;
} VBVACMDVBVAFLUSH;


/* VBVAINFOSCREEN::u8Flags */
#define VBVA_SCREEN_F_NONE     0x0000
#define VBVA_SCREEN_F_ACTIVE   0x0001
/** The virtual monitor has been disabled by the guest and should be removed
 * by the host and ignored for purposes of pointer position calculation. */
#define VBVA_SCREEN_F_DISABLED 0x0002
/** The virtual monitor has been blanked by the guest and should be blacked
 * out by the host using width, height, etc values from the VBVAINFOSCREEN request. */
#define VBVA_SCREEN_F_BLANK    0x0004
/** The virtual monitor has been blanked by the guest and should be blacked
 * out by the host using the previous mode values for width. height, etc. */
#define VBVA_SCREEN_F_BLANK2   0x0008

typedef struct VBVAINFOSCREEN {
	/* Which view contains the screen. */
	u32 view_index;

	/* Physical X origin relative to the primary screen. */
	s32 origin_x;

	/* Physical Y origin relative to the primary screen. */
	s32 origin_y;

	/* Offset of visible framebuffer relative to the framebuffer start. */
	u32 start_offset;

	/* The scan line size in bytes. */
	u32 line_size;

	/* Width of the screen. */
	u32 width;

	/* Height of the screen. */
	u32 height;

	/* Color depth. */
	u16 bits_per_pixel;

	/* VBVA_SCREEN_F_* */
	u16 flags;
} VBVAINFOSCREEN;


/* VBVAENABLE::flags */
#define VBVA_F_NONE    0x00000000
#define VBVA_F_ENABLE  0x00000001
#define VBVA_F_DISABLE 0x00000002
/* extended VBVA to be used with WDDM */
#define VBVA_F_EXTENDED 0x00000004
/* vbva offset is absolute VRAM offset */
#define VBVA_F_ABSOFFSET 0x00000008

typedef struct VBVAENABLE {
	u32 flags;
	u32 offset;
	s32  result;
} VBVAENABLE;

typedef struct vbva_enable_ex {
	VBVAENABLE base;
	u32 screen_id;
} vbva_enable_ex;


typedef struct vbva_mouse_pointer_shape {
	/* The host result. */
	s32 result;

	/* VBOX_MOUSE_POINTER_* bit flags. */
	u32 flags;

	/* X coordinate of the hot spot. */
	u32 hot_x;

	/* Y coordinate of the hot spot. */
	u32 hot_y;

	/* Width of the pointer in pixels. */
	u32 width;

	/* Height of the pointer in scanlines. */
	u32 height;

	/* Pointer data.
	 *
	 ****
	 * The data consists of 1 bpp AND mask followed by 32 bpp XOR (color) mask.
	 *
	 * For pointers without alpha channel the XOR mask pixels are 32 bit values: (lsb)BGR0(msb).
	 * For pointers with alpha channel the XOR mask consists of (lsb)BGRA(msb) 32 bit values.
	 *
	 * Guest driver must create the AND mask for pointers with alpha channel, so if host does not
	 * support alpha, the pointer could be displayed as a normal color pointer. The AND mask can
	 * be constructed from alpha values. For example alpha value >= 0xf0 means bit 0 in the AND mask.
	 *
	 * The AND mask is 1 bpp bitmap with byte aligned scanlines. Size of AND mask,
	 * therefore, is cbAnd = (width + 7) / 8 * height. The padding bits at the
	 * end of any scanline are undefined.
	 *
	 * The XOR mask follows the AND mask on the next 4 bytes aligned offset:
	 * u8 *pXor = pAnd + (cbAnd + 3) & ~3
	 * Bytes in the gap between the AND and the XOR mask are undefined.
	 * XOR mask scanlines have no gap between them and size of XOR mask is:
	 * cXor = width * 4 * height.
	 ****
	 *
	 * Preallocate 4 bytes for accessing actual data as p->data.
	 */
	u8 data[4];

} vbva_mouse_pointer_shape;

/** @name struct vbva_mouse_pointer_shape::flags
 * @note The VBOX_MOUSE_POINTER_* flags are used in the guest video driver,
 *       values must be <= 0x8000 and must not be changed. (try make more sense
 *       of this, please).
 * @{
 */
/** pointer is visible */
#define VBOX_MOUSE_POINTER_VISIBLE (0x0001)
/** pointer has alpha channel */
#define VBOX_MOUSE_POINTER_ALPHA   (0x0002)
/** pointerData contains new pointer shape */
#define VBOX_MOUSE_POINTER_SHAPE   (0x0004)
/** @} */

/* the guest driver can handle asynch guest cmd completion by reading the command offset from io port */
#define VBVACAPS_COMPLETEGCMD_BY_IOREAD 0x00000001
/* the guest driver can handle video adapter IRQs */
#define VBVACAPS_IRQ                    0x00000002
/** The guest can read video mode hints sent via VBVA. */
#define VBVACAPS_VIDEO_MODE_HINTS       0x00000004
/** The guest can switch to a software cursor on demand. */
#define VBVACAPS_DISABLE_CURSOR_INTEGRATION 0x00000008
/** The guest does not depend on host handling the VBE registers. */
#define VBVACAPS_USE_VBVA_ONLY 0x00000010
typedef struct vbva_caps {
	s32 rc;
	u32 caps;
} vbva_caps;

/* makes graphics device generate IRQ on VSYNC */
#define VBVASCANLINECFG_ENABLE_VSYNC_IRQ        0x00000001
/* guest driver may request the current scanline */
#define VBVASCANLINECFG_ENABLE_SCANLINE_INFO    0x00000002
/* request the current refresh period, returned in u32RefreshPeriodMs */
#define VBVASCANLINECFG_QUERY_REFRESH_PERIOD    0x00000004
/* set new refresh period specified in u32RefreshPeriodMs.
 * if used with VBVASCANLINECFG_QUERY_REFRESH_PERIOD,
 * u32RefreshPeriodMs is set to the previous refresh period on return */
#define VBVASCANLINECFG_SET_REFRESH_PERIOD      0x00000008

typedef struct VBVASCANLINECFG {
	s32 rc;
	u32 flags;
	u32 u32RefreshPeriodMs;
	u32 reserved;
} VBVASCANLINECFG;

typedef struct VBVASCANLINEINFO {
	s32 rc;
	u32 screen_id;
	u32 u32InVBlank;
	u32 u32ScanLine;
} VBVASCANLINEINFO;

/** Query the most recent mode hints received from the host. */
typedef struct vbva_query_mode_hints {
	/** The maximum number of screens to return hints for. */
	u16 hints_queried_count;
	/** The size of the mode hint structures directly following this one. */
	u16 cbHintStructureGuest;
	/** The return code for the operation.  Initialise to VERR_NOT_SUPPORTED. */
	s32  rc;
} vbva_query_mode_hints;

/** Structure in which a mode hint is returned.  The guest allocates an array
 *  of these immediately after the struct vbva_query_mode_hints structure.  To accomodate
 *  future extensions, the struct vbva_query_mode_hints structure specifies the size of
 *  the struct vbva_modehint structures allocated by the guest, and the host only fills
 *  out structure elements which fit into that size.  The host should fill any
 *  unused members (e.g. dx, dy) or structure space on the end with ~0.  The
 *  whole structure can legally be set to ~0 to skip a screen. */
typedef struct vbva_modehint {
	u32 magic;
	u32 cx;
	u32 cy;
	u32 bpp;  /* Which has never been used... */
	u32 display;
	u32 dx;  /**< X offset into the virtual frame-buffer. */
	u32 dy;  /**< Y offset into the virtual frame-buffer. */
	u32 fEnabled;  /* Not flags.  Add new members for new flags. */
} vbva_modehint;

#define VBVAMODEHINT_MAGIC 0x0801add9u

/** Report the rectangle relative to which absolute pointer events should be
 *  expressed.  This information remains valid until the next VBVA resize event
 *  for any screen, at which time it is reset to the bounding rectangle of all
 *  virtual screens and must be re-set.
 *  @see VBVA_REPORT_INPUT_MAPPING. */
typedef struct vbva_report_input_mapping {
	s32 x;    /**< Upper left X co-ordinate relative to the first screen. */
	s32 y;    /**< Upper left Y co-ordinate relative to the first screen. */
	u32 cx;  /**< Rectangle width. */
	u32 cy;  /**< Rectangle height. */
} vbva_report_input_mapping;

/** Report the guest cursor position and query the host one.  The host may wish
 *  to use the guest information to re-position its own cursor, particularly
 *  when the cursor is captured and the guest does not support switching to a
 *  software cursor.  After every mode switch the guest must signal that it
 *  supports sending position information by sending an event with
 *  @a report_position set to false.
 *  @see VBVA_CURSOR_POSITION */
typedef struct vbva_cursor_position {
	u32 report_position;  /**< Are we reporting a position? */
	u32 x;                /**< Guest cursor X position */
	u32 y;                /**< Guest cursor Y position */
} vbva_cursor_position;

#pragma pack()

typedef uint64_t VBOXVIDEOOFFSET;

#define VBOXVIDEOOFFSET_VOID ((VBOXVIDEOOFFSET)~0)

#pragma pack(1)

/*
 * VBOXSHGSMI made on top HGSMI and allows receiving notifications
 * about G->H command completion
 */
/* SHGSMI command header */
typedef struct VBOXSHGSMIHEADER {
	uint64_t pvNext;    /*<- completion processing queue */
	u32 flags;    /*<- see VBOXSHGSMI_FLAG_XXX Flags */
	u32 cRefs;     /*<- command referece count */
	uint64_t u64Info1;  /*<- contents depends on the flags value */
	uint64_t u64Info2;  /*<- contents depends on the flags value */
} VBOXSHGSMIHEADER, *PVBOXSHGSMIHEADER;

typedef enum {
	VBOXVDMACMD_TYPE_UNDEFINED         = 0,
	VBOXVDMACMD_TYPE_DMA_PRESENT_BLT   = 1,
	VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER,
	VBOXVDMACMD_TYPE_DMA_BPB_FILL,
	VBOXVDMACMD_TYPE_DMA_PRESENT_SHADOW2PRIMARY,
	VBOXVDMACMD_TYPE_DMA_PRESENT_CLRFILL,
	VBOXVDMACMD_TYPE_DMA_PRESENT_FLIP,
	VBOXVDMACMD_TYPE_DMA_NOP,
	VBOXVDMACMD_TYPE_CHROMIUM_CMD, /* chromium cmd */
	VBOXVDMACMD_TYPE_DMA_BPB_TRANSFER_VRAMSYS,
	VBOXVDMACMD_TYPE_CHILD_STATUS_IRQ /* make the device notify child (monitor) state change IRQ */
} VBOXVDMACMD_TYPE;

#pragma pack()

/* the command processing was asynch, set by the host to indicate asynch command completion
 * must not be cleared once set, the command completion is performed by issuing a host->guest completion command
 * while keeping this flag unchanged */
#define VBOXSHGSMI_FLAG_HG_ASYNCH               0x00010000
#if 0
/* if set     - asynch completion is performed by issuing the event,
 * if cleared - asynch completion is performed by calling a callback */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_EVENT         0x00000001
#endif
/* issue interrupt on asynch completion, used for critical G->H commands,
 * i.e. for completion of which guest is waiting. */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_IRQ           0x00000002
/* guest does not do any op on completion of this command,
 * the host may copy the command and indicate that it does not need the command anymore
 * by not setting VBOXSHGSMI_FLAG_HG_ASYNCH */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_NOCOMPLETION  0x00000004
/* guest requires the command to be processed asynchronously,
 * not setting VBOXSHGSMI_FLAG_HG_ASYNCH by the host in this case is treated as command failure */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_FORCE         0x00000008
/* force IRQ on cmd completion */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_IRQ_FORCE     0x00000010
/* an IRQ-level callback is associated with the command */
#define VBOXSHGSMI_FLAG_GH_ASYNCH_CALLBACK_IRQ  0x00000020
/* guest expects this command to be completed synchronously */
#define VBOXSHGSMI_FLAG_GH_SYNCH                0x00000040


static inline u8  *
VBoxSHGSMIBufferData(const VBOXSHGSMIHEADER  *pHeader)
{
	return (u8  *)pHeader + sizeof(VBOXSHGSMIHEADER);
}

#define VBoxSHGSMIBufferHeaderSize() (sizeof(VBOXSHGSMIHEADER))

static inline VBOXSHGSMIHEADER  * VBoxSHGSMIBufferHeader(const void  *pvData)
{
	return (VBOXSHGSMIHEADER  *)((uintptr_t)pvData - sizeof(VBOXSHGSMIHEADER));
}

#ifdef VBOX_WITH_VDMA
# pragma pack(1)

/* VDMA - Video DMA */

/* VDMA Control API */
/* VBOXVDMA_CTL::flags */
typedef enum {
	VBOXVDMA_CTL_TYPE_NONE = 0,
	VBOXVDMA_CTL_TYPE_ENABLE,
	VBOXVDMA_CTL_TYPE_DISABLE,
	VBOXVDMA_CTL_TYPE_FLUSH,
	VBOXVDMA_CTL_TYPE_WATCHDOG,
	VBOXVDMA_CTL_TYPE_END
} VBOXVDMA_CTL_TYPE;

typedef struct VBOXVDMA_CTL {
	VBOXVDMA_CTL_TYPE enmCtl;
	u32 offset;
	s32  result;
} VBOXVDMA_CTL;

/* VBOXVDMACBUF_DR::phBuf specifies offset in VRAM */
#define VBOXVDMACBUF_FLAG_BUF_VRAM_OFFSET 0x00000001
/* command buffer follows the VBOXVDMACBUF_DR in VRAM, VBOXVDMACBUF_DR::phBuf is ignored */
#define VBOXVDMACBUF_FLAG_BUF_FOLLOWS_DR  0x00000002

/**
 * We can not submit the DMA command via VRAM since we do not have control over
 * DMA command buffer [de]allocation, i.e. we only control the buffer contents.
 * In other words the system may call one of our callbacks to fill a command buffer
 * with the necessary commands and then discard the buffer w/o any notification.
 *
 * We have only DMA command buffer physical address at submission time.
 *
 * so the only way is to */
typedef struct VBOXVDMACBUF_DR {
	u16 flags;
	u16 cbBuf;
	/* RT_SUCCESS()     - on success
	 * VERR_INTERRUPTED - on preemption
	 * VERR_xxx         - on error */
	s32  rc;
	union {
		uint64_t phBuf;
		VBOXVIDEOOFFSET offVramBuf;
	} Location;
	uint64_t aGuestData[7];
} VBOXVDMACBUF_DR, *PVBOXVDMACBUF_DR;

#define VBOXVDMACBUF_DR_TAIL(a_pCmd, a_TailType)       \
	( (a_TailType       *)( ((u8*)(a_pCmd)) + sizeof(VBOXVDMACBUF_DR)) )
#define VBOXVDMACBUF_DR_FROM_TAIL(a_pCmd) \
	( (VBOXVDMACBUF_DR  *)( ((u8*)(a_pCmd)) - sizeof(VBOXVDMACBUF_DR)) )

typedef struct VBOXVDMACMD {
	VBOXVDMACMD_TYPE enmType;
	u32 u32CmdSpecific;
} VBOXVDMACMD;

#define VBOXVDMACMD_HEADER_SIZE()                   sizeof(VBOXVDMACMD)
#define VBOXVDMACMD_SIZE_FROMBODYSIZE(_s)           ((u32)(VBOXVDMACMD_HEADER_SIZE() + (_s)))
#define VBOXVDMACMD_SIZE(_t)                        (VBOXVDMACMD_SIZE_FROMBODYSIZE(sizeof(_t)))
#define VBOXVDMACMD_BODY(a_pCmd, a_TypeBody)        \
	( (a_TypeBody   *)( ((u8 *)(a_pCmd)) + VBOXVDMACMD_HEADER_SIZE()) )
#define VBOXVDMACMD_BODY_SIZE(_s)                   ( (_s) - VBOXVDMACMD_HEADER_SIZE() )
#define VBOXVDMACMD_FROM_BODY(a_pBody)                \
	( (VBOXVDMACMD  *)( ((u8 *)(a_pBody)) - VBOXVDMACMD_HEADER_SIZE()) )
#define VBOXVDMACMD_BODY_FIELD_OFFSET(_ot, _t, _f)  ( (_ot)(uintptr_t)( VBOXVDMACMD_BODY(0, u8) + RT_UOFFSETOF_DYN(_t, _f) ) )

# pragma pack()
#endif /* #ifdef VBOX_WITH_VDMA */


#define VBOXVDMA_CHILD_STATUS_F_CONNECTED    0x01
#define VBOXVDMA_CHILD_STATUS_F_DISCONNECTED 0x02
#define VBOXVDMA_CHILD_STATUS_F_ROTATED      0x04

typedef struct VBOXVDMA_CHILD_STATUS {
	u32 iChild;
	u8  flags;
	u8  u8RotationAngle;
	u16 u16Reserved;
} VBOXVDMA_CHILD_STATUS, *PVBOXVDMA_CHILD_STATUS;

/* apply the aInfos are applied to all targets, the iTarget is ignored */
#define VBOXVDMACMD_CHILD_STATUS_IRQ_F_APPLY_TO_ALL 0x00000001

typedef struct VBOXVDMACMD_CHILD_STATUS_IRQ {
	u32 cInfos;
	u32 flags;
	VBOXVDMA_CHILD_STATUS aInfos[1];
} VBOXVDMACMD_CHILD_STATUS_IRQ, *PVBOXVDMACMD_CHILD_STATUS_IRQ;

#define VBOXCMDVBVA_SCREENMAP_SIZE(_elType) ((VBOX_VIDEO_MAX_SCREENS + sizeof (_elType) - 1) / sizeof (_elType))
#define VBOXCMDVBVA_SCREENMAP_DECL(_elType, _name) _elType _name[VBOXCMDVBVA_SCREENMAP_SIZE(_elType)]

#endif /* !VBOX_INCLUDED_Graphics_VBoxVideo_h */

