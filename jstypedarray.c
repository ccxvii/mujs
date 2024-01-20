#include "jsi.h"

/* https://www.khronos.org/registry/typedarray/specs/latest/ */

#define STR(S) #S
#define XSTR(S) STR(S)

struct ArrayBuffer {
	unsigned byteLength;
	char ptr[1];
};

static struct ArrayBuffer *new_ArrayBuffer_imp(js_State *J, const char *init, unsigned byteLength)
{
	struct ArrayBuffer *buffer = js_malloc(J, offsetof(struct ArrayBuffer, ptr) + byteLength);
	buffer->byteLength = byteLength;
	if (init)
		memcpy(buffer->ptr, init, byteLength);
	else
		memset(buffer->ptr, 0, byteLength);

	js_getglobal(J, "ArrayBuffer");
	js_getproperty(J, -1, "prototype");
	js_newuserdata(J, "ArrayBuffer", buffer, js_free);
	js_rot2pop1(J);

	jsB_propn(J, "byteLength", byteLength);
	return buffer;
}

static void new_ArrayBuffer(js_State *J)
{
	unsigned byteLength = js_touint32(J, 1);
	(void) new_ArrayBuffer_imp(J, NULL, byteLength);
}

static void ArrayBuffer_slice(js_State *J)
{
	struct ArrayBuffer *self = js_touserdata(J, 0, "ArrayBuffer");
	int begin = js_toint32(J, 1);
	int end = js_isdefined(J, 2) ? js_toint32(J, 2) : (int) self->byteLength;

	if (begin < 0) begin = self->byteLength - begin;
	if (end < 0) end = self->byteLength - end;
	if (begin < 0) begin = 0;
	if (end < 0) end = 0;
	if ((unsigned)begin > self->byteLength) begin = self->byteLength;
	if ((unsigned)end > self->byteLength) end = self->byteLength;
	if (end < begin) end = begin;

	(void) new_ArrayBuffer_imp(J, self->ptr + begin, end - begin);
}

struct DataView {
	unsigned byteLength;
	char *ptr;
};

static void new_DataView(js_State *J)
{
	struct ArrayBuffer *buffer = js_touserdata(J, 1, "ArrayBuffer");
	struct DataView *view;
	unsigned byteOffset = js_touint32(J, 2);
	unsigned byteLength = js_isdefined(J, 3) ? js_touint32(J, 3) : buffer->byteLength;

	if (byteOffset + byteLength > buffer->byteLength)
		js_rangeerror(J, "data view range ouf of bounds");

	view = js_malloc(J, sizeof *view);
	view->byteLength = byteLength;
	view->ptr = buffer->ptr + byteOffset;

	js_getglobal(J, "DataView");
	js_getproperty(J, -1, "prototype");
	js_newuserdata(J, "DataView", view, NULL);
	js_rot2pop1(J);

	js_copy(J, 1); /* copy buffer argument */
	js_defproperty(J, -2, "buffer", JS_READONLY | JS_DONTCONF);
	jsB_propn(J, "byteOffset", byteOffset);
	jsB_propn(J, "byteLength", byteLength);
}

struct TypedArray {
	int length;
	char *ptr;
};

static void new_TypedArray(js_State *J, const char *name, int size, js_HasProperty has, js_Put put, js_Finalize fin)
{
	struct ArrayBuffer *buffer = NULL;
	struct TypedArray *array;
	unsigned length, byteLength, byteOffset;
	unsigned k;
	int copy = 0;

	if (js_isuserdata(J, 1, "ArrayBuffer")) {
		buffer = js_touserdata(J, 1, "ArrayBuffer");
		byteOffset = js_touint32(J, 2);
		length = js_isdefined(J, 3) ? js_touint32(J, 3) : buffer->byteLength / size;
		byteLength = length * size;
		if (byteOffset + byteLength > buffer->byteLength)
			js_rangeerror(J, "typed array range ouf of bounds");
	} else if (js_isnumber(J, 1)) {
		length = js_touint32(J, 1);
		byteOffset = 0;
		byteLength = length * size;
	} else {
		/* copy array */
		length = js_getlength(J, 1);
		byteOffset = 0;
		byteLength = length * size;
		copy = 1;
	}

	array = js_malloc(J, sizeof *array);
	array->length = length;

	js_getglobal(J, name);
	js_getproperty(J, -1, "prototype");
	js_newuserdatax(J, name, array, has, put, NULL, fin);
	js_rot2pop1(J);

	jsB_propn(J, "byteOffset", byteOffset);
	jsB_propn(J, "byteLength", byteLength);
	jsB_propn(J, "length", length);

	if (buffer) {
		array->ptr = buffer->ptr + byteOffset;
		js_copy(J, 1); /* copy buffer argument */
		js_defproperty(J, -2, "buffer", JS_READONLY | JS_DONTCONF);
	} else {
		buffer = new_ArrayBuffer_imp(J, NULL, byteLength);
		array->ptr = buffer->ptr;
		js_defproperty(J, -2, "buffer", JS_READONLY | JS_DONTCONF);

		if (copy) {
			for (k = 0; k < length; ++k) {
				js_getindex(J, 1, k);
				js_setindex(J, -2, k);
			}
		}
	}
}

