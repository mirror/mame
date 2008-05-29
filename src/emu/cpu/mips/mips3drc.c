/***************************************************************************

    mips3drc.c

    Universal machine language-based MIPS III/IV emulator.

    Copyright Aaron Giles
    Released for general non-commercial use under the MAME license
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Future improvements/changes:
    
    * Move context to a pointer to reduce context switch times

    * Add DRC option to flush PC before calling memory handlers

    * Constant tracking? (hasn't bought us much in the past)

    * Customized mapped/unmapped memory handlers
        - create 3 sets of handlers: cached, uncached, general
        - default to general
        - in general case, if cached use RECALL to point to cached code
        - (same for uncached)
        - in cached/uncached case, fall back to general case

***************************************************************************/

#include <stddef.h>
#include "cpuintrf.h"
#include "debugger.h"
#include "mips3com.h"
#include "mips3fe.h"
#include "deprecat.h"
#include "cpu/drcfe.h"
#include "cpu/drcuml.h"
#include "cpu/drcumlsh.h"

extern unsigned dasmmips3(char *buffer, unsigned pc, UINT32 op);



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define FORCE_C_BACKEND					(0)
#define LOG_UML							(0)
#define LOG_NATIVE						(0)

#define DISABLE_FAST_REGISTERS			(0)
#define SINGLE_INSTRUCTION_MODE			(0)

#define PRINTF_EXCEPTIONS				(0)
#define PRINTF_MMU						(0)

#define PROBE_ADDRESS					~0



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* map variables */
#define MAPVAR_PC						MVAR(0)
#define MAPVAR_CYCLES					MVAR(1)

/* modes */
#define MODE_KERNEL						0
#define MODE_SUPER						1
#define MODE_USER						2

/* size of the execution code cache */
#define CACHE_SIZE						(32 * 1024 * 1024)

/* compilation boundaries -- how far back/forward does the analysis extend? */
#define COMPILE_BACKWARDS_BYTES			128
#define COMPILE_FORWARDS_BYTES			512
#define COMPILE_MAX_INSTRUCTIONS		((COMPILE_BACKWARDS_BYTES/4) + (COMPILE_FORWARDS_BYTES/4))
#define COMPILE_MAX_SEQUENCE			64

/* exit codes */
#define EXECUTE_OUT_OF_CYCLES			0
#define EXECUTE_MISSING_CODE			1
#define EXECUTE_UNMAPPED_CODE			2
#define EXECUTE_RESET_CACHE				3



/***************************************************************************
    MACROS
***************************************************************************/

#ifdef LSB_FIRST
#define LOPTR(x)				((UINT32 *)(x))
#else
#define LOPTR(x)				((UINT32 *)(x) + 1)
#endif

#define R32(reg)				mips3->impstate->regmaplo[reg].type, mips3->impstate->regmaplo[reg].value
#define LO32					R32(REG_LO)
#define HI32					R32(REG_HI)
#define CPR032(reg)				MEM(LOPTR(&mips3->cpr[0][reg]))
#define CCR032(reg)				MEM(LOPTR(&mips3->ccr[0][reg]))
#define FPR32(reg)				MEM(((mips3->impstate->mode & 1) == 0) ? &((float *)&mips3->cpr[1][0])[reg] : (float *)&mips3->cpr[1][reg])
#define CCR132(reg)				MEM(LOPTR(&mips3->ccr[1][reg]))
#define CPR232(reg)				MEM(LOPTR(&mips3->cpr[2][reg]))
#define CCR232(reg)				MEM(LOPTR(&mips3->ccr[2][reg]))

#define R64(reg)				mips3->impstate->regmap[reg].type, mips3->impstate->regmap[reg].value
#define LO64					R64(REG_LO)
#define HI64					R64(REG_HI)
#define CPR064(reg)				MEM(&mips3->cpr[0][reg])
#define CCR064(reg)				MEM(&mips3->ccr[0][reg])
#define FPR64(reg)				MEM(((mips3->impstate->mode & 1) == 0) ? (double *)&mips3->cpr[1][(reg)/2] : (double *)&mips3->cpr[1][reg])
#define CCR164(reg)				MEM(&mips3->ccr[1][reg])
#define CPR264(reg)				MEM(&mips3->cpr[2][reg])
#define CCR264(reg)				MEM(&mips3->ccr[2][reg])

#define FCCMASK(which)			fcc_mask[(mips3->flavor < MIPS3_TYPE_MIPS_IV) ? 0 : ((which) & 7)]



/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* fast RAM info */
typedef struct _fast_ram_info fast_ram_info;
struct _fast_ram_info
{
	offs_t				start;						/* start of the RAM block */
	offs_t				end;						/* end of the RAM block */
	UINT8				readonly;					/* TRUE if read-only */
	void *				base;						/* base in memory where the RAM lives */
};


/* hotspot info */
typedef struct _hotspot_info hotspot_info;
struct _hotspot_info
{
	offs_t				pc;							/* PC to consider */
	UINT32				opcode;						/* required opcode at that PC */
	UINT32				cycles;						/* number of cycles to eat when hit */
};


/* internal compiler state */
typedef struct _compiler_state compiler_state;
struct _compiler_state
{
	UINT32				cycles;						/* accumulated cycles */
	UINT8				checkints;					/* need to check interrupts before next instruction */
	UINT8				checksoftints;				/* need to check software interrupts before next instruction */
	drcuml_codelabel	labelnum;					/* index for local labels */
};


/* MIPS3 registers */
struct _mips3imp_state
{
	/* core state */
	drccache *			cache;						/* pointer to the DRC code cache */
	drcuml_state *		drcuml;						/* DRC UML generator state */
	drcfe_state *		drcfe;						/* pointer to the DRC front-end state */
	UINT32				drcoptions;					/* configurable DRC options */

	/* internal stuff */
	UINT8				cache_dirty;				/* true if we need to flush the cache */

	/* parameters for subroutines */
	UINT64				numcycles;					/* return value from gettotalcycles */
	UINT32				mode;						/* current global mode */
	UINT32				arg0;						/* print_debug argument 1 */
	UINT32				arg1;						/* print_debug argument 2 */

	/* tables */
	UINT8				fpmode[4];					/* FPU mode table */
	UINT64				slt_table[32];				/* FLAGS table for slt */
	UINT64				sltu_table[32];				/* FLAGS table for sltu */
	UINT32				c_un_table[32];				/* FLAGS table for c_un */
	UINT32				c_eq_table[32];				/* FLAGS table for c_eq */
	UINT32				c_ueq_table[32];			/* FLAGS table for c_ueq */
	UINT32				c_olt_table[32];			/* FLAGS table for c_olt */
	UINT32				c_ult_table[32];			/* FLAGS table for c_ult */
	UINT32				c_ole_table[32];			/* FLAGS table for c_ole */
	UINT32				c_ule_table[32];			/* FLAGS table for c_ule */

	/* register mappings */
	drcuml_parameter	regmap[34];					/* parameter to register mappings for all 32 integer registers */
	drcuml_parameter	regmaplo[34];				/* parameter to register mappings for all 32 integer registers */

	/* subroutines */
	drcuml_codehandle *	entry;						/* entry point */
	drcuml_codehandle *	nocode;						/* nocode exception handler */
	drcuml_codehandle *	out_of_cycles;				/* out of cycles exception handler */
	drcuml_codehandle *	tlb_mismatch;				/* tlb mismatch handler */
	drcuml_codehandle *	read8[3];					/* read byte */
	drcuml_codehandle *	write8[3];					/* write byte */
	drcuml_codehandle *	read16[3];					/* read half */
	drcuml_codehandle *	write16[3];					/* write half */
	drcuml_codehandle *	read32[3];					/* read word */
	drcuml_codehandle *	read32mask[3];				/* read word masked */
	drcuml_codehandle *	write32[3];					/* write word */
	drcuml_codehandle *	write32mask[3];				/* write word masked */
	drcuml_codehandle *	read64[3];					/* read double */
	drcuml_codehandle *	read64mask[3];				/* read double masked */
	drcuml_codehandle *	write64[3];					/* write double */
	drcuml_codehandle *	write64mask[3];				/* write double masked */
	drcuml_codehandle *	exception[EXCEPTION_COUNT];	/* array of exception handlers */
	drcuml_codehandle *	exception_norecover[EXCEPTION_COUNT];	/* array of no-recover exception handlers */

	/* fast RAM */
	UINT32				fastram_select;
	fast_ram_info		fastram[MIPS3_MAX_FASTRAM];

	/* hotspots */
	UINT32				hotspot_select;
	hotspot_info		hotspot[MIPS3_MAX_HOTSPOTS];
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void code_flush_cache(drcuml_state *drcuml);
static void code_compile_block(drcuml_state *drcuml, UINT8 mode, offs_t pc);

static void cfunc_printf_exception(void *param);
static void cfunc_get_cycles(void *param);
static void cfunc_printf_probe(void *param);

static void static_generate_entry_point(drcuml_state *drcuml);
static void static_generate_nocode_handler(drcuml_state *drcuml);
static void static_generate_out_of_cycles(drcuml_state *drcuml);
static void static_generate_tlb_mismatch(drcuml_state *drcuml);
static void static_generate_exception(drcuml_state *drcuml, UINT8 exception, int recover, const char *name);
static void static_generate_memory_accessor(drcuml_state *drcuml, int mode, int size, int iswrite, int ismasked, const char *name, drcuml_codehandle **handleptr);

static void generate_update_mode(drcuml_block *block);
static void generate_update_cycles(drcuml_block *block, compiler_state *compiler, drcuml_ptype ptype, UINT64 pvalue, int allow_exception);
static void generate_checksum_block(drcuml_block *block, compiler_state *compiler, const opcode_desc *seqhead, const opcode_desc *seqlast);
static void generate_sequence_instruction(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static void generate_delay_slot_and_branch(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 linkreg);
static int generate_opcode(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_special(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_regimm(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_idt(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_set_cop0_reg(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 reg);
static int generate_get_cop0_reg(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 reg);
static int generate_cop0(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_cop1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_cop1x(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);

static void log_add_disasm_comment(drcuml_block *block, UINT32 pc, UINT32 op);
static const char *log_desc_flags_to_string(UINT32 flags);
static void log_register_list(drcuml_state *drcuml, const char *string, UINT64 gprmask, UINT64 fprmask);
static void log_opcode_desc(drcuml_state *drcuml, const opcode_desc *desclist, int indent);



/***************************************************************************
    PRIVATE GLOBAL VARIABLES
***************************************************************************/

static mips3_state *mips3;


/* bit indexes for various FCCs */
static const UINT32 fcc_mask[8] = { 1 << 23, 1 << 25, 1 << 26, 1 << 27, 1 << 28, 1 << 29, 1 << 30, 1 << 31 };

/* lookup table for FP modes */
static const UINT8 fpmode_source[4] =
{
	DRCUML_FMOD_ROUND,
	DRCUML_FMOD_TRUNC,
	DRCUML_FMOD_CEIL,
	DRCUML_FMOD_FLOOR
};

/* flag lookup table for SLT */
static const UINT64 slt_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	0,
	/* ...V. */	1,
	/* ...VC */	1,
	/* ..Z.. */	0,
	/* ..Z.C */	0,
	/* ..ZV. */	1,
	/* ..ZVC */	1,
	/* .S... */	1,
	/* .S..C */	1,
	/* .S.V. */	0,
	/* .S.VC */	0,
	/* .SZ.. */	1,
	/* .SZ.C */	1,
	/* .SZV. */	0,
	/* .SZVC */	0,
	/* U.... */	0,
	/* U...C */	0,
	/* U..V. */	1,
	/* U..VC */	1,
	/* U.Z.. */	0,
	/* U.Z.C */	0,
	/* U.ZV. */	1,
	/* U.ZVC */	1,
	/* US... */	1,
	/* US..C */	1,
	/* US.V. */	0,
	/* US.VC */	0,
	/* USZ.. */	1,
	/* USZ.C */	1,
	/* USZV. */	0,
	/* USZVC */	0
};

/* flag lookup table for SLTU */
static const UINT64 sltu_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	1,
	/* ...V. */	0,
	/* ...VC */	1,
	/* ..Z.. */	0,
	/* ..Z.C */	1,
	/* ..ZV. */	0,
	/* ..ZVC */	1,
	/* .S... */	0,
	/* .S..C */	1,
	/* .S.V. */	0,
	/* .S.VC */	1,
	/* .SZ.. */	0,
	/* .SZ.C */	1,
	/* .SZV. */	0,
	/* .SZVC */	1,
	/* U.... */	0,
	/* U...C */	1,
	/* U..V. */	0,
	/* U..VC */	1,
	/* U.Z.. */	0,
	/* U.Z.C */	1,
	/* U.ZV. */	0,
	/* U.ZVC */	1,
	/* US... */	0,
	/* US..C */	1,
	/* US.V. */	0,
	/* US.VC */	1,
	/* USZ.. */	0,
	/* USZ.C */	1,
	/* USZV. */	0,
	/* USZVC */	1
};

/* flag lookup table for C.UN */
static const UINT32 c_un_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	0,
	/* ...V. */	0,
	/* ...VC */	0,
	/* ..Z.. */	0,
	/* ..Z.C */	0,
	/* ..ZV. */	0,
	/* ..ZVC */	0,
	/* .S... */	0,
	/* .S..C */	0,
	/* .S.V. */	0,
	/* .S.VC */	0,
	/* .SZ.. */	0,
	/* .SZ.C */	0,
	/* .SZV. */	0,
	/* .SZVC */	0,
	/* U.... */	~0,
	/* U...C */	~0,
	/* U..V. */	~0,
	/* U..VC */	~0,
	/* U.Z.. */	~0,
	/* U.Z.C */	~0,
	/* U.ZV. */	~0,
	/* U.ZVC */	~0,
	/* US... */	~0,
	/* US..C */	~0,
	/* US.V. */	~0,
	/* US.VC */	~0,
	/* USZ.. */	~0,
	/* USZ.C */	~0,
	/* USZV. */	~0,
	/* USZVC */	~0
};

/* flag lookup table for C.EQ */
static const UINT32 c_eq_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	0,
	/* ...V. */	0,
	/* ...VC */	0,
	/* ..Z.. */	~0,
	/* ..Z.C */	~0,
	/* ..ZV. */	~0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	0,
	/* .S.V. */	0,
	/* .S.VC */	0,
	/* .SZ.. */	~0,
	/* .SZ.C */	~0,
	/* .SZV. */	~0,
	/* .SZVC */	~0,
	/* U.... */	0,
	/* U...C */	0,
	/* U..V. */	0,
	/* U..VC */	0,
	/* U.Z.. */	0,
	/* U.Z.C */	0,
	/* U.ZV. */	0,
	/* U.ZVC */	0,
	/* US... */	0,
	/* US..C */	0,
	/* US.V. */	0,
	/* US.VC */	0,
	/* USZ.. */	0,
	/* USZ.C */	0,
	/* USZV. */	0,
	/* USZVC */	0
};

/* flag lookup table for C.UEQ */
static const UINT32 c_ueq_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	0,
	/* ...V. */	0,
	/* ...VC */	0,
	/* ..Z.. */	~0,
	/* ..Z.C */	~0,
	/* ..ZV. */	~0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	0,
	/* .S.V. */	0,
	/* .S.VC */	0,
	/* .SZ.. */	~0,
	/* .SZ.C */	~0,
	/* .SZV. */	~0,
	/* .SZVC */	~0,
	/* U.... */	~0,
	/* U...C */	~0,
	/* U..V. */	~0,
	/* U..VC */	~0,
	/* U.Z.. */	~0,
	/* U.Z.C */	~0,
	/* U.ZV. */	~0,
	/* U.ZVC */	~0,
	/* US... */	~0,
	/* US..C */	~0,
	/* US.V. */	~0,
	/* US.VC */	~0,
	/* USZ.. */	~0,
	/* USZ.C */	~0,
	/* USZV. */	~0,
	/* USZVC */	~0
};

/* flag lookup table for C.OLT */
static const UINT32 c_olt_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	~0,
	/* ...V. */	0,
	/* ...VC */	~0,
	/* ..Z.. */	0,
	/* ..Z.C */	~0,
	/* ..ZV. */	0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	~0,
	/* .S.V. */	0,
	/* .S.VC */	~0,
	/* .SZ.. */	0,
	/* .SZ.C */	~0,
	/* .SZV. */	0,
	/* .SZVC */	~0,
	/* U.... */	0,
	/* U...C */	0,
	/* U..V. */	0,
	/* U..VC */	0,
	/* U.Z.. */	0,
	/* U.Z.C */	0,
	/* U.ZV. */	0,
	/* U.ZVC */	0,
	/* US... */	0,
	/* US..C */	0,
	/* US.V. */	0,
	/* US.VC */	0,
	/* USZ.. */	0,
	/* USZ.C */	0,
	/* USZV. */	0,
	/* USZVC */	0
};

/* flag lookup table for C.ULT */
static const UINT32 c_ult_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	~0,
	/* ...V. */	0,
	/* ...VC */	~0,
	/* ..Z.. */	0,
	/* ..Z.C */	~0,
	/* ..ZV. */	0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	~0,
	/* .S.V. */	0,
	/* .S.VC */	~0,
	/* .SZ.. */	0,
	/* .SZ.C */	~0,
	/* .SZV. */	0,
	/* .SZVC */	~0,
	/* U.... */	~0,
	/* U...C */	~0,
	/* U..V. */	~0,
	/* U..VC */	~0,
	/* U.Z.. */	~0,
	/* U.Z.C */	~0,
	/* U.ZV. */	~0,
	/* U.ZVC */	~0,
	/* US... */	~0,
	/* US..C */	~0,
	/* US.V. */	~0,
	/* US.VC */	~0,
	/* USZ.. */	~0,
	/* USZ.C */	~0,
	/* USZV. */	~0,
	/* USZVC */	~0
};

/* flag lookup table for C.OLE */
static const UINT32 c_ole_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	~0,
	/* ...V. */	0,
	/* ...VC */	~0,
	/* ..Z.. */	~0,
	/* ..Z.C */	~0,
	/* ..ZV. */	~0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	~0,
	/* .S.V. */	0,
	/* .S.VC */	~0,
	/* .SZ.. */	~0,
	/* .SZ.C */	~0,
	/* .SZV. */	~0,
	/* .SZVC */	~0,
	/* U.... */	0,
	/* U...C */	0,
	/* U..V. */	0,
	/* U..VC */	0,
	/* U.Z.. */	0,
	/* U.Z.C */	0,
	/* U.ZV. */	0,
	/* U.ZVC */	0,
	/* US... */	0,
	/* US..C */	0,
	/* US.V. */	0,
	/* US.VC */	0,
	/* USZ.. */	0,
	/* USZ.C */	0,
	/* USZV. */	0,
	/* USZVC */	0
};

/* flag lookup table for C.ULE */
static const UINT32 c_ule_table_source[32] =
{
	/* ..... */	0,
	/* ....C */	~0,
	/* ...V. */	0,
	/* ...VC */	~0,
	/* ..Z.. */	~0,
	/* ..Z.C */	~0,
	/* ..ZV. */	~0,
	/* ..ZVC */	~0,
	/* .S... */	0,
	/* .S..C */	~0,
	/* .S.V. */	0,
	/* .S.VC */	~0,
	/* .SZ.. */	~0,
	/* .SZ.C */	~0,
	/* .SZV. */	~0,
	/* .SZVC */	~0,
	/* U.... */	~0,
	/* U...C */	~0,
	/* U..V. */	~0,
	/* U..VC */	~0,
	/* U.Z.. */	~0,
	/* U.Z.C */	~0,
	/* U.ZV. */	~0,
	/* U.ZVC */	~0,
	/* US... */	~0,
	/* US..C */	~0,
	/* US.V. */	~0,
	/* US.VC */	~0,
	/* USZ.. */	~0,
	/* USZ.C */	~0,
	/* USZV. */	~0,
	/* USZVC */	~0
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    epc - compute the exception PC from a
    descriptor
-------------------------------------------------*/

INLINE UINT32 epc(const opcode_desc *desc)
{
	return (desc->flags & OPFLAG_IN_DELAY_SLOT) ? (desc->pc - 3) : desc->pc;
}


/*-------------------------------------------------
    alloc_handle - allocate a handle if not
    already allocated
-------------------------------------------------*/

INLINE void alloc_handle(drcuml_state *drcuml, drcuml_codehandle **handleptr, const char *name)
{
	if (*handleptr == NULL)
		*handleptr = drcuml_handle_alloc(drcuml, name);
}


/*-------------------------------------------------
    load_fast_iregs - load any fast integer
    registers
-------------------------------------------------*/

INLINE void load_fast_iregs(drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(mips3->impstate->regmap); regnum++)
		if (mips3->impstate->regmap[regnum].type == DRCUML_PTYPE_INT_REGISTER)
			UML_DMOV(block, IREG(mips3->impstate->regmap[regnum].value - DRCUML_REG_I0), MEM(&mips3->r[regnum]));
}


