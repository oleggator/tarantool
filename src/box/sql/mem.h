#pragma once
/*
 * Copyright 2010-2021, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "box/field_def.h"

struct sql;
struct Vdbe;
struct region;
struct mpstream;
struct VdbeFrame;

/*
 * Internally, the vdbe manipulates nearly all SQL values as Mem
 * structures. Each Mem struct may cache multiple representations (string,
 * integer etc.) of the same value.
 */
struct Mem {
	union MemValue {
		double r;	/* Real value used when MEM_Real is set in flags */
		i64 i;		/* Integer value used when MEM_Int is set in flags */
		uint64_t u;	/* Unsigned integer used when MEM_UInt is set. */
		bool b;         /* Boolean value used when MEM_Bool is set in flags */
		int nZero;	/* Used when bit MEM_Zero is set in flags */
		void *p;	/* Generic pointer */
		/**
		 * A pointer to function implementation.
		 * Used only when flags==MEM_Agg.
		 */
		struct func *func;
		struct VdbeFrame *pFrame;	/* Used when flags==MEM_Frame */
	} u;
	u32 flags;		/* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
	/** Subtype for this value. */
	enum sql_subtype subtype;
	/**
	 * If value is fetched from tuple, then this property
	 * contains type of corresponding space's field. If it's
	 * value field_type_MAX then we can rely on on format
	 * (msgpack) type which is represented by 'flags'.
	 */
	enum field_type field_type;
	int n;			/* size (in bytes) of string value, excluding trailing '\0' */
	char *z;		/* String or BLOB value */
	/* ShallowCopy only needs to copy the information above */
	char *zMalloc;		/* Space to hold MEM_Str or MEM_Blob if szMalloc>0 */
	int szMalloc;		/* Size of the zMalloc allocation */
	u32 uTemp;		/* Transient storage for serial_type in OP_MakeRecord */
	sql *db;		/* The associated database connection */
	void (*xDel) (void *);	/* Destructor for Mem.z - only valid if MEM_Dyn */
#ifdef SQL_DEBUG
	Mem *pScopyFrom;	/* This Mem is a shallow copy of pScopyFrom */
	void *pFiller;		/* So that sizeof(Mem) is a multiple of 8 */
#endif
};

bool
mem_is_null(const struct Mem *mem);

bool
mem_is_unsigned(const struct Mem *mem);

bool
mem_is_string(const struct Mem *mem);

bool
mem_is_number(const struct Mem *mem);

bool
mem_is_double(const struct Mem *mem);

bool
mem_is_integer(const struct Mem *mem);

bool
mem_is_boolean(const struct Mem *mem);

bool
mem_is_binary(const struct Mem *mem);

bool
mem_is_map(const struct Mem *mem);

bool
mem_is_array(const struct Mem *mem);

bool
mem_is_aggregate(const struct Mem *mem);

bool
mem_is_varstring(const struct Mem *mem);

bool
mem_is_frame(const struct Mem *mem);

bool
mem_is_undefined(const struct Mem *mem);

bool
mem_is_static(const struct Mem *mem);

bool
mem_is_ephemeral(const struct Mem *mem);

bool
mem_is_dynamic(const struct Mem *mem);

bool
mem_is_allocated(const struct Mem *mem);

bool
mem_is_cleared(const struct Mem *mem);

bool
mem_is_zeroblob(const struct Mem *mem);

bool
mem_is_same_type(const struct Mem *mem1, const struct Mem *mem2);

/**
 * Return a string that represent content of MEM. String is either allocated
 * using static_alloc() of just a static variable.
 */
const char *
mem_str(const struct Mem *mem);

/**
 * Initialize MEM and set NULL.
 */
void
mem_create(struct Mem *mem);

/**
 * Destroy MEM and set NULL.
 */
void
mem_destroy(struct Mem *mem);

void
mem_set_null(struct Mem *mem);

/**
 * Set integer value. According to is_neg flag value is considered
 * to be signed or unsigned.
 */
void
mem_set_integer(struct Mem *mem, int64_t value, bool is_neg);

void
mem_set_unsigned(struct Mem *mem, uint64_t value);

void
mem_set_boolean(struct Mem *mem, bool value);

void
mem_set_double(struct Mem *mem, double value);

void
mem_set_ephemeral_string(struct Mem *mem, char *value, uint32_t len);

void
mem_set_static_string(struct Mem *mem, char *value, uint32_t len);

void
mem_set_dynamic_string(struct Mem *mem, char *value, uint32_t len);

void
mem_set_allocated_string(struct Mem *mem, char *value, uint32_t len);

void
mem_set_ephemeral_string0(struct Mem *mem, char *value);

void
mem_set_static_string0(struct Mem *mem, char *value);

void
mem_set_dynamic_string0(struct Mem *mem, char *value);

void
mem_set_allocated_string0(struct Mem *mem, char *value);

