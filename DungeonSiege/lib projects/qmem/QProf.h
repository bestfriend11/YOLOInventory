/*
 * qprof.h
 *
 * Copyright (c) 1995, 2001 Rational Software Corporation.  All rights reserved.
 *
 * Rational Software Corporation Proprietary and Confidential Information
 */

/*
 * qprof.h: defines interfaces exposed by QProf
 */

#ifndef __QPROF_H_
#define __QPROF_H_

/*

QProf is a library that exports an API that lets you connect Java
virtual machines to Rational Quantify and Coverage. 

Rational has three profiling solutions for Java and other non-native
environments. One is called "quantify_interpreter," another is called
"QJMon/CJMon," and the third is this one, QProf.

All of these solutions present some sort of API so you can use Quantify
and/or Coverage in a non-OCI environment. The functions you can call in
the API are used to report certain events, like function entry, function
exit, thread create, and other events.

But the various Rational libraries that let you connect to Quantify and
Coverage have different intended uses, different features, and different
strengths.

Quantify_interpreter was written first. It is minimalist in nature (only
281 lines of source!), exposing C-callable hooks into the Quantify and
Coverage runtime libraries. Using it can be complex because you have to
manage your own QuantifyIdInfo objects to describe functions, you have
to make calls like add_file to inform the runtime about loaded DLLs, and
you have to manually manage other details.

QJMon followed. The QJMon library is a COM component which implements
the Microsoft-specific JEvMon interface, which is used by Microsoft Java
Virtual Machines and also by Microsoft Visual Basic. This monitoring
interface delivers notifications when certain events occur, like
function entry and exit, class loading, garbage collection, etc. These
notifications are turned into appropriate calls directly into the
PureQRT library, which is the Quantify runtime. (QJMon could have been
implemented on top of quantify_interpreter, but this reduces performance
without adding value.)

One characteristic of the MS JEvMon design is that it includes
per-byte-code notification: you get a notification for every byte code
that's executed. This lets Quantify use static per-byte-code timing
information, where we predict how long each byte code requires to
execute, to report repeatable performance times; and it lets Coverage
report detailed line-by-line coverage information.

QProf strikes a middle ground between these two. It's not a COM object,
like QJMon is. That means it can be used in environments which are not
based on COM. And its interface is more generic than QJMon's: it's not
tailored to the Microsoft performance monitor API. But on the other hand
it exports higher-level functionality than quantify_interpreter: it
doesn't expose any Quantify or Coverage specific data types or runtime
details. You can pass data and events into the library and forget it.

So that's the story of the three solutions for connecting programs to
Quantify and Coverage without using OCI. 

QPROF LIBRARY OVERVIEW

The basic theory of operation of the QProf library is that you make
calls to report function entry, basic-block entry, and function exit.
Everything else is gravy. Even the basic blocks are optional, if you
only want function-level Quantify and Coverage information.

The most important type exported by the library is qprof_MethodID. Every
function in the program being monitored must have a unique
qprof_MethodID value. It's up to you to assign a unique qprof_MethodID
for each function in the program. If the environment you're profiling
(the JVM or whatever) gives you a unique four-byte method ID, use that.
When you enter a function, you tell what function is being entered by
passing your chosen qprof_MethodID.

The first time the QProf library sees a new qprof_MethodID, it's going
to call back to you and ask for more information about that function.
When this happens, you have to tell QProf the method's name, what class
it's in, and what source file it came from. You also pass in a
line-number table if you have one, and if you care about fine-grain
performance and coverage information.

You don't have to wait for QProf to ask you about a function. You can
"register" any function you want to, at any time. In fact, to get good
Coverage information you *must* register function information ahead of
time, rather than waiting for the callback. You'll only get the callback
(where QProf asks you about a method) for those methods which are
actually *used* in your program; that is, those methods you report
function entry for. That means your coverage data won't include any
functions that are never used -- there won't be any "missed" functions
in the whole Coverage report!

A common way to solve this is to register all the functions in a class
when the class is loaded. You call qprof_register_method_id for each
method in the class, passing its unique qprof_MethodID and the name,
class, source file, and line number information. 

More notes about the callback: if you pre-register *every*
qprof_MethodID before you use it, then the callback will never be used,
and you don't need to implement it. Also, the callback will only be used
once (or zero times if you pre-register) per unique qprof_MethodID. You
do not need to pre-register the pre-defined method IDs from qprof.h:
qprof_Start, qprof_ClassLoader, qprof_GarbageCollector, etc.

Never use a qprof_MethodID value of zero. It's a special "flag" value
and should not be used to represent any actual function.

Another important data type is qprof_StackID. Once again, this is an ID
value that you must determine and use consistently. When you report
function entry, you pass the qprof_MethodID of the function being
entered, and also a qprof_StackID value to represent the location of the
stack pointer at this time. You have to remember the qprof_StackID you
use so you can report function exits properly.

Unfortunately, the StackID usage is a little complicated. It's not as
simple as passing a number at function entry time, and passing that same
number at function exit time. What really happens is that you pass a
number (call it X) to identify the frame you're in at function entry
time, and later when child functions exit you pass that same number,
saying, "We're back in frame X."

An example should make this clear:

	main() {
		fun_one();
		fun_two();
	}

	fun_one() {
		fun_three();
	}
	fun_two() {
		// empty
	}

On entry to main, let's say you choose qprof_StackID 65 to represent
that stack frame. (Never mind how you picked that number.) The sequence
of reports and stack ID's for this program would be:

	fn_entry(main, 65)
	fn_entry(fun_one, 66)
	fn_entry(fun_three, 67)
	fn_exit(66)
	fn_exit(65)
	fn_entry(fun_two, 66)
	fn_exit(65)
	fn_exit(xx)

The last fn_exit is "xx" because this example doesn't show what the
right value would be.

As you can see, the stack frame for function "main" is identified with
the qprof_StackID value 65. Subsequently, the qprof_StackID value 65 is
consistently used to mean, "We're back in main." The same goes for the
qprof_StackID value 66 identifying the frame for fun_one.

The qprof_StackID values have to be maintained on a per-thread basis,
because the stack itself is a per-thread object. StackID values don't
have to be unique across threads.

Note that some implementations can get away with using actual machine SP
values as StackID values. But this is not the case if the execution
stack of native C code is different from the "virtual" stack maintained
in the environment you're trying to profile. Also note that the stack ID
values can count up or down - it doesn't matter.

In practice, you can read the fn_exit notification as, "Pop frames off
the shadow stack until you get to one whose ID matches this value."
Exceptions, which can cause execution to continue in a frame that is
many levels up the stack from the last fn_entry, can be handled in one
of two ways. One way is for the environment you're profiling gives you a
"fn_exit" notification for every frame as it's unwound, and you pass
those notifications along to QProf. The other is for the environment to
give you a single "exception notification" including a stack ID value
for the frame where execution will continue. You can call fn_exit once
using that value. This will unroll the shadow stack all the way up to
the corresponding level.

Do not use a StackID of zero to identify any real stack frame: it has
special internal meaning to QProf. Also, if you mistakenly pass a
StackID to fn_exit which does not correspond to any previous fn_entry,
the shadow stack will be unrolled all the way. (This is not a feature,
just a side-effect of the algorithm, and after it happens the library
might crash. So don't do it on purpose.)

Regarding threads: there is a type, qprof_ThreadID, but the thread model
isn't completely abstracted. The whole QProf system assumes that a
"thread" in the environment you're profiling is exactly the same thing
as an operating-system thread. It also assumes that all notifications
are delivered on the thread where the corresponding event (fn entry, fn
exit, etc.) has happened. Also, qprof_set_thread_name implicitly applies
to the current, executing thread.

The type qprof_Counts is a 32-bit value representing a number of machine
cycles of elapsed time. It's used in qprof_record_counts.

Now for some QProf usage specifics: how do you put together a monitor
package that uses this library?

Then you call qprof_initialize. This function loads the appropriate
Quantify or Coverage runtime and initializes it. It also allocates
QProf's internal data structures and initializes its state.

The arguments to the initialize() call are:

	1. Options for the Quantify or Coverage runtime and server to
	   see. This is a place where Quantify/Coverage details are
	   exposed: the option string you pass is handed directly to the
	   runtime's initialization function. The main thing to pass
	   here is an indication of what source path to use, using
	   -default-source-path.

	2. A function pointer. This must be a pointer to a cdecl
	   function which takes a single qprof_MethodID argument and
	   returns no value (void). This function will be called by
	   QProf as the callback discussed above. In effect, it's called
	   when QProf would like you to call qprof_register_method_id on
	   a qprof_MethodID that QProf hasn't seen yet. As discussed
	   before, if you ensure that you call qprof_register_method_id
	   on every method ID you use (except the predefined ones), you
	   don't need to implement a callback and you can pass NULL as
	   this argument.

The return value from qprof_initialize can be a negative number, which
indicates failure; or a positive number, which indicates success. The
different negative (failure) values are useful in debugging only.

A success value of one (1) from qprof_initialize means the user wants
fine-grain data collection via execute_byte_code; a success value of two
(2) means the user doesn't want that. The value 2 generally means that
the user has requested function-level monitoring, usually by setting
Quantify or Coverage options. In this case you can save time by not
requesting fine-grain events from your host system and/or by not
reporting them to qprof. The determining factor is the -input-file-name
option provided in the initial_options string: that name is compared
with the user preferences to see how to respond. In any case, a QProf
client is *allowed* to report fine-grain events, even if the result code
from qprof_initialize is 2.

When the program that you are profiling has exited, you should call
qprof_shutdown. You can also simply exit the process, and the Quantify
or Coverage slave will notice eventually and report the results.

The function qprof_register_method_id is the way that the QProf system
is able to associate MethodIDs with their method names, class names,
source files, and source-line information.

The "flags" argument for qprof_register_method_id is a bit mask of
attributes for the method that you're registering. These flags' names
all start with QPROF_METHOD_X flags.

The line_number_table argument must point to an array of qprof_LineInfo
structs, or something with identical layout. Each entry gives the
starting code offset value of the code produced by a given line number.
Code at offsets from there until the next one are assumed to have come
from the indicated line.

Example:
	 0 	15
	 7	16
	12	19

This table means that code offsets from 0-6 associate with line 15, and
7-11 associate with line 16, and anything 12 and above associates with
line 19. The table should be sorted in ascending order of code offsets.
The line numbers don't have to be unique or in increasing order: your
optimizer can do anything it wants with them. If the table doesn't start
with code-offset zero, then offsets from zero to the first value are
also associated with the first line number.

The line_number_size argument to qprof_register_method_id tells how many
entries there are in the array.

If you are going to use qprof_execute_byte_code, the offsets in this
table are significant: they're the way the byte code offset gets turned
into a bb_index, a "basic block" number.

If you are not using qprof_execute_byte_code (that is, if you are using
record_bb_counts and/or record_bb_time), the table must have one entry
per BB in the function. The table size is taken to be the number of bb's
in the function, and subsequent bb_index arguments are used as an index
into this table. In this case, the offsets in the table are not
significant.

This source-info format doesn't support include files: the whole method
is presumed to have come from a single file. Sorry, those are the
breaks.

Memory ownership: The qprof system does not keep pointers to any of the
memory passed to it. Thus the caller is always responsible for memory
ownership and cleanup. In particular, the various string buffers and
line number table may be freed as soon as qprof_register_method_id
returns.

Do not recycle qprof_methodID values. Once you've used a qprof_methodID
to identify a function, you can't ever use that methodID to identify
some other function. You might want to: the host system you're
monitoring might have nice, handy methodID's that you want to use, but
be careful. The host might recycle methodID's if (for example) a class
is loaded, then unloaded, then another class is loaded. Watch out for
ID's whose lifetime is shorter than the whole run of the program.

You call qprof_record_fn_entry() to report entry to a function. You pass
the qprof_MethodID of the function being entered, and you pass the
qprof_StackID to identify the new stack frame. Later, when this function
exits, you will *not* pass this same qprof_StackID; rather, you will
pass the stack id of the calling function, to say, "This is the frame
that we are not executing in."

If the method ID has not been registered with QProf sometime in the past
using qprof_register_method_id(), then the callback function will be
called, and given the qprof_MethodID. This is a request that the
function be registered.

You call qprof_record_fn_exit to report exit from a function. As
explained elsewhere, you pass the qprof_StackID value that you used on
entry to the *calling* function. What you are doing is reporting the
qprof_StackID of the stack frame which is being returned to -- the one
that will resume executing. Internally, QProf will unroll its shadow
stack until the identified stack frame is at the top.

There are some special "pseudo-methods" whose time should be accounted
for. At this writing, these are the Class Loader, the Garbage Collector,
and the JIT Compiler. They have predefined method IDs, defined in
qprof.h, and when they start, you should call qprof_record_fn_entry.

However, in some environments there is not a meaningful qprof_StackID to
use when these methods are entered. Thus there is no value you can
usefully pass to qprof_record_fn_exit. For these cases, there is another
API function, qprof_verify_fn_exit. This takes a qprof_MethodID instead
of a stack ID, and its main operation is to pop a single frame off the
shadow stack. However, that frame might only be popped if its method ID
matches the argument; this test is currently a Quantify/Coverage
variation based on the original QJMon implementation.

If you are content with real-time monitoring of performance, you do not
need any other API calls for function-level Quantify or Coverage.

You would call qprof_record_counts if you want to take control of the
time counting for a particular method. You pass a number of machine
cycles, and the function at the top of the shadow stack gets that number
of cycles added to its time. There is an important side effect: the
first time you call this for a given function, that function's timing
mode is changed from "real time" to "cycle counting." As of that moment,
the function will not get automatic real-time accounting; you have to
call qprof_record_counts in order for that function to get any time
allocated to it at all, any time it's called in the future. Any real
time that has passed since the function was entered will be discarded as
well.

For fine-grain counting, call qprof_execute_byte_code and pass a method
ID, a byte-code program counter, and a pc_offset value which indicates
the offset from the start of the method to the corresponding PC.

The pc_offset value is used to determine what line you're on; the PC
itself is used to read the byte-code being executed, and hence the
number of clock cycles needed to execute it. In the case of Quantify,
the calculated cycle count is added to the lines totals. For Coverage,
the corresponding source line is marked as having been "hit" one more
time.

(Note: this notion of statically calculating the "cost" of each byte
code is obsolete and should not persist. It's a TODO to get rid of it
and use real-time-based counting per source line.)

Use the function qprof_thread_create to announce that a new thread is
being created. You choose a qprof_ThreadID for it.

The function qprof_set_thread_name() takes a character string and sets
the current host-native thread's name for display purposes.

(This is a poor interface: it should take a thread ID and change that
thread's name. The deepest PureQRT function being called doesn't have
that ability. So you can't use one thread to change another thread's
name. Is this a TODO item?)

The function qprof_thread_destroy() takes a qprof_ThreadID argument and
reports the destruction of a thread. The PureQRT runtime might have
figured it out anyway, because of the native-thread-based system.

qprof_record_monitor_acquired adds a constant number of "ticks" based on
the presumed cost of acquiring a monitor. (TODO: that cost is currently
zero, not changeable by clients.)

qprof_record_monitor_acquired adds a constant number of "ticks" based on
the presumed cost of releasing a monitor. (TODO: that cost is currently
zero, not changeable by clients.)

You can get access to the Quantify / Coverage option system using
qprof_get_option. It returns a "void *" which you should cast to a
"const char *" for string options, or a bool, int, or unsigned for
numeric options. You have to know the type of the option you're asking
for and cast appropriately. If you ask for a nonexistent option, you get
NULL, which you can't tell from a zero value for a valid option. (You
also can't tell whether the option was allowed to default or was set
manually.)

qprof_current_method_id returns the qprof_MethodID of the method on the
top of the Quantify shadow stack. This can be useful during debugging or
in some unusual situations where you want to check shadow stack
synchronization.

*/