/*-------------------------------------------------
    save_fast_iregs - save any fast integer
    registers
-------------------------------------------------*/

INLINE void save_fast_iregs(drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(mips3->impstate->regmap); regnum++)
		if (mips3->impstate->regmap[regnum].type == DRCUML_PTYPE_INT_REGISTER)
			UML_DMOV(block, MEM(&mips3->r[regnum]), IREG(mips3->impstate->regmap[regnum].value - DRCUML_REG_I0));
}



/***************************************************************************
    CORE CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    mips3_init - initialize the processor
-------------------------------------------------*/

static void mips3_init(mips3_flavor flavor, int bigendian, int index, int clock, const mips3_config *config, int (*irqcallback)(int))
{
	drcfe_config feconfig =
	{
		COMPILE_BACKWARDS_BYTES,	/* code window start offset = startpc - window_start */
		COMPILE_FORWARDS_BYTES,		/* code window end offset = startpc + window_end */
		COMPILE_MAX_SEQUENCE,		/* maximum instructions to include in a sequence */
		mips3fe_describe			/* callback to describe a single instruction */
	};
	drccache *cache;
	drcbe_info beinfo;
	UINT32 flags = 0;
	size_t extrasize;
	void *extramem;
	int regnum;

	/* determine how much memory beyond the core size we need */
 	extrasize = mips3com_init(NULL, flavor, bigendian, index, clock, config, irqcallback, NULL);

	/* allocate enough space for the cache and the core */
	cache = drccache_alloc(CACHE_SIZE + sizeof(*mips3) + extrasize);
	if (cache == NULL)
		fatalerror("Unable to allocate cache of size %d", (UINT32)(CACHE_SIZE + sizeof(*mips3) + extrasize));

	/* allocate the core and the extra memory */
	mips3 = drccache_memory_alloc_near(cache, sizeof(*mips3));
	memset(mips3, 0, sizeof(*mips3));
	extramem = drccache_memory_alloc(cache, extrasize);
	memset(extramem, 0, extrasize);

	/* initialize the core */
	mips3com_init(mips3, flavor, bigendian, index, clock, config, irqcallback, extramem);

	/* allocate the implementation-specific state from the full cache */
	mips3->impstate = drccache_memory_alloc_near(cache, sizeof(*mips3->impstate));
	memset(mips3->impstate, 0, sizeof(*mips3->impstate));
	mips3->impstate->cache = cache;

	/* initialize the UML generator */
	if (FORCE_C_BACKEND)
		flags |= DRCUML_OPTION_USE_C;
	if (LOG_UML)
		flags |= DRCUML_OPTION_LOG_UML;
	if (LOG_NATIVE)
		flags |= DRCUML_OPTION_LOG_NATIVE;
	mips3->impstate->drcuml = drcuml_alloc(cache, flags, 8, 32, 2);
	if (mips3->impstate->drcuml == NULL)
		fatalerror("Error initializing the UML");

	/* initialize the front-end helper */
	if (SINGLE_INSTRUCTION_MODE)
		feconfig.max_sequence = 1;
	mips3->impstate->drcfe = drcfe_init(&feconfig, mips3);

	/* allocate memory for cache-local state and initialize it */
	memcpy(mips3->impstate->fpmode, fpmode_source, sizeof(fpmode_source));
	memcpy(mips3->impstate->slt_table, slt_table_source, sizeof(slt_table_source));
	memcpy(mips3->impstate->sltu_table, sltu_table_source, sizeof(sltu_table_source));
	memcpy(mips3->impstate->c_un_table, c_un_table_source, sizeof(c_un_table_source));
	memcpy(mips3->impstate->c_eq_table, c_eq_table_source, sizeof(c_eq_table_source));
	memcpy(mips3->impstate->c_ueq_table, c_ueq_table_source, sizeof(c_ueq_table_source));
	memcpy(mips3->impstate->c_olt_table, c_olt_table_source, sizeof(c_olt_table_source));
	memcpy(mips3->impstate->c_ult_table, c_ult_table_source, sizeof(c_ult_table_source));
	memcpy(mips3->impstate->c_ole_table, c_ole_table_source, sizeof(c_ole_table_source));
	memcpy(mips3->impstate->c_ule_table, c_ule_table_source, sizeof(c_ule_table_source));

	/* compute the register parameters */
	for (regnum = 0; regnum < 34; regnum++)
	{
		mips3->impstate->regmap[regnum].type = (regnum == 0) ? DRCUML_PTYPE_IMMEDIATE : DRCUML_PTYPE_MEMORY;
		mips3->impstate->regmap[regnum].value = (regnum == 0) ? 0 : (FPTR)&mips3->r[regnum];
		mips3->impstate->regmaplo[regnum].type = (regnum == 0) ? DRCUML_PTYPE_IMMEDIATE : DRCUML_PTYPE_MEMORY;
		mips3->impstate->regmaplo[regnum].value = (regnum == 0) ? 0 : (FPTR)LOPTR(&mips3->r[regnum]);
	}

	/* if we have registers to spare, assign r2, r3, r4 to leftovers */
	if (!DISABLE_FAST_REGISTERS)
	{
		drcuml_get_backend_info(mips3->impstate->drcuml, &beinfo);
		if (beinfo.direct_iregs > 4)
		{
			mips3->impstate->regmap[2].type = mips3->impstate->regmaplo[2].type = DRCUML_PTYPE_INT_REGISTER;
			mips3->impstate->regmap[2].value = mips3->impstate->regmaplo[2].value = DRCUML_REG_I4;
		}
		if (beinfo.direct_iregs > 5)
		{
			mips3->impstate->regmap[3].type = mips3->impstate->regmaplo[3].type = DRCUML_PTYPE_INT_REGISTER;
			mips3->impstate->regmap[3].value = mips3->impstate->regmaplo[3].value = DRCUML_REG_I5;
		}
		if (beinfo.direct_iregs > 6)
		{
			mips3->impstate->regmap[4].type = mips3->impstate->regmaplo[4].type = DRCUML_PTYPE_INT_REGISTER;
			mips3->impstate->regmap[4].value = mips3->impstate->regmaplo[4].value = DRCUML_REG_I6;
		}
	}

	/* mark the cache dirty so it is updated on next execute */
	mips3->impstate->cache_dirty = TRUE;
}


/*-------------------------------------------------
    mips3_reset - reset the processor
-------------------------------------------------*/

static void mips3_reset(void)
{
	/* reset the common code and mark the cache dirty */
	mips3com_reset(mips3);
	mips3->impstate->mode = (MODE_KERNEL << 1) | 0;
	mips3->impstate->cache_dirty = TRUE;
}


/*-------------------------------------------------
    mips3_execute - execute the CPU for the
    specified number of cycles
-------------------------------------------------*/

static int mips3_execute(int cycles)
{
	drcuml_state *drcuml = mips3->impstate->drcuml;
	int execute_result;

	/* reset the cache if dirty */
	if (mips3->impstate->cache_dirty)
		code_flush_cache(drcuml);
	mips3->impstate->cache_dirty = FALSE;

	/* execute */
	mips3->icount = cycles;
	do
	{
		/* run as much as we can */
		execute_result = drcuml_execute(drcuml, mips3->impstate->entry);

		/* if we need to recompile, do it */
		if (execute_result == EXECUTE_MISSING_CODE)
			code_compile_block(drcuml, mips3->impstate->mode, mips3->pc);
		else if (execute_result == EXECUTE_UNMAPPED_CODE)
			fatalerror("Attempted to execute unmapped code at PC=%08X\n", mips3->pc);
		else if (execute_result == EXECUTE_RESET_CACHE)
			code_flush_cache(drcuml);

	} while (execute_result != EXECUTE_OUT_OF_CYCLES);

	/* return the number of cycles executed */
	return cycles - mips3->icount;
}


/*-------------------------------------------------
    mips3_exit - cleanup from execution
-------------------------------------------------*/

static void mips3_exit(void)
{
	/* clean up the DRC */
	drcfe_exit(mips3->impstate->drcfe);
	drcuml_free(mips3->impstate->drcuml);
	drccache_free(mips3->impstate->cache);
}


/*-------------------------------------------------
    mips3_get_context - return a copy of the
    current context
-------------------------------------------------*/

static void mips3_get_context(void *dst)
{
	if (dst != NULL)
		*(mips3_state **)dst = mips3;
}


/*-------------------------------------------------
    mips3_set_context - copy the current context
    into the global state
-------------------------------------------------*/

static void mips3_set_context(void *src)
{
	if (src != NULL)
		mips3 = *(mips3_state **)src;
}


/*-------------------------------------------------
    mips3_translate - perform virtual-to-physical
    address translation
-------------------------------------------------*/

static int mips3_translate(int space, int intention, offs_t *address)
{
	return mips3com_translate_address(mips3, space, intention, address);
}


/*-------------------------------------------------
    mips3_dasm - disassemble an instruction
-------------------------------------------------*/

#ifdef ENABLE_DEBUGGER
static offs_t mips3_dasm(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram)
{
	return mips3com_dasm(mips3, buffer, pc, oprom, opram);
}
#endif /* ENABLE_DEBUGGER */



/*-------------------------------------------------
    mips3_set_info - set information about a given
    CPU instance
-------------------------------------------------*/

static void mips3_set_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_MIPS3_DRC_OPTIONS:				mips3->impstate->drcoptions = info->i;				break;

		case CPUINFO_INT_MIPS3_FASTRAM_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_FASTRAM) mips3->impstate->fastram_select = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_START:			mips3->impstate->fastram[mips3->impstate->fastram_select].start = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_END:				mips3->impstate->fastram[mips3->impstate->fastram_select].end = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_FASTRAM_READONLY:		mips3->impstate->fastram[mips3->impstate->fastram_select].readonly = info->i; mips3->impstate->cache_dirty = TRUE; break;

		case CPUINFO_INT_MIPS3_HOTSPOT_SELECT:			if (info->i >= 0 && info->i < MIPS3_MAX_HOTSPOTS) mips3->impstate->hotspot_select = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_PC:				mips3->impstate->hotspot[mips3->impstate->hotspot_select].pc = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_OPCODE:			mips3->impstate->hotspot[mips3->impstate->hotspot_select].opcode = info->i; mips3->impstate->cache_dirty = TRUE; break;
		case CPUINFO_INT_MIPS3_HOTSPOT_CYCLES:			mips3->impstate->hotspot[mips3->impstate->hotspot_select].cycles = info->i; mips3->impstate->cache_dirty = TRUE; break;

		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_MIPS3_FASTRAM_BASE:			mips3->impstate->fastram[mips3->impstate->fastram_select].base = info->p;	break;

		/* --- everything else is handled generically --- */
		default:										mips3com_set_info(mips3, state, info);	break;
	}
}


/*-------------------------------------------------
    mips3_get_info - return information about a 
    given CPU instance
-------------------------------------------------*/

static void mips3_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(mips3);				break;
		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = mips3_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = mips3_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = mips3_set_context;	break;
		case CPUINFO_PTR_INIT:							/* provided per-CPU */					break;
		case CPUINFO_PTR_RESET:							info->reset = mips3_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = mips3_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = mips3_execute;			break;
#ifdef ENABLE_DEBUGGER
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = mips3_dasm;			break;
#endif /* ENABLE_DEBUGGER */
		case CPUINFO_PTR_TRANSLATE:						info->translate = mips3_translate;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;

		/* --- everything else is handled generically --- */
		default:										mips3com_get_info(mips3, state, info); break;
	}
}



/***************************************************************************
    CACHE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    code_flush_cache - flush the cache and
    regenerate static code
-------------------------------------------------*/

static void code_flush_cache(drcuml_state *drcuml)
{
	int mode;

	/* empty the transient cache contents */
	drcuml_reset(drcuml);

	/* generate the entry point and out-of-cycles handlers */
	static_generate_entry_point(drcuml);
	static_generate_nocode_handler(drcuml);
	static_generate_out_of_cycles(drcuml);
	static_generate_tlb_mismatch(drcuml);

	/* append exception handlers for various types */
	static_generate_exception(drcuml, EXCEPTION_INTERRUPT,     TRUE,  "exception_interrupt");
	static_generate_exception(drcuml, EXCEPTION_INTERRUPT,     FALSE, "exception_interrupt_norecover");
	static_generate_exception(drcuml, EXCEPTION_TLBMOD,        TRUE,  "exception_tlbmod");
	static_generate_exception(drcuml, EXCEPTION_TLBLOAD,       TRUE,  "exception_tlbload");
	static_generate_exception(drcuml, EXCEPTION_TLBSTORE,      TRUE,  "exception_tlbstore");
	static_generate_exception(drcuml, EXCEPTION_TLBLOAD_FILL,  TRUE,  "exception_tlbload_fill");
	static_generate_exception(drcuml, EXCEPTION_TLBSTORE_FILL, TRUE,  "exception_tlbstore_fill");
	static_generate_exception(drcuml, EXCEPTION_ADDRLOAD,      TRUE,  "exception_addrload");
	static_generate_exception(drcuml, EXCEPTION_ADDRSTORE,     TRUE,  "exception_addrstore");
	static_generate_exception(drcuml, EXCEPTION_SYSCALL,       TRUE,  "exception_syscall");
	static_generate_exception(drcuml, EXCEPTION_BREAK,         TRUE,  "exception_break");
	static_generate_exception(drcuml, EXCEPTION_INVALIDOP,     TRUE,  "exception_invalidop");
	static_generate_exception(drcuml, EXCEPTION_BADCOP,        TRUE,  "exception_badcop");
	static_generate_exception(drcuml, EXCEPTION_OVERFLOW,      TRUE,  "exception_overflow");
	static_generate_exception(drcuml, EXCEPTION_TRAP,          TRUE,  "exception_trap");

	/* add subroutines for memory accesses */
	for (mode = 0; mode < 3; mode++)
	{
		static_generate_memory_accessor(drcuml, mode, 1, FALSE, FALSE, "read8",       &mips3->impstate->read8[mode]);
		static_generate_memory_accessor(drcuml, mode, 1, TRUE,  FALSE, "write8",      &mips3->impstate->write8[mode]);
		static_generate_memory_accessor(drcuml, mode, 2, FALSE, FALSE, "read16",      &mips3->impstate->read16[mode]);
		static_generate_memory_accessor(drcuml, mode, 2, TRUE,  FALSE, "write16",     &mips3->impstate->write16[mode]);
		static_generate_memory_accessor(drcuml, mode, 4, FALSE, FALSE, "read32",      &mips3->impstate->read32[mode]);
		static_generate_memory_accessor(drcuml, mode, 4, FALSE, TRUE,  "read32mask",  &mips3->impstate->read32mask[mode]);
		static_generate_memory_accessor(drcuml, mode, 4, TRUE,  FALSE, "write32",     &mips3->impstate->write32[mode]);
		static_generate_memory_accessor(drcuml, mode, 4, TRUE,  TRUE,  "write32mask", &mips3->impstate->write32mask[mode]);
		static_generate_memory_accessor(drcuml, mode, 8, FALSE, FALSE, "read64",      &mips3->impstate->read64[mode]);
		static_generate_memory_accessor(drcuml, mode, 8, FALSE, TRUE,  "read64mask",  &mips3->impstate->read64mask[mode]);
		static_generate_memory_accessor(drcuml, mode, 8, TRUE,  FALSE, "write64",     &mips3->impstate->write64[mode]);
		static_generate_memory_accessor(drcuml, mode, 8, TRUE,  TRUE,  "write64mask", &mips3->impstate->write64mask[mode]);
	}
}


/*-------------------------------------------------
    code_compile_block - compile a block of the
    given mode at the specified pc
-------------------------------------------------*/

static void code_compile_block(drcuml_state *drcuml, UINT8 mode, offs_t pc)
{
	compiler_state compiler = { 0 };
	const opcode_desc *seqhead, *seqlast;
	const opcode_desc *desclist;
	int override = FALSE;
	drcuml_block *block;
	jmp_buf errorbuf;

	/* get a description of this sequence */
	desclist = drcfe_describe_code(mips3->impstate->drcfe, pc);
	if (LOG_UML || LOG_NATIVE)
		log_opcode_desc(drcuml, desclist, 0);

	/* if we get an error back, flush the cache and try again */
	if (setjmp(errorbuf) != 0)
		code_flush_cache(drcuml);

	/* start the block */
	block = drcuml_block_begin(drcuml, 4096, &errorbuf);

	/* loop until we get through all instruction sequences */
	for (seqhead = desclist; seqhead != NULL; seqhead = seqlast->next)
	{
		const opcode_desc *curdesc;
		UINT32 nextpc;

		/* add a code log entry */
		if (LOG_UML)
			UML_COMMENT(block, "-------------------------");						// comment

		/* determine the last instruction in this sequence */
		for (seqlast = seqhead; seqlast != NULL; seqlast = seqlast->next)
			if (seqlast->flags & OPFLAG_END_SEQUENCE)
				break;
		assert(seqlast != NULL);

		/* if we don't have a hash for this mode/pc, or if we are overriding all, add one */
		if (override || !drcuml_hash_exists(drcuml, mode, seqhead->pc))
			UML_HASH(block, mode, seqhead->pc);										// hash    mode,pc

		/* if we already have a hash, and this is the first sequence, assume that we */
		/* are recompiling due to being out of sync and allow future overrides */
		else if (seqhead == desclist)
		{
			override = TRUE;
			UML_HASH(block, mode, seqhead->pc);										// hash    mode,pc
		}

		/* otherwise, redispatch to that fixed PC and skip the rest of the processing */
		else
		{
			UML_LABEL(block, seqhead->pc | 0x80000000);								// label   seqhead->pc | 0x80000000
			UML_HASHJMP(block, IMM(mips3->impstate->mode), IMM(seqhead->pc), mips3->impstate->nocode);
																					// hashjmp <mode>,seqhead->pc,nocode
			continue;
		}

		/* validate this code block if we're not pointing into ROM */
		if (memory_get_write_ptr(cpu_getactivecpu(), ADDRESS_SPACE_PROGRAM, seqhead->physpc) != NULL)
			generate_checksum_block(block, &compiler, seqhead, seqlast);

		/* label this instruction, if it may be jumped to locally */
		if (seqhead->flags & OPFLAG_IS_BRANCH_TARGET)
			UML_LABEL(block, seqhead->pc | 0x80000000);								// label   seqhead->pc | 0x80000000

		/* iterate over instructions in the sequence and compile them */
		for (curdesc = seqhead; curdesc != seqlast->next; curdesc = curdesc->next)
			generate_sequence_instruction(block, &compiler, curdesc);

		/* if we need to return to the start, do it */
		if (seqlast->flags & OPFLAG_RETURN_TO_START)
			nextpc = pc;

		/* otherwise we just go to the next instruction */
		else
			nextpc = seqlast->pc + (seqlast->skipslots + 1) * 4;

		/* count off cycles and go there */
		generate_update_cycles(block, &compiler, IMM(nextpc), TRUE);				// <subtract cycles>

		/* if the last instruction can change modes, use a variable mode; otherwise, assume the same mode */
		if (seqlast->flags & OPFLAG_CAN_CHANGE_MODES)
			UML_HASHJMP(block, MEM(&mips3->impstate->mode), IMM(nextpc), mips3->impstate->nocode);
																					// hashjmp <mode>,nextpc,nocode
		else if (seqlast->next == NULL || seqlast->next->pc != nextpc)
			UML_HASHJMP(block, IMM(mips3->impstate->mode), IMM(nextpc), mips3->impstate->nocode);	
																					// hashjmp <mode>,nextpc,nocode
	}

	/* end the sequence */
	drcuml_block_end(block);
}



