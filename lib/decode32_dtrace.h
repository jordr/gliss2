/* Generated by gep ($(date)) copyright (c) 2008 IRIT - UPS */

#ifndef GLISS_GLISS_INCLUDE_GLISS_DECODE_H	/* Caution: all GLISS_ occurrence will be renamed, the 1rst shouldn't */
#define GLISS_GLISS_INCLUDE_GLISS_DECODE_H


#if defined(__cplusplus)
extern  "C"
{
#endif


#define GLISS_DECODE_STATE
#define GLISS_DECODE_INIT(s)
#define GLISS_DECODE_DESTROY(s)

#define GLISS_DTRACE_CACHE
#ifndef TRACE_DEPTH
#define TRACE_DEPTH (32) // Must be a power of two 
#endif                   // and don't forget to update TRACE_DEPTH_PW
// TRACE_DEPTH is a power of two (2^N)
// TRACE_DEPTH_PW is N
#ifndef TRACE_DEPTH_PW
#define TRACE_DEPTH_PW (5)
#endif


#if defined(__cplusplus)
}
#endif

#endif /* GLISS_GLISS_INCLUDE_GLISS_DECODE_H */