typedef unsigned long qprof_MethodID;
typedef unsigned long qprof_ClassID;
typedef unsigned long qprof_StackID;
typedef unsigned long qprof_ThreadID;
typedef unsigned int qprof_Counts;

#if !defined(MEMPROFILE_TYPEDEFS)
typedef unsigned long jObjectID;
typedef unsigned long jClassID;
typedef unsigned long jArenaID;
typedef unsigned long jArrayID;
typedef unsigned long jSize;
#endif

// Constants for the first arg to qprof_initialize
#define QPROF_QUANTIFY 1
#define QPROF_COVERAGE 2
#define QPROF_PURIFY   3

// Braeak on event flags
#define BRK_ON_FENTRY			1 // Break when Function ID = event_data_ptr entered
#define	BRK_ON_FEXIT			2 // Break when Function ID = event_data_ptr exited
#define BRK_ON_BBENTRY_IN_FID	4 // Break if BB entered in function ID = event_data_ptr

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _EXT_REF
#define _EXT_REF __declspec(dllimport)
#define _HAD_TO_DEFINE_EXT_REF_
#endif

/*
 * The line number table is an array of structs, mapping each code
 * offset to a line number.
 *
 * The table is sorted in ascending order of code offsets. When you
 * register a method, you pass in the start address of an array of
 * these, and an arg telling how many there are.
 *
 * If you are going to use qprof_execute_byte_code, the offsets in this
 * table are significant: they're the way the byte code offset
 * gets turned into a basic-block number.
 *
 * If you are not using qprof_execute_byte_code (that is, if you are
 * using record_bb_counts and/or record_bb_time), the table must have
 * one entry per BB in the function. The table size is taken to be the
 * number of bb's in the function, and subsequent bb_index arguments are
 * used as an index into this table.
 *
 * This format doesn't support include files: the whole method is
 * presumed to have come from a single file. 
 *
 * (Conveniently, this struct exactly matches what the MS JEvMon info
 * interface produces.)
 */