/***************************************************************************
    C FUNCTION CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    cfunc_get_cycles - compute the total number
    of cycles executed so far
-------------------------------------------------*/

static void cfunc_get_cycles(void *param)
{
	UINT64 *dest = param;
	*dest = activecpu_gettotalcycles();
}


/*-------------------------------------------------
    cfunc_printf_exception - log any exceptions that
    aren't interrupts
-------------------------------------------------*/

static void cfunc_printf_exception(void *param)
{
	printf("Exception: EPC=%08X Cause=%08X BadVAddr=%08X Jmp=%08X\n", (UINT32)mips3->cpr[0][COP0_EPC], (UINT32)mips3->cpr[0][COP0_Cause], (UINT32)mips3->cpr[0][COP0_BadVAddr], mips3->pc);
	cfunc_printf_probe((void *)(FPTR)mips3->cpr[0][COP0_EPC]);
}


/*-------------------------------------------------
    cfunc_printf_debug - generic printf for
    debugging
-------------------------------------------------*/

static void cfunc_printf_debug(void *param)
{
	const char *format = param;
	printf(format, mips3->impstate->arg0, mips3->impstate->arg1);
}


/*-------------------------------------------------
    cfunc_printf_probe - print the current CPU
    state and return
-------------------------------------------------*/

static void cfunc_printf_probe(void *param)
{
	UINT32 pc = (UINT32)(FPTR)param;

	printf(" PC=%08X          r1=%08X%08X  r2=%08X%08X  r3=%08X%08X\n",
		pc,
		(UINT32)(mips3->r[1] >> 32), (UINT32)mips3->r[1],
		(UINT32)(mips3->r[2] >> 32), (UINT32)mips3->r[2],
		(UINT32)(mips3->r[3] >> 32), (UINT32)mips3->r[3]);
	printf(" r4=%08X%08X  r5=%08X%08X  r6=%08X%08X  r7=%08X%08X\n",
		(UINT32)(mips3->r[4] >> 32), (UINT32)mips3->r[4],
		(UINT32)(mips3->r[5] >> 32), (UINT32)mips3->r[5],
		(UINT32)(mips3->r[6] >> 32), (UINT32)mips3->r[6],
		(UINT32)(mips3->r[7] >> 32), (UINT32)mips3->r[7]);
	printf(" r8=%08X%08X  r9=%08X%08X r10=%08X%08X r11=%08X%08X\n",
		(UINT32)(mips3->r[8] >> 32), (UINT32)mips3->r[8],
		(UINT32)(mips3->r[9] >> 32), (UINT32)mips3->r[9],
		(UINT32)(mips3->r[10] >> 32), (UINT32)mips3->r[10],
		(UINT32)(mips3->r[11] >> 32), (UINT32)mips3->r[11]);
	printf("r12=%08X%08X r13=%08X%08X r14=%08X%08X r15=%08X%08X\n",
		(UINT32)(mips3->r[12] >> 32), (UINT32)mips3->r[12],
		(UINT32)(mips3->r[13] >> 32), (UINT32)mips3->r[13],
		(UINT32)(mips3->r[14] >> 32), (UINT32)mips3->r[14],
		(UINT32)(mips3->r[15] >> 32), (UINT32)mips3->r[15]);
	printf("r16=%08X%08X r17=%08X%08X r18=%08X%08X r19=%08X%08X\n",
		(UINT32)(mips3->r[16] >> 32), (UINT32)mips3->r[16],
		(UINT32)(mips3->r[17] >> 32), (UINT32)mips3->r[17],
		(UINT32)(mips3->r[18] >> 32), (UINT32)mips3->r[18],
		(UINT32)(mips3->r[19] >> 32), (UINT32)mips3->r[19]);
	printf("r20=%08X%08X r21=%08X%08X r22=%08X%08X r23=%08X%08X\n",
		(UINT32)(mips3->r[20] >> 32), (UINT32)mips3->r[20],
		(UINT32)(mips3->r[21] >> 32), (UINT32)mips3->r[21],
		(UINT32)(mips3->r[22] >> 32), (UINT32)mips3->r[22],
		(UINT32)(mips3->r[23] >> 32), (UINT32)mips3->r[23]);
	printf("r24=%08X%08X r25=%08X%08X r26=%08X%08X r27=%08X%08X\n",
		(UINT32)(mips3->r[24] >> 32), (UINT32)mips3->r[24],
		(UINT32)(mips3->r[25] >> 32), (UINT32)mips3->r[25],
		(UINT32)(mips3->r[26] >> 32), (UINT32)mips3->r[26],
		(UINT32)(mips3->r[27] >> 32), (UINT32)mips3->r[27]);
	printf("r28=%08X%08X r29=%08X%08X r30=%08X%08X r31=%08X%08X\n",
		(UINT32)(mips3->r[28] >> 32), (UINT32)mips3->r[28],
		(UINT32)(mips3->r[29] >> 32), (UINT32)mips3->r[29],
		(UINT32)(mips3->r[30] >> 32), (UINT32)mips3->r[30],
		(UINT32)(mips3->r[31] >> 32), (UINT32)mips3->r[31]);
	printf(" hi=%08X%08X  lo=%08X%08X\n",
		(UINT32)(mips3->r[REG_HI] >> 32), (UINT32)mips3->r[REG_HI],
		(UINT32)(mips3->r[REG_LO] >> 32), (UINT32)mips3->r[REG_LO]);
}


/*-------------------------------------------------
    cfunc_unimplemented - handler for
    unimplemented opcdes
-------------------------------------------------*/

static void cfunc_unimplemented(void *param)
{
	UINT32 opcode = (FPTR)param;
	fatalerror("PC=%08X: Unimplemented op %08X (%02X,%02X)", mips3->pc, opcode, opcode >> 26, opcode & 0x3f);
}



/***************************************************************************
    STATIC CODEGEN
***************************************************************************/

/*-------------------------------------------------
    static_generate_entry_point - generate a
    static entry point
-------------------------------------------------*/

static void static_generate_entry_point(drcuml_state *drcuml)
{
	drcuml_codelabel skip = 1;
	drcuml_block *block;
	jmp_buf errorbuf;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_entry_point");

	/* begin generating */
	block = drcuml_block_begin(drcuml, 20, &errorbuf);

	/* forward references */
	alloc_handle(drcuml, &mips3->impstate->exception_norecover[EXCEPTION_INTERRUPT], "interrupt_norecover");
	alloc_handle(drcuml, &mips3->impstate->nocode, "nocode");

	alloc_handle(drcuml, &mips3->impstate->entry, "entry");
	UML_HANDLE(block, mips3->impstate->entry);										// handle  entry

	/* reset the FPU mode */
	UML_AND(block, IREG(0), CCR132(31), IMM(3));									// and     i0,ccr1[31],3
	UML_LOAD1U(block, IREG(0), &mips3->impstate->fpmode[0], IREG(0));				// load1u  i0,fpmode,i0
	UML_SETFMOD(block, IREG(0));													// setfmod i0

	/* load fast integer registers */
	load_fast_iregs(block);

	/* check for interrupts */
	UML_AND(block, IREG(0), CPR032(COP0_Cause), CPR032(COP0_Status));				// and     i0,[Cause],[Status]
	UML_ANDf(block, IREG(0), IREG(0), IMM(0xfc00), FLAGS_Z);						// and     i0,i0,0xfc00,Z
	UML_JMPc(block, IF_Z, skip);													// jmp     skip,Z
	UML_TEST(block, CPR032(COP0_Status), IMM(SR_IE));								// test    [Status],SR_IE
	UML_JMPc(block, IF_Z, skip);													// jmp     skip,Z
	UML_TEST(block, CPR032(COP0_Status), IMM(SR_EXL | SR_ERL));						// test    [Status],SR_EXL | SR_ERL
	UML_JMPc(block, IF_NZ, skip);													// jmp     skip,NZ
	UML_MOV(block, IREG(0), MEM(&mips3->pc));										// mov     i0,pc
	UML_MOV(block, IREG(1), IMM(0));												// mov     i1,0
	UML_CALLH(block, mips3->impstate->exception_norecover[EXCEPTION_INTERRUPT]);	// callh   exception_norecover
	UML_LABEL(block, skip);														// skip:

	/* generate a hash jump via the current mode and PC */
	UML_HASHJMP(block, MEM(&mips3->impstate->mode), MEM(&mips3->pc), mips3->impstate->nocode);
																					// hashjmp <mode>,<pc>,nocode
	drcuml_block_end(block);
}


/*-------------------------------------------------
    static_generate_nocode_handler - generate an
    exception handler for "out of code"
-------------------------------------------------*/

static void static_generate_nocode_handler(drcuml_state *drcuml)
{
	drcuml_block *block;
	jmp_buf errorbuf;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_nocode_handler");

	/* begin generating */
	block = drcuml_block_begin(drcuml, 10, &errorbuf);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(drcuml, &mips3->impstate->nocode, "nocode");
	UML_HANDLE(block, mips3->impstate->nocode);										// handle  nocode
	UML_GETEXP(block, IREG(0));														// getexp  i0
	UML_MOV(block, MEM(&mips3->pc), IREG(0));										// mov     [pc],i0
	save_fast_iregs(block);
	UML_EXIT(block, IMM(EXECUTE_MISSING_CODE));										// exit    EXECUTE_MISSING_CODE

	drcuml_block_end(block);
}


/*-------------------------------------------------
    static_generate_out_of_cycles - generate an
    out of cycles exception handler
-------------------------------------------------*/

static void static_generate_out_of_cycles(drcuml_state *drcuml)
{
	drcuml_block *block;
	jmp_buf errorbuf;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_out_of_cycles");

	/* begin generating */
	block = drcuml_block_begin(drcuml, 10, &errorbuf);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(drcuml, &mips3->impstate->out_of_cycles, "out_of_cycles");
	UML_HANDLE(block, mips3->impstate->out_of_cycles);								// handle  out_of_cycles
	UML_GETEXP(block, IREG(0));														// getexp  i0
	UML_MOV(block, MEM(&mips3->pc), IREG(0));										// mov     <pc>,i0
	save_fast_iregs(block);
	UML_EXIT(block, IMM(EXECUTE_OUT_OF_CYCLES));									// exit    EXECUTE_OUT_OF_CYCLES

	drcuml_block_end(block);
}


/*-------------------------------------------------
    static_generate_tlb_mismatch - generate a
    TLB mismatch handler
-------------------------------------------------*/

static void static_generate_tlb_mismatch(drcuml_state *drcuml)
{
	drcuml_block *block;
	jmp_buf errorbuf;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_tlb_mismatch");

	/* forward references */
	alloc_handle(drcuml, &mips3->impstate->exception[EXCEPTION_TLBLOAD], "exception_tlbload");
	alloc_handle(drcuml, &mips3->impstate->exception[EXCEPTION_TLBLOAD_FILL], "exception_tlbload_fill");

	/* begin generating */
	block = drcuml_block_begin(drcuml, 20, &errorbuf);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(drcuml, &mips3->impstate->tlb_mismatch, "tlb_mismatch");
	UML_HANDLE(block, mips3->impstate->tlb_mismatch);								// handle  tlb_mismatch
	UML_RECOVER(block, IREG(0), MAPVAR_PC);											// recover i0,PC
	UML_MOV(block, MEM(&mips3->pc), IREG(0));										// mov     <pc>,i0
	UML_SHR(block, IREG(1), IREG(0), IMM(12));										// shr     i1,i0,12
	UML_LOAD4(block, IREG(1), mips3->tlb_table, IREG(1));							// load4   i1,[tlb_table],i1
	if (PRINTF_MMU)
	{
		UML_MOV(block, MEM(&mips3->impstate->arg0), IREG(0));						// mov     [arg0],i0
		UML_MOV(block, MEM(&mips3->impstate->arg1), IREG(1));						// mov     [arg1],i1
		UML_CALLC(block, cfunc_printf_debug, "TLB mismatch @ %08X (ent=%08X)\n");	// callc   printf_debug
	}
	UML_TEST(block, IREG(1), IMM(TLB_VALID));										// test    i1,TLB_VALID
	UML_JMPc(block, IF_NZ, 1);														// jmp     1,nz
	UML_TEST(block, IREG(1), IMM(TLB_PRESENT));										// test    i1,TLB_PRESENT
	UML_EXHc(block, IF_NZ, mips3->impstate->exception[EXCEPTION_TLBLOAD], IREG(0));	// exh     exception[TLBLOAD],i0,nz
	UML_EXH(block, mips3->impstate->exception[EXCEPTION_TLBLOAD_FILL], IREG(0));	// exh     exception[TLBLOAD_FILL],i0
	UML_LABEL(block, 1);														// 1:
	save_fast_iregs(block);
	UML_EXIT(block, IMM(EXECUTE_MISSING_CODE));										// exit    EXECUTE_MISSING_CODE

	drcuml_block_end(block);
}


/*-------------------------------------------------
    static_generate_exception - generate a static
    exception handler
-------------------------------------------------*/

static void static_generate_exception(drcuml_state *drcuml, UINT8 exception, int recover, const char *name)
{
	drcuml_codehandle **exception_handle = recover ? &mips3->impstate->exception[exception] : &mips3->impstate->exception_norecover[exception];
	UINT32 offset = 0x180;
	drcuml_codelabel next = 1;
	drcuml_block *block;
	jmp_buf errorbuf;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_exception");

	/* translate our fake fill exceptions into real exceptions */
	if (exception == EXCEPTION_TLBLOAD_FILL || exception == EXCEPTION_TLBSTORE_FILL)
	{
		offset = 0x000;
		exception = (exception - EXCEPTION_TLBLOAD_FILL) + EXCEPTION_TLBLOAD;
	}

	/* begin generating */
	block = drcuml_block_begin(drcuml, 1024, &errorbuf);

	/* add a global entry for this */
	alloc_handle(drcuml, exception_handle, name);
	UML_HANDLE(block, *exception_handle);											// handle  name

	/* exception parameter is expected to be the fault address in this case */
	if (exception == EXCEPTION_TLBLOAD || exception == EXCEPTION_TLBSTORE || exception == EXCEPTION_ADDRLOAD || exception == EXCEPTION_ADDRSTORE)
	{
		/* set BadVAddr to the fault address */
		UML_GETEXP(block, IREG(0));													// getexp  i0
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_EXL));							// test    [Status],SR_EXL
		UML_MOVc(block, IF_Z, CPR032(COP0_BadVAddr), IREG(0));						// mov     [BadVAddr],i0,Z
	}

	if (exception == EXCEPTION_TLBLOAD || exception == EXCEPTION_TLBSTORE)
	{
		/* set the upper bits of EntryHi and the lower bits of Context to the fault page */
		UML_INSERT(block, CPR032(COP0_EntryHi), IREG(0), IMM(0), IMM(0xffffe000));	// insert  [EntryHi],i0,0,0xffffe000
		UML_INSERT(block, CPR032(COP0_Context), IREG(0), IMM(32-9), IMM(0x7fffff));	// insert  [Context],i0,32-9,0x7fffff
	}

	/* set the EPC and Cause registers */
	if (recover)
	{
		UML_RECOVER(block, IREG(0), MAPVAR_PC);										// recover i0,PC
		UML_RECOVER(block, IREG(1), MAPVAR_CYCLES);									// recover i1,CYCLES
	}

	UML_AND(block, IREG(2), CPR032(COP0_Cause), IMM(~0x800000ff));					// and     i2,[Cause],~0x800000ff
	UML_TEST(block, IREG(0), IMM(1));												// test    i0,1
	UML_JMPc(block, IF_Z, next);													// jz      <next>
	UML_OR(block, IREG(2), IREG(2), IMM(0x80000000));								// or      i2,i2,0x80000000
	UML_SUB(block, IREG(0), IREG(0), IMM(1));										// sub     i0,i0,1
	UML_LABEL(block, next);														// <next>:
	UML_TEST(block, CPR032(COP0_Status), IMM(SR_EXL));								// test    [Status],SR_EXL
	UML_MOVc(block, IF_Z, CPR032(COP0_EPC), IREG(0));								// mov     [EPC],i0,Z
	UML_OR(block, CPR032(COP0_Cause), IREG(2), IMM(exception << 2));				// or      [Cause],i2,exception << 2

	/* set EXL in the SR */
	UML_OR(block, IREG(0), CPR032(COP0_Status), IMM(SR_EXL));						// or      i0,[Status],SR_EXL
	UML_MOV(block, CPR032(COP0_Status), IREG(0));									// mov     [Status],i0
	generate_update_mode(block);

	/* optionally print exceptions */
	if ((PRINTF_EXCEPTIONS && exception != EXCEPTION_INTERRUPT && exception != EXCEPTION_SYSCALL) ||
		(PRINTF_MMU && (exception == EXCEPTION_TLBLOAD || exception == EXCEPTION_TLBSTORE)))
		UML_CALLC(block, cfunc_printf_exception, NULL);								// callc   cfunc_printf_exception,NULL

	/* choose our target PC */
	UML_MOV(block, IREG(0), IMM(0xbfc00200 + offset));								// mov     i0,0xbfc00200 + offset
	UML_TEST(block, IREG(1), IMM(SR_BEV));											// test    i1,SR_BEV
	UML_MOVc(block, IF_Z, IREG(0), IMM(0x80000000 + offset));						// mov     i0,0x80000000 + offset,z

	/* adjust cycles */
	UML_SUBf(block, MEM(&mips3->icount), MEM(&mips3->icount), IREG(1), FLAGS_S); 	// sub icount,icount,cycles,S
	UML_EXHc(block, IF_S, mips3->impstate->out_of_cycles, IREG(0));					// exh     out_of_cycles,i0

	UML_HASHJMP(block, MEM(&mips3->impstate->mode), IREG(0), mips3->impstate->nocode);// hashjmp <mode>,i0,nocode

	drcuml_block_end(block);
}


/*------------------------------------------------------------------
    static_generate_memory_accessor
------------------------------------------------------------------*/