static int bigendian(void) { static const int one = 1; return *(char*)&one == 0; }
static void swap1(char *p) { }
static void swap2(char *p) { char a=p[0], b=p[1]; p[0]=b; p[1]=a; }
static void swap4(char *p) { char a=p[0], b=p[1], c=p[2], d=p[3]; p[0]=d; p[1]=c; p[2]=b; p[3]=a; }
static void swap8(char *p) {
	char a=p[0], b=p[1], c=p[2], d=p[3], e=p[4], f=p[5], g=p[6], h=p[7];
	p[0]=h; p[1]=g; p[2]=f; p[3]=e;
	p[4]=d; p[5]=c; p[6]=b; p[7]=a;
}

#define AX(N,T,swap) \
	static int N ## Array_has(js_State *J, void *v, const char *name) { \
		struct TypedArray *ta = v; \
		int idx; \
		if (js_isarrayindex(J, name, &idx)) { \
			if (idx < 0 || idx >= ta->length) \
				js_rangeerror(J, "index out of bounds"); \
			js_pushnumber(J, ((T*)ta->ptr)[idx]); \
			return 1; \
		} \
		return 0; \
	} \
	static int N ## Array_put(js_State *J, void *v, const char *name) { \
		struct TypedArray *ta = v; \
		int idx; \
		if (js_isarrayindex(J, name, &idx)) { \
			if (idx < 0 || idx >= ta->length) \
				js_rangeerror(J, "index out of bounds"); \
			((T*)ta->ptr)[idx] = js_tonumber(J, -1); \
			return 1; \
		} \
		return 0; \
	} \
	static void DataView_set ## N(js_State *J) { \
		struct DataView *view = js_touserdata(J, 0, "DataView"); \
		unsigned byteOffset = js_touint32(J, 1); \
		T value = js_tonumber(J, 2); \
		if (byteOffset + sizeof(T) > view->byteLength) \
			js_rangeerror(J, "offset out of bounds"); \
		if (js_toboolean(J, 3) ^ bigendian()) \
			swap((char*)&value); \
		*((T*)&view->ptr[byteOffset]) = value; \
	} \
	static void DataView_get ## N(js_State *J) { \
		struct DataView *view = js_touserdata(J, 0, "DataView"); \
		unsigned byteOffset = js_touint32(J, 1); \
		T value; \
		if (byteOffset + sizeof(T) > view->byteLength) \
			js_rangeerror(J, "offset out of bounds"); \
		value = *((T*)&view->ptr[byteOffset]); \
		if (js_toboolean(J, 3) ^ bigendian()) \
			swap((char*)&value); \
		js_pushnumber(J, value); \
	} \
	static void new_ ## N ## Array(js_State *J) { \
		new_TypedArray(J, XSTR(N ## Array), sizeof(T), N ## Array_has, N ## Array_put, js_free); \
	}

AX(Int8, signed char, swap1)
AX(Uint8, unsigned char, swap1)
AX(Int16, signed short, swap2)
AX(Uint16, unsigned short, swap2)
AX(Int32, signed int, swap4)
AX(Uint32, unsigned int, swap4)
AX(Float32, float, swap4)
AX(Float64, double, swap8)

void jsB_inittypedarray(js_State *J)
{
	js_newobject(J); /* ArrayBuffer prototype */
	{
		jsB_propf(J, "slice", ArrayBuffer_slice, 2);
	}
	js_newcconstructor(J, new_ArrayBuffer, new_ArrayBuffer, "ArrayBuffer", 1);
	js_defglobal(J, "ArrayBuffer", JS_DONTENUM);

	js_newobject(J); /* DataView prototype */
	{
		jsB_propf(J, "getInt8", DataView_getInt8, 1);
		jsB_propf(J, "getUint8", DataView_getUint8, 1);
		jsB_propf(J, "getInt16", DataView_getInt16, 1);
		jsB_propf(J, "getUint16", DataView_getUint16, 1);
		jsB_propf(J, "getInt32", DataView_getInt32, 1);
		jsB_propf(J, "getUint32", DataView_getUint32, 1);
		jsB_propf(J, "getFloat32", DataView_getFloat32, 1);
		jsB_propf(J, "getFloat64", DataView_getFloat64, 1);

		jsB_propf(J, "setInt8", DataView_setInt8, 3);
		jsB_propf(J, "setUint8", DataView_setUint8, 3);
		jsB_propf(J, "setInt16", DataView_setInt16, 3);
		jsB_propf(J, "setUint16", DataView_setUint16, 3);
		jsB_propf(J, "setInt32", DataView_setInt32, 3);
		jsB_propf(J, "setUint32", DataView_setUint32, 3);
		jsB_propf(J, "setFloat32", DataView_setFloat32, 3);
		jsB_propf(J, "setFloat64", DataView_setFloat64, 3);
	}
	js_newcconstructor(J, new_DataView, new_DataView, "DataView", 1);
	js_defglobal(J, "DataView", JS_DONTENUM);

#define BX(N,T) \
	js_newobject(J); \
	jsB_propn(J, "BYTES_PER_ELEMENT", sizeof(T)); \
	js_newcconstructor(J, new_ ## N ## Array, new_ ## N ## Array, STR(N), 1); \
	js_defglobal(J, STR(N ## Array), JS_DONTENUM);

	BX(Int8, signed char)
	BX(Uint8, unsigned char)
	BX(Int16, signed short)
	BX(Uint16, unsigned short)
	BX(Int32, signed int)
	BX(Uint32, unsigned int)
	BX(Float32, float)
	BX(Float64, double)
}

void js_newarraybuffer(js_State *J, const void *p, int n)
{
	(void) new_ArrayBuffer_imp(J, p, n);
}

void *js_toarraybuffer(js_State *J, int idx, int *n)
{
	struct ArrayBuffer *buffer = js_touserdata(J, idx, "ArrayBuffer");
	return *n = buffer->byteLength, buffer->ptr;
}

void js_newdataview(js_State *J, const void *p, int n)
{
	js_getglobal(J, "DataView");
	js_pushnull(J);
	js_newarraybuffer(J, p, n);
	js_pushnumber(J, 0);
	js_pushnumber(J, n);
	js_call(J, 3);
}

void *js_todataview(js_State *J, int idx, int *n)
{
	struct DataView *view = js_touserdata(J, idx, "DataView");
	return *n = view->byteLength, view->ptr;
}

void js_newint8array(js_State *J, const signed char *p, int n)
{
	js_getglobal(J, "Int8Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n);
	js_call(J, 1);
}

void js_newint16array(js_State *J, const signed short *p, int n)
{
	js_getglobal(J, "Int16Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 2);
	js_call(J, 1);
}

void js_newint32array(js_State *J, const signed int *p, int n)
{
	js_getglobal(J, "Int32Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 4);
	js_call(J, 1);
}

void js_newuint8array(js_State *J, const unsigned char *p, int n)
{
	js_getglobal(J, "UInt8Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n);
	js_call(J, 1);
}

void js_newuint16array(js_State *J, const unsigned short *p, int n)
{
	js_getglobal(J, "UInt16Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 2);
	js_call(J, 1);
}

void js_newuint32array(js_State *J, const unsigned int *p, int n)
{
	js_getglobal(J, "UInt32Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 4);
	js_call(J, 1);
}

void js_newfloat32array(js_State *J, const float *p, int n)
{
	js_getglobal(J, "Float32Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 4);
	js_call(J, 1);
}

void js_newfloat64array(js_State *J, const double *p, int n)
{
	js_getglobal(J, "Float64Array");
	js_pushnull(J);
	js_newarraybuffer(J, p, n * 8);
	js_call(J, 1);
}

signed char *js_toint8array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "Int8Array");
	return *n = array->length, (signed char *)array->ptr;
}

signed short *js_toint16array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "Int16Array");
	return *n = array->length, (signed short *)array->ptr;
}

signed int *js_toint32array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "Int32Array");
	return *n = array->length, (signed int *)array->ptr;
}

unsigned char *js_touint8array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "UInt8Array");
	return *n = array->length, (unsigned char *)array->ptr;
}

unsigned short *js_touint16array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "UInt16Array");
	return *n = array->length, (unsigned short *)array->ptr;
}

unsigned int *js_touint32array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "UInt32Array");
	return *n = array->length, (unsigned int *)array->ptr;
}

float *js_tofloat32array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "Float32Array");
	return *n = array->length, (float *)array->ptr;
}

double *js_tofloat64array(js_State *J, int idx, int *n)
{
	struct TypedArray *array = js_touserdata(J, idx, "Float64Array");
	return *n = array->length, (double *)array->ptr;
}