typedef struct qprof_LineInfo {
	int code_offset;
	int line_number;
} qprof_LineInfo;

int _EXT_REF qprof_initialize(const char *initial_options,
				void (*callback_get_method_info)(qprof_MethodID));
void _EXT_REF qprof_shutdown(void);
void _EXT_REF qprof_record_fn_entry(qprof_MethodID method_id, qprof_StackID stack_id);
void _EXT_REF qprof_record_fn_exit(qprof_StackID id);
void _EXT_REF qprof_verify_fn_exit(qprof_MethodID id);
void _EXT_REF qprof_thread_create(qprof_ThreadID id);
void _EXT_REF qprof_set_thread_name(const char *new_name);
void _EXT_REF qprof_thread_destroy(qprof_ThreadID id);

void _EXT_REF qprof_record_counts(qprof_Counts arg);

void _EXT_REF qprof_record_bb_counts(int bb_num, qprof_Counts arg);
void _EXT_REF qprof_record_bb_time(int bb_num);
void _EXT_REF qprof_execute_byte_code(qprof_MethodID method_id,const char *pc,int pc_offset);
void _EXT_REF qprof_record_monitor_acquired(void);
void _EXT_REF qprof_record_monitor_released(void);

void _EXT_REF qprof_register_method_id(qprof_MethodID id,
			const char *demangled_name,
			const char *class_name,
			int flags,
			const char *source_file,
			const qprof_LineInfo *line_number_table,
			int line_number_size);