static void static_generate_memory_accessor(drcuml_state *drcuml, int mode, int size, int iswrite, int ismasked, const char *name, drcuml_codehandle **handleptr)
{
	/* on entry, address is in I0; data for writes is in I1; mask for accesses is in I2 */
	/* on exit, read result is in I0 */
	/* routine trashes I0-I3 */
	drcuml_codehandle *exception_tlb = mips3->impstate->exception[iswrite ? EXCEPTION_TLBSTORE : EXCEPTION_TLBLOAD];
	drcuml_codehandle *exception_tlbfill = mips3->impstate->exception[iswrite ? EXCEPTION_TLBSTORE_FILL : EXCEPTION_TLBLOAD_FILL];
	drcuml_codehandle *exception_addrerr = mips3->impstate->exception[iswrite ? EXCEPTION_ADDRSTORE : EXCEPTION_ADDRLOAD];
	drcuml_block *block;
	jmp_buf errorbuf;
	int tlbmiss = 0;
	int label = 1;
	int ramnum;

	/* if we get an error back, we're screwed */
	if (setjmp(errorbuf) != 0)
		fatalerror("Unrecoverable error in static_generate_exception");

	/* begin generating */
	block = drcuml_block_begin(drcuml, 1024, &errorbuf);

	/* add a global entry for this */
	alloc_handle(drcuml, handleptr, name);
	UML_HANDLE(block, *handleptr);													// handle  *handleptr

	/* user mode? generate address exception if top bit is set */
	if (mode == MODE_USER)
	{
		UML_TEST(block, IREG(0), IMM(0x80000000));									// test    i0,0x80000000
		UML_EXHc(block, IF_NZ, exception_addrerr, IREG(0));							// exh     addrerr,i0,nz
	}

	/* supervisor mode? generate address exception if not in user space or in $C0000000-DFFFFFFF */
	if (mode == MODE_SUPER)
	{
		int addrok;
		UML_TEST(block, IREG(0), IMM(0x80000000));									// test    i0,0x80000000
		UML_JMPc(block, IF_Z, addrok = label++);									// jz      addrok
		UML_SHR(block, IREG(3), IREG(0), IMM(29));									// shr     i3,i0,29
		UML_CMP(block, IREG(3), IMM(6));											// cmp     i3,6
		UML_EXHc(block, IF_NE, exception_addrerr, IREG(0));							// exh     addrerr,i0,ne
		UML_LABEL(block, addrok);												// addrok:
	}

	/* general case: assume paging and perform a translation */
	UML_SHR(block, IREG(3), IREG(0), IMM(12));										// shr     i3,i0,12
	UML_LOAD4(block, IREG(3), mips3->tlb_table, IREG(3));							// load4   i3,[tlb_table],i3
	UML_TEST(block, IREG(3), IMM(iswrite ? TLB_DIRTY : TLB_VALID));					// test    i3,iswrite ? TLB_DIRTY : TLB_VALID
	UML_JMPc(block, IF_Z, tlbmiss = label++);										// jmp     tlbmiss,z
	UML_INSERT(block, IREG(0), IREG(3), IMM(0), IMM(0xfffff000));					// insert  i0,i3,0,0xfffff000

	for (ramnum = 0; ramnum < MIPS3_MAX_FASTRAM; ramnum++)
		if (!Machine->debug_mode && mips3->impstate->fastram[ramnum].base != NULL && (!iswrite || !mips3->impstate->fastram[ramnum].readonly))
		{
			void *fastbase = (UINT8 *)mips3->impstate->fastram[ramnum].base - mips3->impstate->fastram[ramnum].start;
			UINT32 skip = label++;
			if (mips3->impstate->fastram[ramnum].end != 0xffffffff)
			{
				UML_CMP(block, IREG(0), IMM(mips3->impstate->fastram[ramnum].end));	// cmp     i0,end
				UML_JMPc(block, IF_A, skip);										// ja      skip
			}
			if (mips3->impstate->fastram[ramnum].start != 0x00000000)
			{
				UML_CMP(block, IREG(0), IMM(mips3->impstate->fastram[ramnum].start));// cmp     i0,fastram_start
				UML_JMPc(block, IF_B, skip);										// jb      skip
			}

			if (!iswrite)
			{
				if (size == 1)
				{
					UML_XOR(block, IREG(0), IREG(0), IMM(mips3->bigendian ? BYTE4_XOR_BE(0) : BYTE4_XOR_LE(0)));
																					// xor     i0,i0,bytexor
					UML_LOAD1U(block, IREG(0), fastbase, IREG(0));					// load1u  i0,fastbase,i0
				}
				else if (size == 2)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(1));						// shr     i0,i0,1
					UML_XOR(block, IREG(0), IREG(0), IMM(mips3->bigendian ? BYTE_XOR_BE(0) : BYTE_XOR_LE(0)));
																					// xor     i0,i0,bytexor
					UML_LOAD2U(block, IREG(0), fastbase, IREG(0));					// load2u  i0,fastbase,i0
				}
				else if (size == 4)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(2));						// shr     i0,i0,2
					UML_LOAD4(block, IREG(0), fastbase, IREG(0));					// load4   i0,fastbase,i0
				}
				else if (size == 8)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(3));						// shr     i0,i0,3
					UML_DLOAD8(block, IREG(0), fastbase, IREG(0));					// dload8  i0,fastbase,i0
					UML_DROR(block, IREG(0), IREG(0), IMM(32 * (mips3->bigendian ? BYTE_XOR_BE(0) : BYTE_XOR_LE(0))));
																					// dror    i0,i0,32*bytexor
				}
				UML_RET(block);														// ret
			}
			else
			{
				if (size == 1)
				{
					UML_XOR(block, IREG(0), IREG(0), IMM(mips3->bigendian ? BYTE4_XOR_BE(0) : BYTE4_XOR_LE(0)));
																					// xor     i0,i0,bytexor
					UML_STORE1(block, fastbase, IREG(0), IREG(1));					// store1  fastbase,i0,i1
				}
				else if (size == 2)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(1));						// shr     i0,i0,1
					UML_XOR(block, IREG(0), IREG(0), IMM(mips3->bigendian ? BYTE_XOR_BE(0) : BYTE_XOR_LE(0)));
																					// xor     i0,i0,bytexor
					UML_STORE2(block, fastbase, IREG(0), IREG(1));					// store2  fastbase,i0,i1
				}
				else if (size == 4)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(2));						// shr     i0,i0,2
					if (ismasked)
					{
						UML_LOAD4(block, IREG(3), fastbase, IREG(0));				// load4   i3,fastbase,i0
						UML_INSERT(block, IREG(3), IREG(1), IMM(0), IREG(2));		// insert  i3,i1,0,i2
						UML_STORE4(block, fastbase, IREG(0), IREG(3));				// store4  fastbase,i0,i3
					}
					else
						UML_STORE4(block, fastbase, IREG(0), IREG(1));				// store4  fastbase,i0,i1
				}
				else if (size == 8)
				{
					UML_SHR(block, IREG(0), IREG(0), IMM(3));						// shr     i0,i0,3
					UML_DROR(block, IREG(1), IREG(1), IMM(32 * (mips3->bigendian ? BYTE_XOR_BE(0) : BYTE_XOR_LE(0))));
																					// dror    i1,i1,32*bytexor
					if (ismasked)
					{
						UML_DROR(block, IREG(2), IREG(2), IMM(32 * (mips3->bigendian ? BYTE_XOR_BE(0) : BYTE_XOR_LE(0))));
																					// dror    i2,i2,32*bytexor
						UML_DLOAD8(block, IREG(3), fastbase, IREG(0));				// dload8  i3,fastbase,i0
						UML_DINSERT(block, IREG(3), IREG(1), IMM(0), IREG(2));		// dinsert i3,i1,0,i2
						UML_DSTORE8(block, fastbase, IREG(0), IREG(3));				// dstore8 fastbase,i0,i3
					}
					else
						UML_DSTORE8(block, fastbase, IREG(0), IREG(1));				// dstore8 fastbase,i0,i1
				}
				UML_RET(block);														// ret
			}

			UML_LABEL(block, skip);												// skip:
		}

	switch (size)
	{
		case 1:
			if (iswrite)
				UML_WRITE1(block, PROGRAM, IREG(0), IREG(1));						// write1  i0,i1
			else
				UML_READ1U(block, IREG(0), PROGRAM, IREG(0));						// read1u  i0,i0
			break;

		case 2:
			if (iswrite)
				UML_WRITE2(block, PROGRAM, IREG(0), IREG(1));						// write2  i0,i1
			else
				UML_READ2U(block, IREG(0), PROGRAM, IREG(0));						// read2u  i0,i0
			break;

		case 4:
			if (iswrite)
			{
				if (!ismasked)
					UML_WRITE4(block, PROGRAM, IREG(0), IREG(1));					// write4  PROGRAM,i0,i1
				else
					UML_WRIT4M(block, PROGRAM, IREG(0), IREG(2), IREG(1));			// writ4m  PROGRAM,i0,i2,i1
			}
			else
			{
				if (!ismasked)
					UML_READ4(block, IREG(0), PROGRAM, IREG(0));					// read4   i0,PROGRAM,i0
				else
					UML_READ4M(block, IREG(0), PROGRAM, IREG(0), IREG(2));			// read4m  i0,PROGRAM,i0,i2
			}
			break;

		case 8:
			if (iswrite)
			{
				if (!ismasked)
					UML_DWRITE8(block, PROGRAM, IREG(0), IREG(1));					// dwrite8 PROGRAM,i0,i1
				else
					UML_DWRIT8M(block, PROGRAM, IREG(0), IREG(2), IREG(1));			// dwrit8m PROGRAM,i0,i2,i1
			}
			else
			{
				if (!ismasked)
					UML_DREAD8(block, IREG(0), PROGRAM, IREG(0));					// dread8  i0,PROGRAM,i0
				else
					UML_DREAD8M(block, IREG(0), PROGRAM, IREG(0), IREG(2));			// dread8m i0,PROGRAM,i0,i2
			}
			break;
	}
	UML_RET(block);																	// ret

	if (tlbmiss != 0)
	{
		UML_LABEL(block, tlbmiss);												// tlbmiss:
		if (iswrite)
		{
			UML_TEST(block, IREG(3), IMM(TLB_VALID));								// test    i3,TLB_VALID
			UML_EXHc(block, IF_NZ, mips3->impstate->exception[EXCEPTION_TLBMOD], IREG(0));
																					// exh     tlbmod,i0,nz
		}
		UML_TEST(block, IREG(3), IMM(TLB_PRESENT));									// test    i3,TLB_PRESENT
		UML_EXHc(block, IF_NZ, exception_tlb, IREG(0));								// exh     tlb,i0,nz
		UML_EXH(block, exception_tlbfill, IREG(0));									// exh     tlbfill,i0
	}

	drcuml_block_end(block);
}



/***************************************************************************
    CODE GENERATION
***************************************************************************/

/*-------------------------------------------------
    generate_update_mode - update the mode based
    on a new SR (in i0); trashes i2
-------------------------------------------------*/

static void generate_update_mode(drcuml_block *block)
{
	UML_XTRACT(block, IREG(2), IREG(0), IMM(32-2), IMM(0x06));						// xtract  i2,i0,32-2,0x06
	UML_TEST(block, IREG(0), IMM(SR_EXL | SR_ERL));									// test    i0,SR_EXL | SR_ERL
	UML_MOVc(block, IF_NZ, IREG(2), IMM(0));										// mov     i2,0,nz
	UML_INSERT(block, IREG(2), IREG(0), IMM(32-26), IMM(0x01));						// insert  i2,i0,32-26,0x01
	UML_MOV(block, MEM(&mips3->impstate->mode), IREG(2));							// mov     [mode],i2
}


/*-------------------------------------------------
    generate_update_cycles - generate code to
    subtract cycles from the icount and generate
    an exception if out
-------------------------------------------------*/

static void generate_update_cycles(drcuml_block *block, compiler_state *compiler, drcuml_ptype ptype, UINT64 pvalue, int allow_exception)
{
	/* check software interrupts if pending */
	if (compiler->checksoftints)
	{
		drcuml_codelabel skip;

		compiler->checksoftints = FALSE;
		UML_AND(block, IREG(0), CPR032(COP0_Cause), CPR032(COP0_Status));			// and     i0,[Cause],[Status]
		UML_ANDf(block, IREG(0), IREG(0), IMM(0x0300), FLAGS_Z);					// and     i0,i0,0x0300,Z
		UML_JMPc(block, IF_Z, skip = compiler->labelnum++);							// jmp     skip,Z
		UML_MOV(block, IREG(0), PARAM(ptype, pvalue));								// mov     i0,nextpc
		UML_MOV(block, IREG(1), IMM(compiler->cycles));								// mov     i1,cycles
		UML_CALLH(block, mips3->impstate->exception_norecover[EXCEPTION_INTERRUPT]);// callh   interrupt_norecover
		UML_LABEL(block, skip);													// skip:
	}

	/* check full interrupts if pending */
	if (compiler->checkints)
	{
		drcuml_codelabel skip;

		compiler->checkints = FALSE;
		UML_AND(block, IREG(0), CPR032(COP0_Cause), CPR032(COP0_Status));			// and     i0,[Cause],[Status]
		UML_ANDf(block, IREG(0), IREG(0), IMM(0xfc00), FLAGS_Z);					// and     i0,i0,0xfc00,Z
		UML_JMPc(block, IF_Z, skip = compiler->labelnum++);							// jmp     skip,Z
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_IE));							// test    [Status],SR_IE
		UML_JMPc(block, IF_Z, skip);												// jmp     skip,Z
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_EXL | SR_ERL));					// test    [Status],SR_EXL | SR_ERL
		UML_JMPc(block, IF_NZ, skip);												// jmp     skip,NZ
		UML_MOV(block, IREG(0), PARAM(ptype, pvalue));								// mov     i0,nextpc
		UML_MOV(block, IREG(1), IMM(compiler->cycles));								// mov     i1,cycles
		UML_CALLH(block, mips3->impstate->exception_norecover[EXCEPTION_INTERRUPT]);// callh   interrupt_norecover
		UML_LABEL(block, skip);													// skip:
	}

	/* account for cycles */
	if (compiler->cycles > 0)
	{
		UML_SUBf(block, MEM(&mips3->icount), MEM(&mips3->icount), MAPVAR_CYCLES, allow_exception ? FLAGS_S : 0);
																					// sub     icount,icount,cycles,S
		UML_MAPVAR(block, MAPVAR_CYCLES, 0);										// mapvar  cycles,0
		if (allow_exception)
			UML_EXHc(block, IF_S, mips3->impstate->out_of_cycles, PARAM(ptype, pvalue));
																					// exh     out_of_cycles,nextpc
	}
	compiler->cycles = 0;
}


/*-------------------------------------------------
    generate_checksum_block - generate code to
    validate a sequence of opcodes
-------------------------------------------------*/

static void generate_checksum_block(drcuml_block *block, compiler_state *compiler, const opcode_desc *seqhead, const opcode_desc *seqlast)
{
	const opcode_desc *curdesc;
	if (LOG_UML)
		UML_COMMENT(block, "[Validation for %08X]", seqhead->pc);					// comment

	/* loose verify or single instruction: just compare and fail */
	if (!(mips3->impstate->drcoptions & MIPS3DRC_STRICT_VERIFY) || seqhead->next == NULL)
	{
		if (!(seqhead->flags & OPFLAG_VIRTUAL_NOOP))
		{
			UML_LOAD4(block, IREG(0), seqhead->opptr.l, IMM(0));					// load4   i0,*opptr
			UML_CMP(block, IREG(0), IMM(*seqhead->opptr.l));						// cmp     i0,*opptr
			UML_EXHc(block, IF_NE, mips3->impstate->nocode, IMM(epc(seqhead)));		// exne    nocode,seqhead->pc
		}
	}

	/* full verification; sum up everything */
	else
	{
#if 0
		for (curdesc = seqhead->next; curdesc != seqlast->next; curdesc = curdesc->next)
			if (!(curdesc->flags & OPFLAG_VIRTUAL_NOOP))
			{
				UML_LOAD4(block, IREG(0), curdesc->opptr.l, IMM(0));				// load4   i0,*opptr
				UML_CMP(block, IREG(0), IMM(*curdesc->opptr.l));					// cmp     i0,*opptr
				UML_EXHc(block, IF_NE, mips3->impstate->nocode, IMM(epc(seqhead)));	// exne    nocode,seqhead->pc
			}
#else
		UINT32 sum = 0;
		UML_LOAD4(block, IREG(0), seqhead->opptr.l, IMM(0));						// load4   i0,*opptr
		sum += *seqhead->opptr.l;
		for (curdesc = seqhead->next; curdesc != seqlast->next; curdesc = curdesc->next)
			if (!(curdesc->flags & OPFLAG_VIRTUAL_NOOP))
			{
				UML_LOAD4(block, IREG(1), curdesc->opptr.l, IMM(0));				// load4   i1,*opptr
				UML_ADD(block, IREG(0), IREG(0), IREG(1));							// add     i0,i0,i1
				sum += *curdesc->opptr.l;
			}
		UML_CMP(block, IREG(0), IMM(sum));											// cmp     i0,sum
		UML_EXHc(block, IF_NE, mips3->impstate->nocode, IMM(epc(seqhead)));			// exne    nocode,seqhead->pc
#endif
	}
}


/*-------------------------------------------------
    generate_sequence_instruction - generate code
    for a single instruction in a sequence
-------------------------------------------------*/

static void generate_sequence_instruction(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	offs_t expc;
	int hotnum;

	/* add an entry for the log */
	if (LOG_UML && !(desc->flags & OPFLAG_VIRTUAL_NOOP))
		log_add_disasm_comment(block, desc->pc, *desc->opptr.l);

	/* set the PC map variable */
	expc = (desc->flags & OPFLAG_IN_DELAY_SLOT) ? desc->pc - 3 : desc->pc;
	UML_MAPVAR(block, MAPVAR_PC, expc);												// mapvar  PC,expc

	/* accumulate total cycles */
	compiler->cycles += desc->cycles;

	/* is this a hotspot? */
	for (hotnum = 0; hotnum < MIPS3_MAX_HOTSPOTS; hotnum++)
		if (mips3->impstate->hotspot[hotnum].pc != 0 && desc->pc == mips3->impstate->hotspot[hotnum].pc && *desc->opptr.l == mips3->impstate->hotspot[hotnum].opcode)
		{
			compiler->cycles += mips3->impstate->hotspot[hotnum].cycles;
			break;
		}

	/* update the icount map variable */
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);								// mapvar  CYCLES,compiler->cycles

	/* if we want a probe, add it here */
	if (desc->pc == PROBE_ADDRESS)
		UML_CALLC(block, cfunc_printf_probe, desc->pc);								// callc   cfunc_printf_probe,desc->pc

	/* if we are debugging, call the debugger */
	if (Machine->debug_mode)
	{
		UML_MOV(block, MEM(&mips3->pc), IMM(desc->pc));								// mov     [pc],desc->pc
		save_fast_iregs(block);
		UML_DEBUG(block, IMM(desc->pc));											// debug   desc->pc
	}

	/* if we hit an unmapped address, fatal error */
	if (desc->flags & OPFLAG_COMPILER_UNMAPPED)
	{
		UML_MOV(block, MEM(&mips3->pc), IMM(desc->pc));								// mov     [pc],desc->pc
		save_fast_iregs(block);
		UML_EXIT(block, IMM(EXECUTE_UNMAPPED_CODE));								// exit    EXECUTE_UNMAPPED_CODE
	}

	/* if we hit a compiler page fault, it's just like a TLB mismatch */
	if (desc->flags & OPFLAG_COMPILER_PAGE_FAULT)
	{
		if (PRINTF_MMU)
		{
			UML_MOV(block, MEM(&mips3->impstate->arg0), IMM(desc->pc));				// mov     [arg0],desc->pc
			UML_CALLC(block, cfunc_printf_debug, "Compiler page fault @ %08X\n");	// callc   printf_debug
		}
		UML_EXH(block, mips3->impstate->tlb_mismatch, IMM(0));						// exh     tlb_mismatch,0
	}

	/* validate our TLB entry at this PC; if we fail, we need to handle it */
	if ((desc->flags & OPFLAG_VALIDATE_TLB) && (desc->pc < 0x80000000 || desc->pc >= 0xc0000000))
	{
		/* if we currently have a valid TLB read entry, we just verify */
		if (mips3->tlb_table[desc->pc >> 12] & TLB_VALID)
		{
			if (PRINTF_MMU)
			{
				UML_MOV(block, MEM(&mips3->impstate->arg0), IMM(desc->pc));			// mov     [arg0],desc->pc
				UML_CALLC(block, cfunc_printf_debug, "Checking TLB at @ %08X\n");	// callc   printf_debug
			}
			UML_LOAD4(block, IREG(0), &mips3->tlb_table[desc->pc >> 12], IMM(0));	// load4   i0,tlb_table[desc->pc >> 12]
			UML_CMP(block, IREG(0), IMM(mips3->tlb_table[desc->pc >> 12]));			// cmp     i0,*tlbentry
			UML_EXHc(block, IF_NE, mips3->impstate->tlb_mismatch, IMM(0));			// exh     tlb_mismatch,0,NE
		}

		/* otherwise, we generate an unconditional exception */
		else
		{
			if (PRINTF_MMU)
			{
				UML_MOV(block, MEM(&mips3->impstate->arg0), IMM(desc->pc));			// mov     [arg0],desc->pc
				UML_CALLC(block, cfunc_printf_debug, "No valid TLB @ %08X\n");		// callc   printf_debug
			}
			UML_EXH(block, mips3->impstate->tlb_mismatch, IMM(0));					// exh     tlb_mismatch,0
		}
	}

	/* if this is an invalid opcode, generate the exception now */
	if (desc->flags & OPFLAG_INVALID_OPCODE)
		UML_EXH(block, mips3->impstate->exception[EXCEPTION_INVALIDOP], IMM(0));	// exh     invalidop,0

	/* otherwise, unless this is a virtual no-op, it's a regular instruction */
	else if (!(desc->flags & OPFLAG_VIRTUAL_NOOP))
	{
		/* compile the instruction */
		if (!generate_opcode(block, compiler, desc))
		{
			UML_MOV(block, MEM(&mips3->pc), IMM(desc->pc));							// mov     [pc],desc->pc
			UML_CALLC(block, cfunc_unimplemented, (void *)(FPTR)*desc->opptr.l);	// callc   cfunc_unimplemented
		}
	}
}


/*------------------------------------------------------------------
    generate_delay_slot_and_branch
------------------------------------------------------------------*/