/**
 * Copy content of MEM from one MEM to another. In case source MEM contains
 * string or binary and allocation type is not STATIC, this value is copied to
 * newly allocated by destination MEM memory.
 */
int
mem_copy_string(struct Mem *mem, const char *value, uint32_t len);

int
mem_copy_string0(struct Mem *mem, const char *value);

void
mem_set_ephemeral_binary(struct Mem *mem, char *value, uint32_t size);

void
mem_set_static_binary(struct Mem *mem, char *value, uint32_t size);

void
mem_set_dynamic_binary(struct Mem *mem, char *value, uint32_t size);

void
mem_set_allocated_binary(struct Mem *mem, char *value, uint32_t size);

void
mem_set_zerobinary(struct Mem *mem, int n);

int
mem_copy_binary(struct Mem *mem, const char *value, uint32_t size);

int
mem_append_to_binary(struct Mem *mem, const char *value, uint32_t size);

void
mem_set_ephemeral_map(struct Mem *mem, char *value, uint32_t size);

void
mem_set_static_map(struct Mem *mem, char *value, uint32_t size);

void
mem_set_dynamic_map(struct Mem *mem, char *value, uint32_t size);

void
mem_set_allocated_map(struct Mem *mem, char *value, uint32_t size);

void
mem_set_ephemeral_array(struct Mem *mem, char *value, uint32_t size);

void
mem_set_static_array(struct Mem *mem, char *value, uint32_t size);

void
mem_set_dynamic_array(struct Mem *mem, char *value, uint32_t size);

void
mem_set_allocated_array(struct Mem *mem, char *value, uint32_t size);

void
mem_set_undefined(struct Mem *mem);

void
mem_set_pointer(struct Mem *mem, void *ptr);

void
mem_set_frame(struct Mem *mem, struct VdbeFrame *frame);

int
mem_prepare_aggregate(struct Mem *mem, struct func *func, int size);

void *
mem_get_aggregate(struct Mem *mem);

void
mem_set_cleared(struct Mem *mem);

int
mem_copy(struct Mem *to, const struct Mem *from);

/**
 * Copy content of MEM from one MEM to another. In case source MEM contains
 * string or binary and allocation type is not STATIC, this value is copied as
 * value with ephemeral allocation type.
 */
int
mem_copy_as_ephemeral(struct Mem *to, const struct Mem *from);

/**
 * Move all content of source MEM to destination MEM. Source MEM is set to NULL.
 */
int
mem_move(struct Mem *to, struct Mem *from);

/**
 * Concatenate strings or binaries from the first and the second MEMs and write
 * the result to the third MEM.
 */
int
mem_concat(struct Mem *left, struct Mem *right, struct Mem *result);

int
mem_arithmetic(const struct Mem *left, const struct Mem *right,
	       struct Mem *result, int op);

int
mem_bitwise(struct Mem *left, struct Mem *right, struct Mem *result, int op);

int
mem_bit_not(struct Mem *mem, struct Mem *result);

int
mem_compare(const struct Mem *left, const struct Mem *right, int *result,
	    enum field_type type, struct coll *coll);

int
mem_convert_to_integer(struct Mem *mem);

int
mem_convert_to_integer_lossless(struct Mem *mem);

int
mem_convert_to_double(struct Mem *mem);

int
mem_convert_to_number(struct Mem *mem);

int
mem_convert_to_string(struct Mem *mem);

int
mem_convert_to_string0(struct Mem *mem);

int
mem_explicit_cast(struct Mem *mem, enum field_type type);

int
mem_implicit_cast(struct Mem *mem, enum field_type type);

int
mem_implicit_cast_old(struct Mem *mem, enum field_type type);

int
mem_get_integer(const struct Mem *mem, int64_t *i, bool *is_neg);

int
mem_get_unsigned(const struct Mem *mem, uint64_t *u);

int
mem_get_double(const struct Mem *mem, double *d);

int
mem_get_boolean(const struct Mem *mem, bool *b);

/**
 * Simple type to str convertor. It is used to simplify
 * error reporting.
 */
char *
mem_type_to_str(const struct Mem *p);

/*
 * Return the MP_type of the value of the MEM.
 * Analogue of sql_value_type() but operates directly on
 * transparent memory cell.
 */
enum mp_type
mem_mp_type(struct Mem *mem);

enum mp_type
sql_value_type(struct Mem *);

int sqlValueBytes(struct Mem *);

#ifdef SQL_DEBUG
void sqlVdbeMemAboutToChange(struct Vdbe *, struct Mem *);
int sqlVdbeCheckMemInvariants(struct Mem *);
void sqlVdbePrintSql(Vdbe *);
void sqlVdbeMemPrettyPrint(Mem * pMem, char *zBuf);
void
registerTrace(int iReg, Mem *p);

/*
 * Return true if a memory cell is not marked as invalid.  This macro
 * is for use inside assert() statements only.
 */
#define memIsValid(M) !mem_is_undefined(M)
#endif