_EXT_REF void * qprof_get_option(const char *optionName);
_EXT_REF qprof_MethodID qprof_current_method_id(void);

/*
 * This enum is for the "flags" argument to qprof_register_method_id.
 * You should OR together the flags that apply to the method you're
 * registering.
 *
 * So far the only flags are "this method blocks" and "this method
 * waits." 
 *
 * Examples of methods that block:
 *	java.lang.Object.wait
 *	java.lang.Thread.sleep(long)
 *	com.ms.awt.peer.CToolkit.callbackEventLoop
 *
 * Examples of methods that wait:
 *	Stream.read / write
 *	Stream.socketRead / socketWrite
 * 
 */
enum QPROF_METHOD_FLAGS {
	QPROF_METHOD_FLAGS_NONE=0,
	QPROF_METHOD_FLAGS_BLOCKS=1,
	QPROF_METHOD_FLAGS_WAITS=2
};

/* Quantify and Coverage API functions */

int _EXT_REF qprof_is_running(void);
int _EXT_REF qprof_disable_recording_data(void);
int _EXT_REF qprof_start_recording_data(void);
int _EXT_REF qprof_stop_recording_data(void);
int _EXT_REF qprof_clear_data(void);
int _EXT_REF qprof_is_recording_data(void);
int _EXT_REF qprof_save_data(void);
int _EXT_REF qprof_new_phase(void);     // quantify only

int _EXT_REF qprof_add_annotation(const char * comment);


void _EXT_REF qprof_set_break_on_event (unsigned long flags, void* event_data_ptr);

#ifdef _HAD_TO_DEFINE_EXT_REF_
#undef _EXT_REF
#undef _HAD_TO_DEFINE_EXT_REF_
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Copied from qjmon.cpp: special IDs for special functions */
#define qprof_Start			(qprof_MethodID)(0xfffffffe)
#define qprof_ClassLoader		(qprof_MethodID)(qprof_Start - 1) /* (QuantifyID)0xfffffffd */
#define qprof_GarbageCollector		(qprof_MethodID)(qprof_Start - 2) /* (QuantifyID)0xfffffffc */
#define qprof_JITCompiler		(qprof_MethodID)(qprof_Start - 3) /* (QuantifyID)0xfffffffb */

#endif // end of ifndef __QPROF_H_