static void generate_delay_slot_and_branch(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 linkreg)
{
	compiler_state compiler_temp = *compiler;
	UINT32 op = *desc->opptr.l;

	/* set the link if needed */
	if (linkreg != 0)
		UML_DSEXT4(block, R64(linkreg), IMM((INT32)(desc->pc + 8)));				// dsext4  <linkreg>,desc->pc + 8

	/* compile the delay slot using temporary compiler state */
	assert(desc->delay != NULL);
	generate_sequence_instruction(block, &compiler_temp, desc->delay);				// <next instruction>

	/* update the cycles and jump through the hash table to the target */
	if (desc->targetpc != BRANCH_TARGET_DYNAMIC)
	{
		generate_update_cycles(block, &compiler_temp, IMM(desc->targetpc), TRUE);	// <subtract cycles>
		if (desc->flags & OPFLAG_INTRABLOCK_BRANCH)
			UML_JMP(block, desc->targetpc | 0x80000000);							// jmp     desc->targetpc | 0x80000000
		else
			UML_HASHJMP(block, IMM(mips3->impstate->mode), IMM(desc->targetpc), mips3->impstate->nocode);
																					// hashjmp <mode>,desc->targetpc,nocode
	}
	else
	{
		generate_update_cycles(block, &compiler_temp, R32(RSREG), TRUE);
																					// <subtract cycles>
		UML_HASHJMP(block, IMM(mips3->impstate->mode), R32(RSREG), mips3->impstate->nocode);		
																					// hashjmp <mode>,<rsreg>,nocode
	}

	/* update the label */
	compiler->labelnum = compiler_temp.labelnum;

	/* reset the mapvar to the current cycles */
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);								// mapvar  CYCLES,compiler->cycles
}


/*-------------------------------------------------
    generate_opcode - generate code for a specific
    opcode
-------------------------------------------------*/

static int generate_opcode(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	int in_delay_slot = ((desc->flags & OPFLAG_IN_DELAY_SLOT) != 0);
	UINT32 op = *desc->opptr.l;
	UINT8 opswitch = op >> 26;
	drcuml_codelabel skip;

	switch (opswitch)
	{
		/* ----- sub-groups ----- */

		case 0x00:	/* SPECIAL - MIPS I */
			return generate_special(block, compiler, desc);

		case 0x01:	/* REGIMM - MIPS I */
			return generate_regimm(block, compiler, desc);

		case 0x1c:	/* IDT-specific */
			return generate_idt(block, compiler, desc);


		/* ----- jumps and branches ----- */

		case 0x02:	/* J - MIPS I */
			generate_delay_slot_and_branch(block, compiler, desc, 0);				// <next instruction + hashjmp>
			return TRUE;

		case 0x03:	/* JAL - MIPS I */
			generate_delay_slot_and_branch(block, compiler, desc, 31);				// <next instruction + hashjmp>
			return TRUE;

		case 0x04:	/* BEQ - MIPS I */
		case 0x14:	/* BEQL - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_JMPc(block, IF_NE, skip = compiler->labelnum++);					// jmp     skip,NE
			generate_delay_slot_and_branch(block, compiler, desc, 0);				// <next instruction + hashjmp>
			UML_LABEL(block, skip);												// skip:
			return TRUE;

		case 0x05:	/* BNE - MIPS I */
		case 0x15:	/* BNEL - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_JMPc(block, IF_E, skip = compiler->labelnum++);						// jmp     skip,E
			generate_delay_slot_and_branch(block, compiler, desc, 0);				// <next instruction + hashjmp>
			UML_LABEL(block, skip);												// skip:
			return TRUE;

		case 0x06:	/* BLEZ - MIPS I */
		case 0x16:	/* BLEZL - MIPS II */
			if (RSREG != 0)
			{
				UML_DCMP(block, R64(RSREG), IMM(0));								// dcmp    <rsreg>,0
				UML_JMPc(block, IF_G, skip = compiler->labelnum++);					// jmp     skip,G
				generate_delay_slot_and_branch(block, compiler, desc, 0);			// <next instruction + hashjmp>
				UML_LABEL(block, skip);											// skip:
			}
			else
				generate_delay_slot_and_branch(block, compiler, desc, 0);			// <next instruction + hashjmp>
			return TRUE;

		case 0x07:	/* BGTZ - MIPS I */
		case 0x17:	/* BGTZL - MIPS II */
			UML_DCMP(block, R64(RSREG), IMM(0));									// dcmp    <rsreg>,0
			UML_JMPc(block, IF_LE, skip = compiler->labelnum++);					// jmp     skip,LE
			generate_delay_slot_and_branch(block, compiler, desc, 0);				// <next instruction + hashjmp>
			UML_LABEL(block, skip);												// skip:
			return TRUE;


		/* ----- immediate arithmetic ----- */

		case 0x0f:	/* LUI - MIPS I */
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), IMM(SIMMVAL << 16));					// dsext4  <rtreg>,SIMMVAL << 16
			return TRUE;

		case 0x08:	/* ADDI - MIPS I */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_ADDf(block, IREG(0), R32(RSREG), IMM(SIMMVAL), FLAGS_V);		// add     i0,<rsreg>,SIMMVAL,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh    overflow,0
			}
			else
				UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));					// add     i0,<rsreg>,SIMMVAL
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), IREG(0));								// dsext4  <rtreg>,i0
			return TRUE;

		case 0x09:	/* ADDIU - MIPS I */
			if (RTREG != 0)
			{
				UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));					// add     i0,<rsreg>,SIMMVAL,V
				UML_DSEXT4(block, R64(RTREG), IREG(0));								// dsext4  <rtreg>,i0
			}
			return TRUE;

		case 0x18:	/* DADDI - MIPS III */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_DADDf(block, IREG(0), R64(RSREG), IMM(SIMMVAL), FLAGS_V);		// dadd    i0,<rsreg>,SIMMVAL,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh    overflow,0
			}
			else
				UML_DADD(block, IREG(0), R64(RSREG), IMM(SIMMVAL));					// dadd    i0,<rsreg>,SIMMVAL
			if (RTREG != 0)
				UML_DMOV(block, R64(RTREG), IREG(0));								// dmov    <rtreg>,i0
			return TRUE;

		case 0x19:	/* DADDIU - MIPS III */
			if (RTREG != 0)
				UML_DADD(block, R64(RTREG), R64(RSREG), IMM(SIMMVAL));				// dadd    <rtreg>,<rsreg>,SIMMVAL
			return TRUE;

		case 0x0c:	/* ANDI - MIPS I */
			if (RTREG != 0)
				UML_DAND(block, R64(RTREG), R64(RSREG), IMM(UIMMVAL));				// dand    <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		case 0x0d:	/* ORI - MIPS I */
			if (RTREG != 0)
				UML_DOR(block, R64(RTREG), R64(RSREG), IMM(UIMMVAL));				// dor     <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		case 0x0e:	/* XORI - MIPS I */
			if (RTREG != 0)
				UML_DXOR(block, R64(RTREG), R64(RSREG), IMM(UIMMVAL));				// dxor    <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		case 0x0a:	/* SLTI - MIPS I */
			if (RTREG != 0)
			{
				UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));							// dcmp    <rsreg>,SIMMVAL
				UML_DFLAGS(block, R64(RTREG), (UINT64)~0, mips3->impstate->slt_table);	
																					// dflags  <rtreg>,~0,slt_table
			}
			return TRUE;

		case 0x0b:	/* SLTIU - MIPS I */
			if (RTREG != 0)
			{
				UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));							// dcmp    <rsreg>,SIMMVAL
				UML_DFLAGS(block, R64(RTREG), (UINT64)~0, mips3->impstate->sltu_table);
																					// dflags  <rtreg>,~0,sltu_table
			}
			return TRUE;


		/* ----- memory load operations ----- */

		case 0x20:	/* LB - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read8[mips3->impstate->mode >> 1]);	// callh   read8
			if (RTREG != 0)
				UML_DSEXT1(block, R64(RTREG), IREG(0));								// dsext1  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x21:	/* LH - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read16[mips3->impstate->mode >> 1]);	// callh   read16
			if (RTREG != 0)
				UML_DSEXT2(block, R64(RTREG), IREG(0));								// dsext2  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x23:	/* LW - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read32[mips3->impstate->mode >> 1]);	// callh   read32
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), IREG(0));								// dsext4  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x24:	/* LBU - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read8[mips3->impstate->mode >> 1]);	// callh   read8
			if (RTREG != 0)
				UML_DZEXT1(block, R64(RTREG), IREG(0));								// dzext1  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x25:	/* LHU - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read16[mips3->impstate->mode >> 1]);	// callh   read16
			if (RTREG != 0)
				UML_DZEXT2(block, R64(RTREG), IREG(0));								// dzext2  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x27:	/* LWU - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read32[mips3->impstate->mode >> 1]);	// callh   read32
			if (RTREG != 0)
				UML_DZEXT4(block, R64(RTREG), IREG(0));								// dzext4  <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x37:	/* LD - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read64[mips3->impstate->mode >> 1]);	// callh   read64
			if (RTREG != 0)
				UML_DMOV(block, R64(RTREG), IREG(0));								// dmov    <rtreg>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x22:	/* LWL - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(1), IREG(0), IMM(3));								// shl     i1,i0,3
			if (!mips3->bigendian)
				UML_XOR(block, IREG(1), IREG(1), IMM(0x18));						// xor     i1,i1,0x18
			UML_SHR(block, IREG(2), IMM(~0), IREG(1));								// shr     i2,~0,i1
			UML_CALLH(block, mips3->impstate->read32mask[mips3->impstate->mode >> 1]);
																					// callh   read32mask
			if (RTREG != 0)
			{
				UML_SHL(block, IREG(2), IMM(~0), IREG(1));							// shl     i2,~0,i1
				UML_MOV(block, IREG(3), R32(RTREG));								// mov     i3,<rtreg>
				UML_INSERT(block, IREG(3), IREG(0), IREG(1), IREG(2));				// insert  i3,i0,i1,i2
				UML_DSEXT4(block, R64(RTREG), IREG(3));								// dsext4  <rtreg>,i3
			}
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x26:	/* LWR - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(1), IREG(0), IMM(3));								// shl     i1,i0,3
			if (mips3->bigendian)
				UML_XOR(block, IREG(1), IREG(1), IMM(0x18));						// xor     i1,i1,0x18
			UML_SHL(block, IREG(2), IMM(~0), IREG(1));								// shl     i2,~0,i1
			UML_CALLH(block, mips3->impstate->read32mask[mips3->impstate->mode >> 1]);			
																					// callh   read32mask
			if (RTREG != 0)
			{
				UML_SHR(block, IREG(2), IMM(~0), IREG(1));							// shr     i2,~0,i1
				UML_SUB(block, IREG(1), IMM(32), IREG(1));							// sub     i1,32,i1
				UML_MOV(block, IREG(3), R32(RTREG));								// mov     i3,<rtreg>
				UML_INSERT(block, IREG(3), IREG(0), IREG(1), IREG(2));				// insert  i3,i0,i1,i2
				UML_DSEXT4(block, R64(RTREG), IREG(3));								// dsext4  <rtreg>,i3
			}
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x1a:	/* LDL - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(1), IREG(0), IMM(3));								// shl     i1,i0,3
			if (!mips3->bigendian)
				UML_XOR(block, IREG(1), IREG(1), IMM(0x38));						// xor     i1,i1,0x38
			UML_DSHR(block, IREG(2), IMM((UINT64)~0), IREG(1));						// dshr    i2,~0,i1
			UML_CALLH(block, mips3->impstate->read64mask[mips3->impstate->mode >> 1]);			
																					// callh   read64mask
			if (RTREG != 0)
			{
				UML_DSHL(block, IREG(2), IMM((UINT64)~0), IREG(1));					// dshl    i2,~0,i1
				UML_DINSERT(block, R64(RTREG), IREG(0), IREG(1), IREG(2));			// dinsert <rtreg>,i0,i1,i2
			}
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x1b:	/* LDR - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(1), IREG(0), IMM(3));								// shl     i1,i0,3
			if (mips3->bigendian)
				UML_XOR(block, IREG(1), IREG(1), IMM(0x38));						// xor     i1,i1,0x38
			UML_DSHL(block, IREG(2), IMM((UINT64)~0), IREG(1));						// dshl    i2,~0,i1
			UML_CALLH(block, mips3->impstate->read64mask[mips3->impstate->mode >> 1]);			
																					// callh   read64mask
			if (RTREG != 0)
			{
				UML_DSHR(block, IREG(2), IMM((UINT64)~0), IREG(1));					// dshr    i2,~0,i1
				UML_SUB(block, IREG(1), IMM(64), IREG(1));							// sub     i1,64,i1
				UML_DINSERT(block, R64(RTREG), IREG(0), IREG(1), IREG(2));			// dinsert <rtreg>,i0,i1,i2
			}
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x31:	/* LWC1 - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read32[mips3->impstate->mode >> 1]);	// callh   read32
			UML_MOV(block, FPR32(RTREG), IREG(0));									// mov     <cpr1_rt>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x35:	/* LDC1 - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read64[mips3->impstate->mode >> 1]);	// callh   read64
			UML_DMOV(block, FPR64(RTREG), IREG(0));									// dmov    <cpr1_rt>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x32:	/* LWC2 - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read32[mips3->impstate->mode >> 1]);	// callh   read32
			UML_DZEXT4(block, CPR264(RTREG), IREG(0));								// dzext4  <cpr2_rt>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x36:	/* LDC2 - MIPS II */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, mips3->impstate->read64[mips3->impstate->mode >> 1]);	// callh   read64
			UML_DMOV(block, CPR264(RTREG), IREG(0));								// dmov    <cpr2_rt>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;


		/* ----- memory store operations ----- */

		case 0x28:	/* SB - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, IREG(1), R32(RTREG));									// mov     i1,<rtreg>
			UML_CALLH(block, mips3->impstate->write8[mips3->impstate->mode >> 1]);	// callh   write8
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x29:	/* SH - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, IREG(1), R32(RTREG));									// mov     i1,<rtreg>
			UML_CALLH(block, mips3->impstate->write16[mips3->impstate->mode >> 1]);	// callh   write16
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x2b:	/* SW - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, IREG(1), R32(RTREG));									// mov     i1,<rtreg>
			UML_CALLH(block, mips3->impstate->write32[mips3->impstate->mode >> 1]);	// callh   write32
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x3f:	/* SD - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_DMOV(block, IREG(1), R64(RTREG));									// dmov    i1,<rtreg>
			UML_CALLH(block, mips3->impstate->write64[mips3->impstate->mode >> 1]);	// callh   write64
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x2a:	/* SWL - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(3), IREG(0), IMM(3));								// shl     i3,i0,3
			UML_MOV(block, IREG(1), R32(RTREG));									// mov     i1,<rtreg>
			if (!mips3->bigendian)
				UML_XOR(block, IREG(3), IREG(3), IMM(0x18));						// xor     i3,i3,0x18
			UML_SHR(block, IREG(2), IMM(~0), IREG(3));								// shr     i2,~0,i3
			UML_SHR(block, IREG(1), IREG(1), IREG(3));								// shr     i1,i1,i3
			UML_CALLH(block, mips3->impstate->write32mask[mips3->impstate->mode >> 1]);			
																					// callh   write32mask
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x2e:	/* SWR - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(3), IREG(0), IMM(3));								// shl     i3,i0,3
			UML_MOV(block, IREG(1), R32(RTREG));									// mov     i1,<rtreg>
			if (mips3->bigendian)
				UML_XOR(block, IREG(3), IREG(3), IMM(0x18));						// xor     i3,i3,0x18
			UML_SHL(block, IREG(2), IMM(~0), IREG(3));								// shl     i2,~0,i3
			UML_SHL(block, IREG(1), IREG(1), IREG(3));								// shl     i1,i1,i3
			UML_CALLH(block, mips3->impstate->write32mask[mips3->impstate->mode >> 1]);			
																					// callh   write32mask
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x2c:	/* SDL - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(3), IREG(0), IMM(3));								// shl     i3,i0,3
			UML_DMOV(block, IREG(1), R64(RTREG));									// dmov    i1,<rtreg>
			if (!mips3->bigendian)
				UML_XOR(block, IREG(3), IREG(3), IMM(0x38));						// xor     i3,i3,0x38
			UML_DSHR(block, IREG(2), IMM((UINT64)~0), IREG(3));						// dshr    i2,~0,i3
			UML_DSHR(block, IREG(1), IREG(1), IREG(3));								// dshr    i1,i1,i3
			UML_CALLH(block, mips3->impstate->write64mask[mips3->impstate->mode >> 1]);			
																					// callh   write64mask
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x2d:	/* SDR - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_SHL(block, IREG(3), IREG(0), IMM(3));								// shl     i3,i0,3
			UML_DMOV(block, IREG(1), R64(RTREG));									// dmov    i1,<rtreg>
			if (mips3->bigendian)
				UML_XOR(block, IREG(3), IREG(3), IMM(0x38));						// xor     i3,i3,0x38
			UML_DSHL(block, IREG(2), IMM((UINT64)~0), IREG(3));						// dshl    i2,~0,i3
			UML_DSHL(block, IREG(1), IREG(1), IREG(3));								// dshl    i1,i1,i3
			UML_CALLH(block, mips3->impstate->write64mask[mips3->impstate->mode >> 1]);			
																					// callh   write64mask
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x39:	/* SWC1 - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, IREG(1), FPR32(RTREG));									// mov     i1,<cpr1_rt>
			UML_CALLH(block, mips3->impstate->write32[mips3->impstate->mode >> 1]);	// callh   write32
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x3d:	/* SDC1 - MIPS III */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_DMOV(block, IREG(1), FPR64(RTREG));									// dmov    i1,<cpr1_rt>
			UML_CALLH(block, mips3->impstate->write64[mips3->impstate->mode >> 1]);	// callh   write64
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x3a:	/* SWC2 - MIPS I */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, IREG(1), CPR232(RTREG));									// mov     i1,<cpr2_rt>
			UML_CALLH(block, mips3->impstate->write32[mips3->impstate->mode >> 1]);	// callh   write32
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x3e:	/* SDC2 - MIPS II */
			UML_ADD(block, IREG(0), R32(RSREG), IMM(SIMMVAL));						// add     i0,<rsreg>,SIMMVAL
			UML_DMOV(block, IREG(1), CPR264(RTREG));								// dmov    i1,<cpr2_rt>
			UML_CALLH(block, mips3->impstate->write64[mips3->impstate->mode >> 1]);	// callh   write64
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;


		/* ----- effective no-ops ----- */

		case 0x2f:	/* CACHE - MIPS II */
		case 0x33:	/* PREF - MIPS IV */
			return TRUE;


		/* ----- coprocessor instructions ----- */

		case 0x10:	/* COP0 - MIPS I */
			return generate_cop0(block, compiler, desc);

		case 0x11:	/* COP1 - MIPS I */
			return generate_cop1(block, compiler, desc);

		case 0x13:	/* COP1X - MIPS IV */
			return generate_cop1x(block, compiler, desc);

		case 0x12:	/* COP2 - MIPS I */
			UML_EXH(block, mips3->impstate->exception[EXCEPTION_INVALIDOP], IMM(0));// exh     invalidop,0
			return TRUE;


		/* ----- unimplemented/illegal instructions ----- */

//      case 0x30:  /* LL - MIPS II */        logerror("mips3 Unhandled op: LL\n");                                   break;
//      case 0x34:  /* LLD - MIPS III */       logerror("mips3 Unhandled op: LLD\n");                                  break;
//      case 0x38:  /* SC - MIPS II */        logerror("mips3 Unhandled op: SC\n");                                   break;
//      case 0x3c:  /* SCD - MIPS III */       logerror("mips3 Unhandled op: SCD\n");                                  break;
//      default:    /* ??? */       invalid_instruction(op);                                                break;
	}

	return FALSE;
}


/*-------------------------------------------------
    generate_special - compile opcodes in the
    'SPECIAL' group
-------------------------------------------------*/