/*
 * Invoke this macro on memory cells just prior to changing the
 * value of the cell.  This macro verifies that shallow copies are
 * not misused.  A shallow copy of a string or blob just copies a
 * pointer to the string or blob, not the content.  If the original
 * is changed while the copy is still in use, the string or blob might
 * be changed out from under the copy.  This macro verifies that nothing
 * like that ever happens.
 */
#ifdef SQL_DEBUG
# define memAboutToChange(P,M) sqlVdbeMemAboutToChange(P,M)
#else
# define memAboutToChange(P,M)
#endif

int sqlVdbeMemNulTerminate(struct Mem *);
int sqlVdbeMemExpandBlob(struct Mem *);
#define ExpandBlob(P) (mem_is_zeroblob(P)? sqlVdbeMemExpandBlob(P) : 0)

/** Setters = Change MEM value. */

int sqlVdbeMemGrow(struct Mem * pMem, int n, int preserve);
int sqlVdbeMemClearAndResize(struct Mem * pMem, int n);

void sqlValueFree(struct Mem *);
struct Mem *sqlValueNew(struct sql *);

/*
 * Release an array of N Mem elements
 */
void
releaseMemArray(Mem * p, int N);

/*
 * Clear any existing type flags from a Mem and replace them with f
 */
#define MemSetTypeFlag(p, f) \
   ((p)->flags = ((p)->flags&~(MEM_TypeMask|MEM_Zero))|f)

/** Getters. */

const void *
sql_value_blob(struct Mem *);

int
sql_value_bytes(struct Mem *);

const unsigned char *
sql_value_text(struct Mem *);

const void *sqlValueText(struct Mem *);

#define VdbeFrameMem(p) ((Mem *)&((u8 *)p)[ROUND8(sizeof(VdbeFrame))])

const Mem *
columnNullValue(void);

/** Checkers. */

int sqlVdbeMemTooBig(Mem *);

int sqlMemCompare(const Mem *, const Mem *, const struct coll *);

/** MEM manipulate functions. */

/**
 * Memory cell mem contains the context of an aggregate function.
 * This routine calls the finalize method for that function. The
 * result of the aggregate is stored back into mem.
 *
 * Returns -1 if the finalizer reports an error. 0 otherwise.
 */
int
sql_vdbemem_finalize(struct Mem *mem, struct func *func);

/** MEM and msgpack functions. */

/**
 * Perform comparison of two keys: one is packed and one is not.
 *
 * @param key1 Pointer to pointer to first key.
 * @param unpacked Pointer to unpacked tuple.
 * @param key2_idx index of key in umpacked record to compare.
 *
 * @retval +1 if key1 > pUnpacked[iKey2], -1 ptherwise.
 */
int sqlVdbeCompareMsgpack(const char **key1,
			      struct UnpackedRecord *unpacked, int key2_idx);

/**
 * Perform comparison of two tuples: unpacked (key1) and packed (key2)
 *
 * @param key1 Packed key.
 * @param unpacked Unpacked key.
 *
 * @retval +1 if key1 > unpacked, -1 otherwise.
 */
int sqlVdbeRecordCompareMsgpack(const void *key1,
				    struct UnpackedRecord *key2);

/**
 * Decode msgpack and save value into VDBE memory cell. String and binary string
 * values set as ephemeral.
 *
 * @param buf Buffer to deserialize msgpack from.
 * @param mem Memory cell to write value into.
 * @param len[out] Length of decoded part.
 * @retval Return code: < 0 in case of error.
 * @retval 0 on success.
 */
int
vdbe_decode_msgpack_into_ephemeral_mem(const char *buf, struct Mem *mem,
				       uint32_t *len);

/**
 * Decode msgpack and save value into VDBE memory cell. String and binary string
 * values copied to newly allocated memory.
 *
 * @param buf Buffer to deserialize msgpack from.
 * @param mem Memory cell to write value into.
 * @param len[out] Length of decoded part.
 * @retval Return code: < 0 in case of error.
 * @retval 0 on success.
 */
int
vdbe_decode_msgpack_into_mem(const char *buf, struct Mem *mem, uint32_t *len);

/**
 * Perform encoding memory variable to stream.
 * @param stream Initialized mpstream encoder object.
 * @param var Vdbe memory variable to encode with stream.
 */
void
mpstream_encode_vdbe_mem(struct mpstream *stream, struct Mem *var);

/**
 * Perform encoding field_count Vdbe memory fields on region as
 * msgpack array.
 * @param fields The first Vdbe memory field to encode.
 * @param field_count Count of fields to encode.
 * @param[out] tuple_size Size of encoded tuple.
 * @param region Region to use.
 * @retval NULL on error, diag message is set.
 * @retval Pointer to valid tuple on success.
 */
char *
sql_vdbe_mem_encode_tuple(struct Mem *fields, uint32_t field_count,
			  uint32_t *tuple_size, struct region *region);