static int generate_special(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = *desc->opptr.l;
	UINT8 opswitch = op & 63;

	switch (opswitch)
	{
		/* ----- shift instructions ----- */

		case 0x00:	/* SLL - MIPS I */
			if (RDREG != 0)
			{
				UML_SHL(block, IREG(0), R32(RTREG), IMM(SHIFT));					// shl     i0,<rtreg>,<shift>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x02:	/* SRL - MIPS I */
			if (RDREG != 0)
			{
				UML_SHR(block, IREG(0), R32(RTREG), IMM(SHIFT));					// shr     i0,<rtreg>,<shift>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x03:	/* SRA - MIPS I */
			if (RDREG != 0)
			{
				UML_SAR(block, IREG(0), R32(RTREG), IMM(SHIFT));					// sar     i0,<rtreg>,<shift>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x04:	/* SLLV - MIPS I */
			if (RDREG != 0)
			{
				UML_SHL(block, IREG(0), R32(RTREG), R32(RSREG));					// shl     i0,<rtreg>,<rsreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x06:	/* SRLV - MIPS I */
			if (RDREG != 0)
			{
				UML_SHR(block, IREG(0), R32(RTREG), R32(RSREG));					// shr     i0,<rtreg>,<rsreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x07:	/* SRAV - MIPS I */
			if (RDREG != 0)
			{
				UML_SAR(block, IREG(0), R32(RTREG), R32(RSREG));					// sar     i0,<rtreg>,<rsreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x38:	/* DSLL - MIPS III */
			if (RDREG != 0)
				UML_DSHL(block, R64(RDREG), R64(RTREG), IMM(SHIFT));				// dshl    <rdreg>,<rtreg>,<shift>
			return TRUE;

		case 0x3a:	/* DSRL - MIPS III */
			if (RDREG != 0)
				UML_DSHR(block, R64(RDREG), R64(RTREG), IMM(SHIFT));				// dshr    <rdreg>,<rtreg>,<shift>
			return TRUE;

		case 0x3b:	/* DSRA - MIPS III */
			if (RDREG != 0)
				UML_DSAR(block, R64(RDREG), R64(RTREG), IMM(SHIFT));				// dsar    <rdreg>,<rtreg>,<shift>
			return TRUE;

		case 0x3c:	/* DSLL32 - MIPS III */
			if (RDREG != 0)
				UML_DSHL(block, R64(RDREG), R64(RTREG), IMM(SHIFT + 32));			// dshl    <rdreg>,<rtreg>,<shift>+32
			return TRUE;

		case 0x3e:	/* DSRL32 - MIPS III */
			if (RDREG != 0)
				UML_DSHR(block, R64(RDREG), R64(RTREG), IMM(SHIFT + 32));			// dshr    <rdreg>,<rtreg>,<shift>+32
			return TRUE;

		case 0x3f:	/* DSRA32 - MIPS III */
			if (RDREG != 0)
				UML_DSAR(block, R64(RDREG), R64(RTREG), IMM(SHIFT + 32));			// dsar    <rdreg>,<rtreg>,<shift>+32
			return TRUE;

		case 0x14:	/* DSLLV - MIPS III */
			if (RDREG != 0)
				UML_DSHL(block, R64(RDREG), R64(RTREG), R64(RSREG));				// dshl    <rdreg>,<rtreg>,<rsreg>
			return TRUE;

		case 0x16:	/* DSRLV - MIPS III */
			if (RDREG != 0)
				UML_DSHR(block, R64(RDREG), R64(RTREG), R64(RSREG));				// dshr    <rdreg>,<rtreg>,<rsreg>
			return TRUE;

		case 0x17:	/* DSRAV - MIPS III */
			if (RDREG != 0)
				UML_DSAR(block, R64(RDREG), R64(RTREG), R64(RSREG));				// dsar    <rdreg>,<rtreg>,<rsreg>
			return TRUE;


		/* ----- basic arithmetic ----- */

		case 0x20:	/* ADD - MIPS I */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_ADDf(block, IREG(0), R32(RSREG), R32(RTREG), FLAGS_V);			// add     i0,<rsreg>,<rtreg>,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh     overflow,0,V
				if (RDREG != 0)
					UML_DSEXT4(block, R64(RDREG), IREG(0));							// dsext4  <rdreg>,i0
			}
			else if (RDREG != 0)
			{
				UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));					// add     i0,<rsreg>,<rtreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x21:	/* ADDU - MIPS I */
			if (RDREG != 0)
			{
				UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));					// add     i0,<rsreg>,<rtreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x2c:	/* DADD - MIPS III */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_DADDf(block, IREG(0), R64(RSREG), R64(RTREG), FLAGS_V);			// dadd    i0,<rsreg>,<rtreg>,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh     overflow,0,V
				if (RDREG != 0)
					UML_DMOV(block, R64(RDREG), IREG(0));							// dmov    <rdreg>,i0
			}
			else if (RDREG != 0)
				UML_DADD(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dadd    <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x2d:	/* DADDU - MIPS III */
			if (RDREG != 0)
				UML_DADD(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dadd    <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x22:	/* SUB - MIPS I */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_SUBf(block, IREG(0), R32(RSREG), R32(RTREG), FLAGS_V);			// sub     i0,<rsreg>,<rtreg>,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh     overflow,0,V
				if (RDREG != 0)
					UML_DSEXT4(block, R64(RDREG), IREG(0));							// dsext4  <rdreg>,i0
			}
			else if (RDREG != 0)
			{
				UML_SUB(block, IREG(0), R32(RSREG), R32(RTREG));					// sub     i0,<rsreg>,<rtreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x23:	/* SUBU - MIPS I */
			if (RDREG != 0)
			{
				UML_SUB(block, IREG(0), R32(RSREG), R32(RTREG));					// sub     i0,<rsreg>,<rtreg>
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4  <rdreg>,i0
			}
			return TRUE;

		case 0x2e:	/* DSUB - MIPS III */
			if (mips3->impstate->drcoptions & MIPS3DRC_CHECK_OVERFLOWS)
			{
				UML_DSUBf(block, IREG(0), R64(RSREG), R64(RTREG), FLAGS_V);			// dsub    i0,<rsreg>,<rtreg>,V
				UML_EXHc(block, IF_V, mips3->impstate->exception[EXCEPTION_OVERFLOW], IMM(0));	
																					// exh     overflow,0,V
				if (RDREG != 0)
					UML_DMOV(block, R64(RDREG), IREG(0));							// dmov    <rdreg>,i0
			}
			else if (RDREG != 0)
				UML_DSUB(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dsub    <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x2f:	/* DSUBU - MIPS III */
			if (RDREG != 0)
				UML_DSUB(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dsub    <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x18:	/* MULT - MIPS I */
			UML_MULS(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));				// muls    i0,i1,<rsreg>,<rtreg>
			UML_DSEXT4(block, LO64, IREG(0));										// dsext4  lo,i0
			UML_DSEXT4(block, HI64, IREG(1));										// dsext4  hi,i1
			return TRUE;

		case 0x19:	/* MULTU - MIPS I */
			UML_MULU(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));				// mulu    i0,i1,<rsreg>,<rtreg>
			UML_DSEXT4(block, LO64, IREG(0));										// dsext4  lo,i0
			UML_DSEXT4(block, HI64, IREG(1));										// dsext4  hi,i1
			return TRUE;

		case 0x1c:	/* DMULT - MIPS III */
			UML_DMULS(block, LO64, HI64, R64(RSREG), R64(RTREG));					// dmuls   lo,hi,<rsreg>,<rtreg>
			return TRUE;

		case 0x1d:	/* DMULTU - MIPS III */
			UML_DMULU(block, LO64, HI64, R64(RSREG), R64(RTREG));					// dmulu   lo,hi,<rsreg>,<rtreg>
			return TRUE;

		case 0x1a:	/* DIV - MIPS I */
			UML_DIVS(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));				// divs    i0,i1,<rsreg>,<rtreg>
			UML_DSEXT4(block, LO64, IREG(0));										// dsext4  lo,i0
			UML_DSEXT4(block, HI64, IREG(1));										// dsext4  hi,i1
			return TRUE;

		case 0x1b:	/* DIVU - MIPS I */
			UML_DIVU(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));				// divu    i0,i1,<rsreg>,<rtreg>
			UML_DSEXT4(block, LO64, IREG(0));										// dsext4  lo,i0
			UML_DSEXT4(block, HI64, IREG(1));										// dsext4  hi,i1
			return TRUE;

		case 0x1e:	/* DDIV - MIPS III */
			UML_DDIVS(block, LO64, HI64, R64(RSREG), R64(RTREG));					// ddivs    lo,hi,<rsreg>,<rtreg>
			return TRUE;

		case 0x1f:	/* DDIVU - MIPS III */
			UML_DDIVU(block, LO64, HI64, R64(RSREG), R64(RTREG));					// ddivu    lo,hi,<rsreg>,<rtreg>
			return TRUE;


		/* ----- basic logical ops ----- */

		case 0x24:	/* AND - MIPS I */
			if (RDREG != 0)
				UML_DAND(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dand     <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x25:	/* OR - MIPS I */
			if (RDREG != 0)
				UML_DOR(block, R64(RDREG), R64(RSREG), R64(RTREG));					// dor      <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x26:	/* XOR - MIPS I */
			if (RDREG != 0)
				UML_DXOR(block, R64(RDREG), R64(RSREG), R64(RTREG));				// dxor     <rdreg>,<rsreg>,<rtreg>
			return TRUE;

		case 0x27:	/* NOR - MIPS I */
			if (RDREG != 0)
			{
				UML_DOR(block, IREG(0), R64(RSREG), R64(RTREG));					// dor      i0,<rsreg>,<rtreg>
				UML_DXOR(block, R64(RDREG), IREG(0), IMM((UINT64)~0));				// dxor     <rdreg>,i0,~0
			}
			return TRUE;


		/* ----- basic comparisons ----- */

		case 0x2a:	/* SLT - MIPS I */
			if (RDREG != 0)
			{
				UML_DCMP(block, R64(RSREG), R64(RTREG));							// dcmp    <rsreg>,<rtreg>
				UML_DFLAGS(block, R64(RDREG), (UINT64)~0, mips3->impstate->slt_table);// dflags  <rdreg>,~0,slt_table
			}
			return TRUE;

		case 0x2b:	/* SLTU - MIPS I */
			if (RDREG != 0)
			{
				UML_DCMP(block, R64(RSREG), R64(RTREG));							// dcmp    <rsreg>,<rtreg>
				UML_DFLAGS(block, R64(RDREG), (UINT64)~0, mips3->impstate->sltu_table);// dflags  <rdreg>,~0,sltu_table
			}
			return TRUE;


		/* ----- conditional traps ----- */

		case 0x30:	/* TGE - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_GE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,GE
			return TRUE;

		case 0x31:	/* TGEU - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_AE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,AE
			return TRUE;

		case 0x32:	/* TLT - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_L, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,LT
			return TRUE;

		case 0x33:	/* TLTU - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_B, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,B
			return TRUE;

		case 0x34:	/* TEQ - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_E, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,E
			return TRUE;

		case 0x36:	/* TNE - MIPS II */
			UML_DCMP(block, R64(RSREG), R64(RTREG));								// dcmp    <rsreg>,<rtreg>
			UML_EXHc(block, IF_NE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,NE
			return TRUE;


		/* ----- conditional moves ----- */

		case 0x0a:	/* MOVZ - MIPS IV */
			if (RDREG != 0)
			{
				UML_DCMP(block, R64(RTREG), IMM(0));								// dcmp    <rtreg>,0
				UML_DMOVc(block, IF_Z, R64(RDREG), R64(RSREG));						// mov     <rdreg>,<rsreg>,Z
			}
			return TRUE;

		case 0x0b:	/* MOVN - MIPS IV */
			if (RDREG != 0)
			{
				UML_DCMP(block, R64(RTREG), IMM(0));								// dcmp    <rtreg>,0
				UML_DMOVc(block, IF_NZ, R64(RDREG), R64(RSREG));					// mov     <rdreg>,<rsreg>,NZ
			}
			return TRUE;

		case 0x01:	/* MOVF/MOVT - MIPS IV */
			if (RDREG != 0)
			{
				UML_TEST(block, CCR132(31), IMM(fcc_mask[(op >> 18) & 7]));			// test    ccr31,fcc_mask[x]
				UML_DMOVc(block, ((op >> 16) & 1) ? IF_NZ : IF_Z, R64(RDREG), R64(RSREG));
																					// mov     <rdreg>,<rsreg>,NZ/Z
			}
			return TRUE;


		/* ----- jumps and branches ----- */

		case 0x08:	/* JR - MIPS I */
			generate_delay_slot_and_branch(block, compiler, desc, 0);				// <next instruction + hashjmp>
			return TRUE;

		case 0x09:	/* JALR - MIPS I */
			generate_delay_slot_and_branch(block, compiler, desc, 31);				// <next instruction + hashjmp>
			return TRUE;


		/* ----- system calls ----- */

		case 0x0c:	/* SYSCALL - MIPS I */
			UML_EXH(block, mips3->impstate->exception[EXCEPTION_SYSCALL], IMM(0));	// exh     syscall,0
			return TRUE;

		case 0x0d:	/* BREAK - MIPS I */
			UML_EXH(block, mips3->impstate->exception[EXCEPTION_BREAK], IMM(0));	// exh     break,0
			return TRUE;


		/* ----- effective no-ops ----- */

		case 0x0f:	/* SYNC - MIPS II */
			return TRUE;


		/* ----- hi/lo register access ----- */

		case 0x10:	/* MFHI - MIPS I */
			if (RDREG != 0)
				UML_DMOV(block, R64(RDREG), HI64);									// dmov    <rdreg>,hi
			return TRUE;

		case 0x11:	/* MTHI - MIPS I */
			UML_DMOV(block, HI64, R64(RSREG));										// dmov    hi,<rsreg>
			return TRUE;

		case 0x12:	/* MFLO - MIPS I */
			if (RDREG != 0)
				UML_DMOV(block, R64(RDREG), LO64);									// dmov    <rdreg>,lo
			return TRUE;

		case 0x13:	/* MTLO - MIPS I */
			UML_DMOV(block, LO64, R64(RSREG));										// dmov    lo,<rsreg>
			return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    generate_regimm - compile opcodes in the
    'REGIMM' group
-------------------------------------------------*/

static int generate_regimm(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = *desc->opptr.l;
	UINT8 opswitch = RTREG;
	drcuml_codelabel skip;

	switch (opswitch)
	{
		case 0x00:	/* BLTZ */
		case 0x02:	/* BLTZL */
		case 0x10:	/* BLTZAL */
		case 0x12:	/* BLTZALL */
			if (RSREG != 0)
			{
				UML_DCMP(block, R64(RSREG), IMM(0));								// dcmp    <rsreg>,0
				UML_JMPc(block, IF_GE, skip = compiler->labelnum++);				// jmp     skip,GE
				generate_delay_slot_and_branch(block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
				UML_LABEL(block, skip);											// skip:
			}
			return TRUE;

		case 0x01:	/* BGEZ */
		case 0x03:	/* BGEZL */
		case 0x11:	/* BGEZAL */
		case 0x13:	/* BGEZALL */
			if (RSREG != 0)
			{
				UML_DCMP(block, R64(RSREG), IMM(0));								// dcmp    <rsreg>,0
				UML_JMPc(block, IF_L, skip = compiler->labelnum++);					// jmp     skip,L
				generate_delay_slot_and_branch(block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
				UML_LABEL(block, skip);											// skip:
			}
			else
				generate_delay_slot_and_branch(block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
			return TRUE;

		case 0x08:	/* TGEI */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_GE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,GE
			return TRUE;

		case 0x09:	/* TGEIU */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_AE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,AE
			return TRUE;

		case 0x0a:	/* TLTI */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_L, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,L
			return TRUE;

		case 0x0b:	/* TLTIU */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_B, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,B
			return TRUE;

		case 0x0c:	/* TEQI */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_E, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,E
			return TRUE;

		case 0x0e:	/* TNEI */
			UML_DCMP(block, R64(RSREG), IMM(SIMMVAL));								// dcmp    <rsreg>,SIMMVAL
			UML_EXHc(block, IF_NE, mips3->impstate->exception[EXCEPTION_TRAP], IMM(0));// exh     trap,0,NE
			return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    generate_idt - compile opcodes in the IDT-
    specific group
-------------------------------------------------*/

static int generate_idt(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = *desc->opptr.l;
	UINT8 opswitch = op & 0x1f;

	/* only enabled on IDT processors */
	if (mips3->flavor != MIPS3_TYPE_R4650)
		return FALSE;

	switch (opswitch)
	{
		case 0: /* MAD */
			if (RSREG != 0 && RTREG != 0)
			{
				UML_MULS(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));			// muls   i0,i1,rsreg,rtreg
				UML_ADDf(block, FLAGS_C, IREG(0), IREG(0), LO32);					// add    i0,i0,lo
				UML_ADDC(block, IREG(1), IREG(1), HI32);							// addc   i1,i1,hi
				UML_DSEXT4(block, LO64, IREG(0));									// dsext4 lo,i0
				UML_DSEXT4(block, HI64, IREG(1));									// dsext4 hi,i0
			}
			return TRUE;

		case 1: /* MADU */
			if (RSREG != 0 && RTREG != 0)
			{
				UML_MULU(block, IREG(0), IREG(1), R32(RSREG), R32(RTREG));			// mulu   i0,i1,rsreg,rtreg
				UML_ADDf(block, FLAGS_C, IREG(0), IREG(0), LO32);					// add    i0,i0,lo
				UML_ADDC(block, IREG(1), IREG(1), HI32);							// addc   i1,i1,hi
				UML_DSEXT4(block, LO64, IREG(0));									// dsext4 lo,i0
				UML_DSEXT4(block, HI64, IREG(1));									// dsext4 hi,i0
			}
			return TRUE;

		case 2: /* MUL */
			if (RDREG != 0)
			{
				UML_MULS(block, IREG(0), IREG(0), R32(RSREG), R32(RTREG));			// muls   i0,i0,rsreg,rtreg
				UML_DSEXT4(block, R64(RDREG), IREG(0));								// dsext4 rdreg,i0
			}
			return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    generate_set_cop0_reg - generate code to
    handle special COP0 registers
-------------------------------------------------*/

static int generate_set_cop0_reg(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 reg)
{
	int in_delay_slot = ((desc->flags & OPFLAG_IN_DELAY_SLOT) != 0);
	drcuml_codelabel link;

	switch (reg)
	{
		case COP0_Cause:
			UML_INSERT(block, CPR032(COP0_Cause), IREG(0), IMM(0), IMM(~0xfc00));	// insert  [Cause],i0,0,~0xfc00
			compiler->checksoftints = TRUE;
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case COP0_Status:
			generate_update_cycles(block, compiler, IMM(desc->pc), !in_delay_slot);	// <subtract cycles>
			UML_MOV(block, IREG(1), CPR032(COP0_Status));							// mov     i1,[Status]
			UML_MOV(block, CPR032(COP0_Status), IREG(0));							// mov     [Status],i0
			generate_update_mode(block);
			UML_XOR(block, IREG(0), IREG(0), IREG(1));								// xor     i0,i0,i1
			UML_TEST(block, IREG(0), IMM(0x8000));									// test    i0,0x8000
			UML_CALLCc(block, IF_NZ, mips3com_update_cycle_counting, mips3);		// callc   mips3com_update_cycle_counting,mips.core,NZ
			compiler->checkints = TRUE;
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case COP0_Count:
			generate_update_cycles(block, compiler, IMM(desc->pc), !in_delay_slot);	// <subtract cycles>
			UML_MOV(block, CPR032(COP0_Count), IREG(0));							// mov     [Count],i0
			UML_CALLC(block, cfunc_get_cycles, &mips3->impstate->numcycles);		// callc   cfunc_get_cycles,&mips3->impstate->numcycles
			UML_DZEXT4(block, IREG(0), IREG(0));									// dzext4  i0,i0
			UML_DADD(block, IREG(0), IREG(0), IREG(0));								// dadd    i0,i0,i0
			UML_DSUB(block, MEM(&mips3->count_zero_time), MEM(&mips3->impstate->numcycles), IREG(0));
																					// dsub    [count_zero_time],[mips3->impstate->numcycles],i0
			UML_CALLC(block, mips3com_update_cycle_counting, mips3);				// callc   mips3com_update_cycle_counting,mips.core
			return TRUE;

		case COP0_Compare:
			generate_update_cycles(block, compiler, IMM(desc->pc), !in_delay_slot);	// <subtract cycles>
			UML_MOV(block, CPR032(COP0_Compare), IREG(0));							// mov     [Compare],i0
			UML_AND(block, CPR032(COP0_Cause), CPR032(COP0_Cause), IMM(~0x8000));	// and     [Cause],[Cause],~0x8000
			UML_CALLC(block, mips3com_update_cycle_counting, mips3);				// callc   mips3com_update_cycle_counting,mips.core
			return TRUE;

		case COP0_PRId:
			return TRUE;

		case COP0_Config:
			UML_INSERT(block, CPR032(COP0_Config), IREG(0), IMM(0), IMM(0x0007));	// insert  [Config],i0,0,0x0007
			return TRUE;

		case COP0_EntryHi:
			UML_XOR(block, IREG(1), IREG(0), CPR032(reg));							// xor     i1,i0,cpr0[reg]
			UML_MOV(block, CPR032(reg), IREG(0));									// mov     cpr0[reg],i0
			UML_TEST(block, IREG(1), IMM(0xff));									// test    i1,0xff
			UML_JMPc(block, IF_Z, link = compiler->labelnum++);						// jmp     link,z
			UML_CALLC(block, mips3com_unmap_tlb_entries, mips3);					// callc   mips3com_unmap_tlb_entries
			UML_CALLC(block, mips3com_map_tlb_entries, mips3);						// callc   mips3com_map_tlb_entries
			UML_LABEL(block, link);												// link:
			return TRUE;

		default:
			UML_MOV(block, CPR032(reg), IREG(0));									// mov     cpr0[reg],i0
			return TRUE;
	}
}


/*-------------------------------------------------
    generate_get_cop0_reg - generate code to
    read special COP0 registers
-------------------------------------------------*/

static int generate_get_cop0_reg(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 reg)
{
	drcuml_codelabel link1, link2;

	switch (reg)
	{
		case COP0_Count:
			generate_update_cycles(block, compiler, IMM(desc->pc), FALSE);			// <subtract cycles>
			UML_CALLC(block, cfunc_get_cycles, &mips3->impstate->numcycles);		// callc   cfunc_get_cycles,&mips3->impstate->numcycles
			UML_DSUB(block, IREG(0), MEM(&mips3->impstate->numcycles), MEM(&mips3->count_zero_time));
																					// dsub    i0,[numcycles],[count_zero_time]
			UML_DSHR(block, IREG(0), IREG(0), IMM(1));								// dshr    i0,i0,1
			UML_DSEXT4(block, IREG(0), IREG(0));									// dsext4  i0,i0
			return TRUE;

		case COP0_Random:
			generate_update_cycles(block, compiler, IMM(desc->pc), FALSE);			// <subtract cycles>
			UML_CALLC(block, cfunc_get_cycles, &mips3->impstate->numcycles);		// callc   cfunc_get_cycles,&mips3->impstate->numcycles
			UML_DSUB(block, IREG(0), MEM(&mips3->impstate->numcycles), MEM(&mips3->count_zero_time));
																					// dsub    i0,[numcycles],[count_zero_time]
			UML_AND(block, IREG(1), CPR032(COP0_Wired), IMM(0x3f));					// and     i1,[Wired],0x3f
			UML_SUBf(block, IREG(2), IMM(48), IREG(1), FLAGS_BE);					// sub     i2,48,i1,ALL
			UML_JMPc(block, IF_BE, link1 = compiler->labelnum++);					// jmp     link1,BE
			UML_DZEXT4(block, IREG(2), IREG(2));									// dzext4  i2,i2
			UML_DDIVU(block, IREG(0), IREG(2), IREG(0), IREG(2));					// ddivu   i0,i2,i0,i2
			UML_ADD(block, IREG(0), IREG(2), IREG(1));								// add     i0,i2,i1
			UML_DAND(block, IREG(0), IREG(0), IMM(0x3f));							// dand    i0,i0,0x3f
			UML_JMP(block, link2 = compiler->labelnum++);							// jmp     link2
			UML_LABEL(block, link1);											// link1:
			UML_DSEXT4(block, IREG(0), IMM(47));									// dsext4  i0,47
			UML_LABEL(block, link2);											// link2:
			return TRUE;

		default:
			UML_DSEXT4(block, IREG(0), CPR032(reg));								// dsext4  i0,cpr0[reg]
			return TRUE;
	}
}


/*-------------------------------------------------
    generate_cop0 - compile COP0 opcodes
-------------------------------------------------*/

static int generate_cop0(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = *desc->opptr.l;
	UINT8 opswitch = RSREG;
	int skip;

	/* generate an exception if COP0 is disabled unless we are in kernel mode */
	if ((mips3->impstate->mode >> 1) != MODE_KERNEL)
	{
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_COP0));							// test    [Status],SR_COP0
		UML_EXHc(block, IF_Z, mips3->impstate->exception[EXCEPTION_BADCOP], IMM(0));// exh     cop,0,Z
	}

	switch (opswitch)
	{
		case 0x00:	/* MFCz */
			if (RTREG != 0)
			{
				generate_get_cop0_reg(block, compiler, desc, RDREG);				// <get cop0 reg>
				UML_DSEXT4(block, R64(RTREG), IREG(0));								// dsext4  <rtreg>,i0
			}
			return TRUE;

		case 0x01:	/* DMFCz */
			if (RTREG != 0)
			{
				generate_get_cop0_reg(block, compiler, desc, RDREG);				// <get cop0 reg>
				UML_DMOV(block, R64(RTREG), IREG(0));								// dmov    <rtreg>,i0
			}
			return TRUE;

		case 0x02:	/* CFCz */
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), CCR032(RDREG));						// dsext4  <rtreg>,ccr0[rdreg]
			return TRUE;

		case 0x04:	/* MTCz */
			UML_DSEXT4(block, IREG(0), R32(RTREG));									// dsext4  i0,<rtreg>
			generate_set_cop0_reg(block, compiler, desc, RDREG);					// <set cop0 reg>
			return TRUE;

		case 0x05:	/* DMTCz */
			UML_DMOV(block, IREG(0), R64(RTREG));									// dmov    i0,<rtreg>
			generate_set_cop0_reg(block, compiler, desc, RDREG);					// <set cop0 reg>
			return TRUE;

		case 0x06:	/* CTCz */
			UML_DSEXT4(block, CCR064(RDREG), R32(RTREG));							// dsext4  ccr0[rdreg],<rtreg>
			return TRUE;

		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:	/* COP */
			switch (op & 0x01ffffff)
			{
				case 0x01:	/* TLBR */
					UML_CALLC(block, mips3com_tlbr, mips3);							// callc   mips3com_tlbr,mips3
					return TRUE;

				case 0x02:	/* TLBWI */
					UML_CALLC(block, mips3com_tlbwi, mips3);						// callc   mips3com_tlbwi,mips3
					return TRUE;

				case 0x06:	/* TLBWR */
					UML_CALLC(block, mips3com_tlbwr, mips3);						// callc   mips3com_tlbwr,mips3
					return TRUE;

				case 0x08:	/* TLBP */
					UML_CALLC(block, mips3com_tlbp, mips3);							// callc   mips3com_tlbp,mips3
					return TRUE;

				case 0x18:	/* ERET */
					UML_MOV(block, IREG(0), CPR032(COP0_Status));					// mov     i0,[Status]
					UML_TEST(block, IREG(0), IMM(SR_ERL));							// test    i0,SR_ERL
					UML_JMPc(block, IF_NZ, skip = compiler->labelnum++);			// jmp     skip,nz
					UML_AND(block, IREG(0), IREG(0), IMM(~SR_EXL));					// and     i0,i0,~SR_EXL
					UML_MOV(block, CPR032(COP0_Status), IREG(0));					// mov     [Status],i0
					generate_update_mode(block);
					compiler->checkints = TRUE;
					generate_update_cycles(block, compiler, CPR032(COP0_EPC), TRUE);// <subtract cycles>
					UML_HASHJMP(block, MEM(&mips3->impstate->mode), CPR032(COP0_EPC), mips3->impstate->nocode);
																					// hashjmp <mode>,[EPC],nocode
					UML_LABEL(block, skip);										// skip:
					UML_AND(block, IREG(0), IREG(0), IMM(~SR_ERL));					// and     i0,i0,~SR_ERL
					UML_MOV(block, CPR032(COP0_Status), IREG(0));					// mov     [Status],i0
					generate_update_mode(block);
					compiler->checkints = TRUE;
					generate_update_cycles(block, compiler, CPR032(COP0_ErrorPC), TRUE);
																					// <subtract cycles>
					UML_HASHJMP(block, MEM(&mips3->impstate->mode), CPR032(COP0_ErrorPC), mips3->impstate->nocode);
																					// hashjmp <mode>,[EPC],nocode
					return TRUE;

				case 0x20:	/* WAIT */
					return TRUE;
			}
			break;
	}

	return FALSE;
}



/***************************************************************************
    COP1 RECOMPILATION
***************************************************************************/

/*-------------------------------------------------
    generate_cop1 - compile COP1 opcodes
-------------------------------------------------*/

static int generate_cop1(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = *desc->opptr.l;
	drcuml_codelabel skip;
	int condition;

	/* generate an exception if COP1 is disabled */
	if (mips3->impstate->drcoptions & MIPS3DRC_STRICT_COP1)
	{
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_COP1));							// test    [Status],SR_COP1
		UML_EXHc(block, IF_Z, mips3->impstate->exception[EXCEPTION_BADCOP], IMM(0));// exh     cop,0,Z
	}

	switch (RSREG)
	{
		case 0x00:	/* MFCz - MIPS I */
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), FPR32(RDREG));						// dsext4  <rtreg>,fpr[rdreg]
			return TRUE;

		case 0x01:	/* DMFCz - MIPS III */
			if (RTREG != 0)
				UML_DMOV(block, R64(RTREG), FPR64(RDREG));							// dmov    <rtreg>,fpr[rdreg]
			return TRUE;

		case 0x02:	/* CFCz - MIPS I */
			if (RTREG != 0)
				UML_DSEXT4(block, R64(RTREG), CCR132(RDREG));						// dsext4  <rtreg>,ccr132[rdreg]
			return TRUE;

		case 0x04:	/* MTCz - MIPS I */
			UML_MOV(block, FPR32(RDREG), R32(RTREG));								// mov     fpr[rdreg],<rtreg>
			return TRUE;

		case 0x05:	/* DMTCz - MIPS III */
			UML_DMOV(block, FPR64(RDREG), R64(RTREG));								// dmov    fpr[rdreg],<rtreg>
			return TRUE;

		case 0x06:	/* CTCz - MIPS I */
			if (RDREG != 31)
				UML_DSEXT4(block, CCR164(RDREG), R32(RTREG));						// dsext4  ccr1[rdreg],<rtreg>
			else
			{
				UML_XOR(block, IREG(0), CCR132(31), R32(RTREG));					// xor     i0,ccr1[31],<rtreg>
				UML_DSEXT4(block, CCR164(31), R32(RTREG));							// dsext4  ccr1[31],<rtreg>
				UML_TEST(block, IREG(0), IMM(3));									// test    i0,3
				UML_JMPc(block, IF_Z, skip = compiler->labelnum++);					// jmp     skip,Z
				UML_AND(block, IREG(0), CCR132(31), IMM(3));						// and     i0,ccr1[31],3
				UML_LOAD1U(block, IREG(0), &mips3->impstate->fpmode[0], IREG(0));	// load1u  i0,fpmode,i0
				UML_SETFMOD(block, IREG(0));										// setfmod i0
				UML_LABEL(block, skip);											// skip:
			}
			return TRUE;

		case 0x08:	/* BC */
			switch ((op >> 16) & 3)
			{
				case 0x00:	/* BCzF - MIPS I */
				case 0x02:	/* BCzFL - MIPS II */
					UML_TEST(block, CCR132(31), IMM(FCCMASK(op >> 18)));			// test    ccr1[31],fccmask[which]
					UML_JMPc(block, IF_NZ, skip = compiler->labelnum++);			// jmp     skip,NZ
					generate_delay_slot_and_branch(block, compiler, desc, 0);		// <next instruction + hashjmp>
					UML_LABEL(block, skip);										// skip:
					return TRUE;

				case 0x01:	/* BCzT - MIPS I */
				case 0x03:	/* BCzTL - MIPS II */
					UML_TEST(block, CCR132(31), IMM(FCCMASK(op >> 18)));			// test    ccr1[31],fccmask[which]
					UML_JMPc(block, IF_Z, skip = compiler->labelnum++);				// jmp     skip,Z
					generate_delay_slot_and_branch(block, compiler, desc, 0);		// <next instruction + hashjmp>
					UML_LABEL(block, skip);										// skip:
					return TRUE;
			}
			break;

		default:
			switch (op & 0x3f)
			{
				case 0x00:
					if (IS_SINGLE(op))	/* ADD.S - MIPS I */
						UML_FSADD(block, FPR32(FDREG), FPR32(FSREG), FPR32(FTREG));	// fsadd   <fdreg>,<fsreg>,<ftreg>
					else				/* ADD.D - MIPS I */
						UML_FDADD(block, FPR64(FDREG), FPR64(FSREG), FPR64(FTREG));	// fdadd   <fdreg>,<fsreg>,<ftreg>
					return TRUE;

				case 0x01:
					if (IS_SINGLE(op))	/* SUB.S - MIPS I */
						UML_FSSUB(block, FPR32(FDREG), FPR32(FSREG), FPR32(FTREG));	// fssub   <fdreg>,<fsreg>,<ftreg>
					else				/* SUB.D - MIPS I */
						UML_FDSUB(block, FPR64(FDREG), FPR64(FSREG), FPR64(FTREG));	// fdsub   <fdreg>,<fsreg>,<ftreg>
					return TRUE;

				case 0x02:
					if (IS_SINGLE(op))	/* MUL.S - MIPS I */
						UML_FSMUL(block, FPR32(FDREG), FPR32(FSREG), FPR32(FTREG));	// fsmul   <fdreg>,<fsreg>,<ftreg>
					else				/* MUL.D - MIPS I */
						UML_FDMUL(block, FPR64(FDREG), FPR64(FSREG), FPR64(FTREG));	// fdmul   <fdreg>,<fsreg>,<ftreg>
					return TRUE;

				case 0x03:
					if (IS_SINGLE(op))	/* DIV.S - MIPS I */
						UML_FSDIV(block, FPR32(FDREG), FPR32(FSREG), FPR32(FTREG));	// fsdiv   <fdreg>,<fsreg>,<ftreg>
					else				/* DIV.D - MIPS I */
						UML_FDDIV(block, FPR64(FDREG), FPR64(FSREG), FPR64(FTREG));	// fddiv   <fdreg>,<fsreg>,<ftreg>
					return TRUE;

				case 0x04:
					if (IS_SINGLE(op))	/* SQRT.S - MIPS II */
						UML_FSSQRT(block, FPR32(FDREG), FPR32(FSREG));				// fssqrt  <fdreg>,<fsreg>
					else				/* SQRT.D - MIPS II */
						UML_FDSQRT(block, FPR64(FDREG), FPR64(FSREG));				// fdsqrt  <fdreg>,<fsreg>
					return TRUE;

				case 0x05:
					if (IS_SINGLE(op))	/* ABS.S - MIPS I */
						UML_FSABS(block, FPR32(FDREG), FPR32(FSREG));				// fsabs   <fdreg>,<fsreg>
					else				/* ABS.D - MIPS I */
						UML_FDABS(block, FPR64(FDREG), FPR64(FSREG));				// fdabs   <fdreg>,<fsreg>
					return TRUE;

				case 0x06:
					if (IS_SINGLE(op))	/* MOV.S - MIPS I */
						UML_FSMOV(block, FPR32(FDREG), FPR32(FSREG));				// fsmov   <fdreg>,<fsreg>
					else				/* MOV.D - MIPS I */
						UML_FDMOV(block, FPR64(FDREG), FPR64(FSREG));				// fdmov   <fdreg>,<fsreg>
					return TRUE;

				case 0x07:
					if (IS_SINGLE(op))	/* NEG.S - MIPS I */
						UML_FSNEG(block, FPR32(FDREG), FPR32(FSREG));				// fsneg   <fdreg>,<fsreg>
					else				/* NEG.D - MIPS I */
						UML_FDNEG(block, FPR64(FDREG), FPR64(FSREG));				// fdneg   <fdreg>,<fsreg>
					return TRUE;

				case 0x08:
					if (IS_SINGLE(op))	/* ROUND.L.S - MIPS III */
						UML_FSTOI8R(block, FPR64(FDREG), FPR32(FSREG));				// fstoi8r <fdreg>,<fsreg>
					else				/* ROUND.L.D - MIPS III */
						UML_FDTOI8R(block, FPR64(FDREG), FPR64(FSREG));				// fdtoi8r <fdreg>,<fsreg>
					return TRUE;

				case 0x09:
					if (IS_SINGLE(op))	/* TRUNC.L.S - MIPS III */
						UML_FSTOI8T(block, FPR64(FDREG), FPR32(FSREG));				// fstoi8t <fdreg>,<fsreg>
					else				/* TRUNC.L.D - MIPS III */
						UML_FDTOI8T(block, FPR64(FDREG), FPR64(FSREG));				// fdtoi8t <fdreg>,<fsreg>
					return TRUE;

				case 0x0a:
					if (IS_SINGLE(op))	/* CEIL.L.S - MIPS III */
						UML_FSTOI8C(block, FPR64(FDREG), FPR32(FSREG));				// fstoi8c <fdreg>,<fsreg>
					else				/* CEIL.L.D - MIPS III */
						UML_FDTOI8C(block, FPR64(FDREG), FPR64(FSREG));				// fdtoi8c <fdreg>,<fsreg>
					return TRUE;

				case 0x0b:
					if (IS_SINGLE(op))	/* FLOOR.L.S - MIPS III */
						UML_FSTOI8F(block, FPR64(FDREG), FPR32(FSREG));				// fstoi8f <fdreg>,<fsreg>
					else				/* FLOOR.L.D - MIPS III */
						UML_FDTOI8F(block, FPR64(FDREG), FPR64(FSREG));				// fdtoi8f <fdreg>,<fsreg>
					return TRUE;

				case 0x0c:
					if (IS_SINGLE(op))	/* ROUND.W.S - MIPS II */
						UML_FSTOI4R(block, FPR32(FDREG), FPR32(FSREG));				// fstoi4r <fdreg>,<fsreg>
					else				/* ROUND.W.D - MIPS II */
						UML_FDTOI4R(block, FPR32(FDREG), FPR64(FSREG));				// fdtoi4r <fdreg>,<fsreg>
					return TRUE;

				case 0x0d:
					if (IS_SINGLE(op))	/* TRUNC.W.S - MIPS II */
						UML_FSTOI4T(block, FPR32(FDREG), FPR32(FSREG));				// fstoi4t <fdreg>,<fsreg>
					else				/* TRUNC.W.D - MIPS II */
						UML_FDTOI4T(block, FPR32(FDREG), FPR64(FSREG));				// fdtoi4t <fdreg>,<fsreg>
					return TRUE;

				case 0x0e:
					if (IS_SINGLE(op))	/* CEIL.W.S - MIPS II */
						UML_FSTOI4C(block, FPR32(FDREG), FPR32(FSREG));				// fstoi4c <fdreg>,<fsreg>
					else				/* CEIL.W.D - MIPS II */
						UML_FDTOI4C(block, FPR32(FDREG), FPR64(FSREG));				// fdtoi4c <fdreg>,<fsreg>
					return TRUE;

				case 0x0f:
					if (IS_SINGLE(op))	/* FLOOR.W.S - MIPS II */
						UML_FSTOI4F(block, FPR32(FDREG), FPR32(FSREG));				// fstoi4f <fdreg>,<fsreg>
					else				/* FLOOR.W.D - MIPS II */
						UML_FDTOI4F(block, FPR32(FDREG), FPR64(FSREG));				// fdtoi4f <fdreg>,<fsreg>
					return TRUE;

				case 0x11:
					condition = ((op >> 16) & 1) ? IF_NZ : IF_Z;
					UML_TEST(block, CCR132(31), IMM(FCCMASK(op >> 18)));			// test    ccr31,fccmask[op]
					if (IS_SINGLE(op))	/* MOVT/F.S - MIPS IV */
						UML_FSMOVc(block, condition, FPR32(FDREG), FPR32(FSREG));	// fsmov   <fdreg>,<fsreg>,condition
					else				/* MOVT/F.D - MIPS IV */
						UML_FDMOVc(block, condition, FPR64(FDREG), FPR64(FSREG));	// fdmov   <fdreg>,<fsreg>,condition
					return TRUE;

				case 0x12:
					UML_DCMP(block, R64(RTREG), IMM(0));							// dcmp    <rtreg>,0
					if (IS_SINGLE(op))	/* MOVZ.S - MIPS IV */
						UML_FSMOVc(block, IF_Z, FPR32(FDREG), FPR32(FSREG));		// fsmov   <fdreg>,<fsreg>,Z
					else				/* MOVZ.D - MIPS IV */
						UML_FDMOVc(block, IF_Z, FPR64(FDREG), FPR64(FSREG));		// fdmov   <fdreg>,<fsreg>,Z
					return TRUE;

				case 0x13:
					UML_DCMP(block, R64(RTREG), IMM(0));							// dcmp    <rtreg>,0
					if (IS_SINGLE(op))	/* MOVN.S - MIPS IV */
						UML_FSMOVc(block, IF_NZ, FPR32(FDREG), FPR32(FSREG));		// fsmov   <fdreg>,<fsreg>,NZ
					else				/* MOVN.D - MIPS IV */
						UML_FDMOVc(block, IF_NZ, FPR64(FDREG), FPR64(FSREG));		// fdmov   <fdreg>,<fsreg>,NZ
					return TRUE;

				case 0x15:
					if (IS_SINGLE(op))	/* RECIP.S - MIPS IV */
						UML_FSRECIP(block, FPR32(FDREG), FPR32(FSREG));				// fsrecip <fdreg>,<fsreg>
					else				/* RECIP.D - MIPS IV */
						UML_FDRECIP(block, FPR64(FDREG), FPR64(FSREG));				// fdrecip <fdreg>,<fsreg>
					return TRUE;

				case 0x16:
					if (IS_SINGLE(op))	/* RSQRT.S - MIPS IV */
						UML_FSRSQRT(block, FPR32(FDREG), FPR32(FSREG));				// fsrsqrt <fdreg>,<fsreg>
					else				/* RSQRT.D - MIPS IV */
						UML_FDRSQRT(block, FPR64(FDREG), FPR64(FSREG));				// fdrsqrt <fdreg>,<fsreg>
					return TRUE;

				case 0x20:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.S.W - MIPS I */
							UML_FSFRI4(block, FPR32(FDREG), FPR32(FSREG));			// fsfri4  <fdreg>,<fsreg>
						else				/* CVT.S.L - MIPS I */
							UML_FSFRI8(block, FPR32(FDREG), FPR64(FSREG));			// fsfri8  <fdreg>,<fsreg>
					}
					else					/* CVT.S.D - MIPS I */
						UML_FSFRFD(block, FPR32(FDREG), FPR64(FSREG));				// fsfrfd  <fdreg>,<fsreg>
					return TRUE;

				case 0x21:
					if (IS_INTEGRAL(op))
					{
						if (IS_SINGLE(op))	/* CVT.D.W - MIPS I */
							UML_FDFRI4(block, FPR64(FDREG), FPR32(FSREG));			// fdfri4  <fdreg>,<fsreg>
						else				/* CVT.D.L - MIPS I */
							UML_FDFRI8(block, FPR64(FDREG), FPR64(FSREG));			// fdfri8  <fdreg>,<fsreg>
					}
					else					/* CVT.D.S - MIPS I */
						UML_FDFRFS(block, FPR64(FDREG), FPR32(FSREG));				// fdfrfs  <fdreg>,<fsreg>
					return TRUE;

				case 0x24:
					if (IS_SINGLE(op))	/* CVT.W.S - MIPS I */
						UML_FSTOI4(block, FPR32(FDREG), FPR32(FSREG));				// fstoi4  <fdreg>,<fsreg>
					else				/* CVT.W.D - MIPS I */
						UML_FDTOI4(block, FPR32(FDREG), FPR64(FSREG));				// fdtoi4  <fdreg>,<fsreg>
					return TRUE;

				case 0x25:
					if (IS_SINGLE(op))	/* CVT.L.S - MIPS I */
						UML_FSTOI8(block, FPR64(FDREG), FPR32(FSREG));				// fstoi8  <fdreg>,<fsreg>
					else				/* CVT.L.D - MIPS I */
						UML_FDTOI8(block, FPR64(FDREG), FPR64(FSREG));				// fdtoi8  <fdreg>,<fsreg>
					return TRUE;

				case 0x30:
				case 0x38:				/* C.F.S/D - MIPS I */
					UML_AND(block, CCR132(31), CCR132(31), IMM(~FCCMASK(op >> 8)));	// and     ccr31,ccr31,~fccmask[op]
					return TRUE;

				case 0x31:
				case 0x39:
					if (IS_SINGLE(op))	/* C.UN.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.UN.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_un_table);
																					// flags   ccr31,fccmask[op],c_eq_table
					return TRUE;

				case 0x32:
				case 0x3a:
					if (IS_SINGLE(op))	/* C.EQ.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.EQ.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_eq_table);
																					// flags   ccr31,fccmask[op],c_eq_table
					return TRUE;

				case 0x33:
				case 0x3b:
					if (IS_SINGLE(op))	/* C.UEQ.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.UEQ.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_ueq_table);
																					// flags   ccr31,fccmask[op],c_eq_table
					return TRUE;

				case 0x34:
				case 0x3c:
					if (IS_SINGLE(op))	/* C.OLT.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.OLT.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_olt_table);
																					// flags   ccr31,fccmask[op],c_lt_table
					return TRUE;

				case 0x35:
				case 0x3d:
					if (IS_SINGLE(op))	/* C.ULT.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.ULT.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_ult_table);
																					// flags   ccr31,fccmask[op],c_lt_table
					return TRUE;

				case 0x36:
				case 0x3e:
					if (IS_SINGLE(op))	/* C.OLE.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.OLE.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_ole_table);
																					// flags   ccr31,fccmask[op],c_le_table
					return TRUE;

				case 0x37:
				case 0x3f:
					if (IS_SINGLE(op))	/* C.ULE.S - MIPS I */
						UML_FSCMP(block, FPR32(FSREG), FPR32(FTREG));				// fscmp   <fsreg>,<ftreg>
					else				/* C.ULE.D - MIPS I */
						UML_FDCMP(block, FPR64(FSREG), FPR64(FTREG));				// fdcmp   <fsreg>,<ftreg>
					UML_FLAGS(block, CCR132(31), FCCMASK(op >> 8), mips3->impstate->c_ule_table);
																					// flags   ccr31,fccmask[op],c_le_table
					return TRUE;
			}
			break;
	}
	return FALSE;
}



/***************************************************************************
    COP1X RECOMPILATION
***************************************************************************/

/*-------------------------------------------------
    generate_cop1x - compile COP1X opcodes
-------------------------------------------------*/

static int generate_cop1x(drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	int in_delay_slot = ((desc->flags & OPFLAG_IN_DELAY_SLOT) != 0);
	UINT32 op = *desc->opptr.l;

	if (mips3->impstate->drcoptions & MIPS3DRC_STRICT_COP1)
	{
		UML_TEST(block, CPR032(COP0_Status), IMM(SR_COP1));							// test    [Status],SR_COP1
		UML_EXHc(block, IF_Z, mips3->impstate->exception[EXCEPTION_BADCOP], IMM(0));// exh     cop,0,Z
	}

	switch (op & 0x3f)
	{
		case 0x00:		/* LWXC1 - MIPS IV */
			UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));						// add     i0,<rsreg>,<rtreg>
			UML_CALLH(block, mips3->impstate->read32[mips3->impstate->mode >> 1]);	// callh   read32
			UML_MOV(block, FPR32(RDREG), IREG(0));									// mov     <cpr1_rd>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x01:		/* LDXC1 - MIPS IV */
			UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));						// add     i0,<rsreg>,<rtreg>
			UML_CALLH(block, mips3->impstate->read64[mips3->impstate->mode >> 1]);	// callh   read64
			UML_DMOV(block, FPR64(RDREG), IREG(0));									// dmov    <cpr1_rd>,i0
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x08:		/* SWXC1 - MIPS IV */
			UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));						// add     i0,<rsreg>,<rtreg>
			UML_MOV(block, IREG(1), FPR32(RTREG));									// mov     i1,<cpr1_rt>
			UML_CALLH(block, mips3->impstate->write32[mips3->impstate->mode >> 1]);	// callh   write32
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x09:		/* SDXC1 - MIPS IV */
			UML_ADD(block, IREG(0), R32(RSREG), R32(RTREG));						// add     i0,<rsreg>,<rtreg>
			UML_DMOV(block, IREG(1), FPR64(RTREG));									// dmov    i1,<cpr1_rt>
			UML_CALLH(block, mips3->impstate->write64[mips3->impstate->mode >> 1]);	// callh   write64
			if (!in_delay_slot)
				generate_update_cycles(block, compiler, IMM(desc->pc + 4), TRUE);
			return TRUE;

		case 0x0f:		/* PREFX */
			return TRUE;

		case 0x20:		/* MADD.S - MIPS IV */
			UML_FSMUL(block, FREG(0), FPR32(FSREG), FPR32(FTREG));					// fsmul   f0,<fsreg>,<ftreg>
			UML_FSADD(block, FPR32(FDREG), FREG(0), FPR32(FRREG));					// fsadd   <fdreg>,f0,<frreg>
			return TRUE;

		case 0x21:		/* MADD.D - MIPS IV */
			UML_FDMUL(block, FREG(0), FPR64(FSREG), FPR64(FTREG));					// fdmul   f0,<fsreg>,<ftreg>
			UML_FDADD(block, FPR64(FDREG), FREG(0), FPR64(FRREG));					// fdadd   <fdreg>,f0,<frreg>
			return TRUE;

		case 0x28:		/* MSUB.S - MIPS IV */
			UML_FSMUL(block, FREG(0), FPR32(FSREG), FPR32(FTREG));					// fsmul   f0,<fsreg>,<ftreg>
			UML_FSSUB(block, FPR32(FDREG), FREG(0), FPR32(FRREG));					// fssub   <fdreg>,f0,<frreg>
			return TRUE;

		case 0x29:		/* MSUB.D - MIPS IV */
			UML_FDMUL(block, FREG(0), FPR64(FSREG), FPR64(FTREG));					// fdmul   f0,<fsreg>,<ftreg>
			UML_FDSUB(block, FPR64(FDREG), FREG(0), FPR64(FRREG));					// fdsub   <fdreg>,f0,<frreg>
			return TRUE;

		case 0x30:		/* NMADD.S - MIPS IV */
			UML_FSMUL(block, FREG(0), FPR32(FSREG), FPR32(FTREG));					// fsmul   f0,<fsreg>,<ftreg>
			UML_FSADD(block, FREG(0), FREG(0), FPR32(FRREG));						// fsadd   f0,f0,<frreg>
			UML_FSNEG(block, FPR32(FDREG), FREG(0));								// fsneg   <fdreg>,f0
			return TRUE;

		case 0x31:		/* NMADD.D - MIPS IV */
			UML_FDMUL(block, FREG(0), FPR64(FSREG), FPR64(FTREG));					// fdmul   f0,<fsreg>,<ftreg>
			UML_FDADD(block, FREG(0), FREG(0), FPR64(FRREG));						// fdadd   f0,f0,<frreg>
			UML_FDNEG(block, FPR64(FDREG), FREG(0));								// fdneg   <fdreg>,f0
			return TRUE;

		case 0x38:		/* NMSUB.S - MIPS IV */
			UML_FSMUL(block, FREG(0), FPR32(FSREG), FPR32(FTREG));					// fsmul   f0,<fsreg>,<ftreg>
			UML_FSSUB(block, FPR32(FDREG), FPR32(FRREG), FREG(0));					// fssub   <fdreg>,<frreg>,f0
			return TRUE;

		case 0x39:		/* NMSUB.D - MIPS IV */
			UML_FDMUL(block, FREG(0), FPR64(FSREG), FPR64(FTREG));					// fdmul   f0,<fsreg>,<ftreg>
			UML_FDSUB(block, FPR64(FDREG), FPR64(FRREG), FREG(0));					// fdsub   <fdreg>,<frreg>,f0
			return TRUE;

		default:
			fprintf(stderr, "cop1x %X\n", op);
			break;
	}
	return FALSE;
}



/***************************************************************************
    CODE LOGGING HELPERS
***************************************************************************/

/*-------------------------------------------------
    log_add_disasm_comment - add a comment
    including disassembly of a MIPS instruction
-------------------------------------------------*/

static void log_add_disasm_comment(drcuml_block *block, UINT32 pc, UINT32 op)
{
#if (LOG_UML)
	char buffer[100];
	dasmmips3(buffer, pc, op);
	UML_COMMENT(block, "%08X: %s", pc, buffer);										// comment
#endif
}


/*-------------------------------------------------
    log_desc_flags_to_string - generate a string
    representing the instruction description
    flags
-------------------------------------------------*/

static const char *log_desc_flags_to_string(UINT32 flags)
{
	static char tempbuf[30];
	char *dest = tempbuf;

	/* branches */
	if (flags & OPFLAG_IS_UNCONDITIONAL_BRANCH)
		*dest++ = 'U';
	else if (flags & OPFLAG_IS_CONDITIONAL_BRANCH)
		*dest++ = 'C';
	else
		*dest++ = '.';

	/* intrablock branches */
	*dest++ = (flags & OPFLAG_INTRABLOCK_BRANCH) ? 'i' : '.';

	/* branch targets */
	*dest++ = (flags & OPFLAG_IS_BRANCH_TARGET) ? 'B' : '.';

	/* delay slots */
	*dest++ = (flags & OPFLAG_IN_DELAY_SLOT) ? 'D' : '.';

	/* exceptions */
	if (flags & OPFLAG_WILL_CAUSE_EXCEPTION)
		*dest++ = 'E';
	else if (flags & OPFLAG_CAN_CAUSE_EXCEPTION)
		*dest++ = 'e';
	else
		*dest++ = '.';

	/* read/write */
	if (flags & OPFLAG_READS_MEMORY)
		*dest++ = 'R';
	else if (flags & OPFLAG_WRITES_MEMORY)
		*dest++ = 'W';
	else
		*dest++ = '.';

	/* TLB validation */
	*dest++ = (flags & OPFLAG_VALIDATE_TLB) ? 'V' : '.';

	/* TLB modification */
	*dest++ = (flags & OPFLAG_MODIFIES_TRANSLATION) ? 'T' : '.';

	/* redispatch */
	*dest++ = (flags & OPFLAG_REDISPATCH) ? 'R' : '.';
	return tempbuf;
}


/*-------------------------------------------------
    log_register_list - log a list of GPR registers
-------------------------------------------------*/

static void log_register_list(drcuml_state *drcuml, const char *string, UINT64 gprmask, UINT64 fprmask)
{
	int count = 0;
	int regnum;

	/* skip if nothing */
	if ((gprmask & ~1) == 0 && fprmask == 0)
		return;

	drcuml_log_printf(drcuml, "[%s:", string);

	for (regnum = 1; regnum < 32; regnum++)
		if (gprmask & ((UINT64)1 << regnum))
			drcuml_log_printf(drcuml, "%sr%d", (count++ == 0) ? "" : ",", regnum);
	if (gprmask & ((UINT64)1 << REG_LO))
		drcuml_log_printf(drcuml, "%slo", (count++ == 0) ? "" : ",");
	if (gprmask & ((UINT64)1 << REG_HI))
		drcuml_log_printf(drcuml, "%shi", (count++ == 0) ? "" : ",");

	for (regnum = 0; regnum < 32; regnum++)
		if (fprmask & ((UINT64)1 << regnum))
			drcuml_log_printf(drcuml, "%sfpr%d", (count++ == 0) ? "" : ",", regnum);
	if (fprmask & REGFLAG_FCC)
		drcuml_log_printf(drcuml, "%sfcc", (count++ == 0) ? "" : ",");
	drcuml_log_printf(drcuml, "] ");
}


/*-------------------------------------------------
    log_opcode_desc - log a list of descriptions
-------------------------------------------------*/

static void log_opcode_desc(drcuml_state *drcuml, const opcode_desc *desclist, int indent)
{
	/* open the file, creating it if necessary */
	if (indent == 0)
		drcuml_log_printf(drcuml, "\nDescriptor list @ %08X\n", desclist->pc);

	/* output each descriptor */
	for ( ; desclist != NULL; desclist = desclist->next)
	{
		char buffer[100];

		/* disassemle the current instruction and output it to the log */
#if (LOG_UML || LOG_NATIVE)
		if (desclist->flags & OPFLAG_VIRTUAL_NOOP)
			strcpy(buffer, "<virtual nop>");
		else
			dasmmips3(buffer, desclist->pc, *desclist->opptr.l);
#else
		strcpy(buffer, "???");
#endif
		drcuml_log_printf(drcuml, "%08X [%08X] t:%08X f:%s: %-30s", desclist->pc, desclist->physpc, desclist->targetpc, log_desc_flags_to_string(desclist->flags), buffer);

		/* output register states */
		log_register_list(drcuml, "use", desclist->gpr.used, desclist->fpr.used);
		log_register_list(drcuml, "mod", desclist->gpr.modified, desclist->fpr.modified);
		log_register_list(drcuml, "lrd", desclist->gpr.liveread, desclist->fpr.liveread);
		log_register_list(drcuml, "lwr", desclist->gpr.livewrite, desclist->fpr.livewrite);
		drcuml_log_printf(drcuml, "\n");

		/* if we have a delay slot, output it recursively */
		if (desclist->delay != NULL)
			log_opcode_desc(drcuml, desclist->delay, indent + 1);

		/* at the end of a sequence add a dividing line */
		if (desclist->flags & OPFLAG_END_SEQUENCE)
			drcuml_log_printf(drcuml, "-----\n");
	}
}



/***************************************************************************
    R4600 VARIANTS
***************************************************************************/

#if (HAS_R4600)
static void r4600be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4600, TRUE, index, clock, config, irqcallback);
}

static void r4600le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4600, FALSE, index, clock, config, irqcallback);
}

void r4600be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4600be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (big)");			break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void r4600le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4600le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4600 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R4650 VARIANTS
***************************************************************************/

#if (HAS_R4650)
static void r4650be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4650, TRUE, index, clock, config, irqcallback);
}

static void r4650le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4650, FALSE, index, clock, config, irqcallback);
}

void r4650be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4650be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (big)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void r4650le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4650le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "IDT R4650 (little)");	break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R4700 VARIANTS
***************************************************************************/

#if (HAS_R4700)
static void r4700be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4700, TRUE, index, clock, config, irqcallback);
}

static void r4700le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R4700, FALSE, index, clock, config, irqcallback);
}

void r4700be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4700be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (big)");			break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void r4700le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r4700le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R4700 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    R5000 VARIANTS
***************************************************************************/

#if (HAS_R5000)
static void r5000be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R5000, TRUE, index, clock, config, irqcallback);
}

static void r5000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_R5000, FALSE, index, clock, config, irqcallback);
}

void r5000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r5000be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (big)");			break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void r5000le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = r5000le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "R5000 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    QED5271 VARIANTS
***************************************************************************/

#if (HAS_QED5271)
static void qed5271be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_QED5271, TRUE, index, clock, config, irqcallback);
}

static void qed5271le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_QED5271, FALSE, index, clock, config, irqcallback);
}

void qed5271be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = qed5271be_init;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (big)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void qed5271le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = qed5271le_init;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "QED5271 (little)");	break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    RM7000 VARIANTS
***************************************************************************/

#if (HAS_RM7000)
static void rm7000be_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_RM7000, TRUE, index, clock, config, irqcallback);
}

static void rm7000le_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	mips3_init(MIPS3_TYPE_RM7000, FALSE, index, clock, config, irqcallback);
}

void rm7000be_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = rm7000be_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (big)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}

void rm7000le_get_info(UINT32 state, cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_INIT:							info->init = rm7000le_init;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "RM7000 (little)");		break;

		/* --- everything else is handled generically --- */
		default:										mips3_get_info(state, info);			break;
	}
}
#endif



/***************************************************************************
    DISASSEMBLERS
***************************************************************************/

#if !defined(MAME_DEBUG) && (LOG_UML || LOG_NATIVE)
#include "mips3dsm.c"
#endif

