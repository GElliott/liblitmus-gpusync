#ifndef LITMUS_H
#define LITMUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdint.h>
#include <setjmp.h>

/* Include kernel header.
 * This is required for the rt_param
 * and control_page structures.
 */
#include "litmus/rt_param.h"
#include "litmus/signal.h"

#include "asm/cycles.h" /* for null_call() */

#include "migration.h"

void init_rt_task_param(struct rt_task* param);
int set_rt_task_param(pid_t pid, struct rt_task* param);
int get_rt_task_param(pid_t pid, struct rt_task* param);

/* Release-master-aware functions for getting the first
 * CPU in a particular cluster or partition. Use these
 * to set rt_task::cpu for cluster/partitioned scheduling.
 */
int partition_to_cpu(int partition);
int cluster_to_first_cpu(int cluster, int cluster_size);

/* Convenience functions for setting up real-time tasks.
 * Default behaviors set by init_rt_task_params() used.
 * Also sets affinity masks for clustered/partitions
 * functions. Time units in nanoseconds. */
int sporadic_global(lt_t e_ns, lt_t p_ns);
int sporadic_partitioned(lt_t e_ns, lt_t p_ns, int partition);
int sporadic_clustered(lt_t e_ns, lt_t p_ns, int cluster, int cluster_size);

/* simple time unit conversion macros */
#define s2ns(s)   ((s)*1000000000LL)
#define s2us(s)   ((s)*1000000LL)
#define s2ms(s)   ((s)*1000LL)
#define ms2ns(ms) ((ms)*1000000LL)
#define ms2us(ms) ((ms)*1000LL)
#define us2ns(us) ((us)*1000LL)

/* file descriptor attached shared objects support */
typedef enum  {
	FMLP_SEM	= 0,
	SRP_SEM		= 1,
	MPCP_SEM	= 2,
	MPCP_VS_SEM	= 3,
	DPCP_SEM	= 4,
	PCP_SEM		= 5,

	FIFO_MUTEX	= 6,
	IKGLP_SEM	= 7,
	KFMLP_SEM	= 8,

	IKGLP_SIMPLE_GPU_AFF_OBS = 9,
	IKGLP_GPU_AFF_OBS = 10,
	KFMLP_SIMPLE_GPU_AFF_OBS = 11,
	KFMLP_GPU_AFF_OBS = 12,

	PRIOQ_MUTEX = 13,
} obj_type_t;

int lock_protocol_for_name(const char* name);
const char* name_for_lock_protocol(int id);

int od_openx(int fd, obj_type_t type, int obj_id, void* config);
int od_close(int od);

static inline int od_open(int fd, obj_type_t type, int obj_id)
{
	return od_openx(fd, type, obj_id, 0);
}

int litmus_open_lock(
	obj_type_t protocol,	/* which locking protocol to use, e.g., FMLP_SEM */
	int lock_id,		/* numerical id of the lock, user-specified */
	const char* ns,	/* path to a shared file */
	void *config_param);	/* any extra info needed by the protocol (such
				 * as CPU under SRP and PCP), may be NULL */

/* real-time locking protocol support */
int litmus_lock(int od);
int litmus_unlock(int od);
int litmus_should_yield_lock(int od);

/* Dynamic group lock support.  ods arrays MUST BE PARTIALLY ORDERED!!!!!!
 * Use the same ordering for lock and unlock.
 *
 * Ex:
 *   litmus_dgl_lock({A, B, C, D}, 4);
 *   litmus_dgl_unlock({A, B, C, D}, 4);
 */
int litmus_dgl_lock(int* ods, int dgl_size);
int litmus_dgl_unlock(int* ods, int dgl_size);
int litmus_dgl_should_yield_lock(int* ods, int dgl_size);

/* nvidia graphics cards */
int register_nv_device(int nv_device_id);
int unregister_nv_device(int nv_device_id);

/* job control*/
int get_job_no(unsigned int* job_no);
int wait_for_job_release(unsigned int job_no);
int sleep_next_period(void);

/*  library functions */
int  init_litmus(void);
int  init_rt_thread(void);
void exit_litmus(void);

/* A real-time program. */
typedef int (*rt_fn_t)(void*);

/* exec another program as a real-time task. */
int create_rt_task(rt_fn_t rt_prog, void *arg, struct rt_task* param);

/*	per-task modes */
enum rt_task_mode_t {
	BACKGROUND_TASK = 0,
	LITMUS_RT_TASK  = 1
};
int task_mode(int target_mode);

void show_rt_param(struct rt_task* tp);
task_class_t str2class(const char* str);

/* non-preemptive section support */
void enter_np(void);
void exit_np(void);
int  requested_to_preempt(void);

/* task system support */
int wait_for_ts_release();
int wait_for_ts_release2(struct timespec *release);
int release_ts(lt_t *delay);
int get_nr_ts_release_waiters(void);
int read_litmus_stats(int *ready, int *total);

int enable_aux_rt_tasks(int flags);
int disable_aux_rt_tasks(int flags);

/* sleep for some number of nanoseconds */
int lt_sleep(lt_t timeout);

/* CPU time consumed so far in seconds */
double cputime(void);

/* wall-clock time in seconds */
double wctime(void);

/* semaphore allocation */

typedef int (*open_sem_t)(int fd, int name);

static inline int open_fmlp_sem(int fd, int name)
{
	return od_open(fd, FMLP_SEM, name);
}

static inline int open_kfmlp_sem(int fd, int name, unsigned int nr_replicas)
{
	if (!nr_replicas)
		return -1;
	return od_openx(fd, KFMLP_SEM, name, &nr_replicas);
}

static inline int open_srp_sem(int fd, int name)
{
	return od_open(fd, SRP_SEM, name);
}

static inline int open_pcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, PCP_SEM, name, &cpu);
}

static inline int open_mpcp_sem(int fd, int name)
{
	return od_open(fd, MPCP_SEM, name);
}

static inline int open_dpcp_sem(int fd, int name, int cpu)
{
	return od_openx(fd, DPCP_SEM, name, &cpu);
}

static inline int open_fifo_sem(int fd, int name)
{
	return od_open(fd, FIFO_MUTEX, name);
}

static inline int open_prioq_sem(int fd, int name)
{
	return od_open(fd, PRIOQ_MUTEX, name);
}

int open_ikglp_sem(int fd, int name, unsigned int nr_replicas);

/* KFMLP-based Token Lock for GPUs
 * Legacy; mostly untested.
 */
int open_kfmlp_gpu_sem(int fd, int name,
	unsigned int num_gpus, unsigned int gpu_offset, unsigned int rho,
	int affinity_aware /* bool */);

/* -- Example Configurations --
 *
 * Optimal IKGLP Configuration:
 *   max_in_fifos = IKGLP_M_IN_FIFOS
 *   max_fifo_len = IKGLP_OPTIMAL_FIFO_LEN
 *
 * IKGLP with Relaxed FIFO Length Constraints:
 *   max_in_fifos = IKGLP_M_IN_FIFOS
 *   max_fifo_len = IKGLP_UNLIMITED_FIFO_LEN
 * NOTE: max_in_fifos still limits total number of requests in FIFOs.
 *
 * KFMLP Configuration (FIFO queues only):
 *   max_in_fifos = IKGLP_UNLIMITED_IN_FIFOS
 *   max_fifo_len = IKGLP_UNLIMITED_FIFO_LEN
 * NOTE: Uses a non-optimal IKGLP configuration, not an actual KFMLP_SEM.
 *
 * RGEM-like Configuration (priority queues only):
 *   max_in_fifos = 1..(rho*num_gpus)
 *   max_fifo_len = 1
 *
 * For exclusive GPU allocation, use rho = 1
 * For trivial token lock, use rho = # of tasks in task set
 *
 * A simple load-balancing heuristic will still be used if
 * enable_affinity_heuristics = 0.
 *
 * Other constraints:
 *  - max_in_fifos <= max_fifo_len * rho
 *        (unless max_in_fifos = IKGLP_UNLIMITED_IN_FIFOS and
 *         max_fifo_len = IKGLP_UNLIMITED_FIFO_LEN
 *  - rho > 0
 *  - num_gpus > 0
 */
// takes names 'name' and 'name+1'
int open_gpusync_token_lock(int fd, int name,
		unsigned int num_gpus, unsigned int gpu_offset,
		unsigned int rho, unsigned int max_in_fifos,
		unsigned int max_fifo_len,
		int enable_affinity_heuristics /* bool */);

/* syscall overhead measuring */
int null_call(cycles_t *timestamp);

/*
 * get control page:
 * atm it is used only by preemption migration overhead code
 * but it is very general and can be used for different purposes
 */
struct control_page* get_ctrl_page(void);


/* sched_trace injection */
int inject_name(void);
int inject_param(void); /* sporadic_task_ns*() must have already been called */
int inject_release(lt_t release, lt_t deadline, unsigned int job_no);
int inject_completion(unsigned int job_no);
int inject_gpu_migration(unsigned int to, unsigned int from);
int __inject_action(unsigned int action);

/*
#define inject_action(COUNT) \
do { \
unsigned int temp = (COUNT); \
printf("%s:%d:%d\n",__FUNCTION__,__LINE__,temp); \
__inject_action(temp); \
}while(0);
*/

#define inject_action(COUNT) \
do { \
}while(0);


/* Litmus signal handling */

typedef struct litmus_sigjmp
{
	sigjmp_buf env;
	struct litmus_sigjmp *prev;
} litmus_sigjmp_t;

void push_sigjmp(litmus_sigjmp_t* buf);
litmus_sigjmp_t* pop_sigjmp(void);

typedef void (*litmus_sig_handler_t)(int);
typedef void (*litmus_sig_actions_t)(int, siginfo_t *, void *);

/* ignore specified signals. all signals raised while ignored are dropped */
void ignore_litmus_signals(unsigned long litmus_sig_mask);

/* register a handler for the given set of litmus signals */
void activate_litmus_signals(unsigned long litmus_sig_mask,
				litmus_sig_handler_t handler);

/* register an action signal handler for a given set of signals */
void activate_litmus_signal_actions(unsigned long litmus_sig_mask,
				litmus_sig_actions_t handler);

/* Block a given set of litmus signals. Any signals raised while blocked
 * are queued and delivered after unblocking. Call ignore_litmus_signals()
 * before unblocking if you wish to discard these. Blocking may be
 * useful to protect COTS code in Litmus that may not be able to deal
 * with exception-raising signals.
 */
void block_litmus_signals(unsigned long litmus_sig_mask);

/* Unblock a given set of litmus signals. */
void unblock_litmus_signals(unsigned long litmus_sig_mask);

#define SIG_BUDGET_MASK			0x00000001
/* more ... */

#define ALL_LITMUS_SIG_MASKS	(SIG_BUDGET_MASK)

/* Try/Catch structures useful for implementing abortable jobs.
 * Should only be used in legitimate cases. ;)
 */
#define LITMUS_TRY \
do { \
	int sigsetjmp_ret_##__FUNCTION__##__LINE__; \
	litmus_sigjmp_t lit_env_##__FUNCTION__##__LINE__; \
	push_sigjmp(&lit_env_##__FUNCTION__##__LINE__); \
	sigsetjmp_ret_##__FUNCTION__##__LINE__ = \
		sigsetjmp(lit_env_##__FUNCTION__##__LINE__.env, 1); \
	if (sigsetjmp_ret_##__FUNCTION__##__LINE__ == 0) {

#define LITMUS_CATCH(x) \
	} else if (sigsetjmp_ret_##__FUNCTION__##__LINE__ == (x)) {

#define END_LITMUS_TRY \
	} /* end if-else-if chain */ \
} while(0); /* end do from 'LITMUS_TRY' */

/* Calls siglongjmp(signum). Use with TRY/CATCH.
 * Example:
 *  activate_litmus_signals(SIG_BUDGET_MASK, longjmp_on_litmus_signal);
 */
void longjmp_on_litmus_signal(int signum);

#ifdef __cplusplus
}
#endif




#ifdef __cplusplus
/* Expose litmus exceptions if C++.
 *
 * KLUDGE: We define everything in the header since liblitmus is a C-only
 * library, but this header could be included in C++ code.
 */

#include <exception>

namespace litmus
{
	class litmus_exception: public std::exception
	{
	public:
		litmus_exception() throw() {}
		virtual ~litmus_exception() throw() {}
		virtual const char* what() const throw() { return "litmus_exception";}
	};

	class sigbudget: public litmus_exception
	{
	public:
		sigbudget() throw() {}
		virtual ~sigbudget() throw() {}
		virtual const char* what() const throw() { return "sigbudget"; }
	};

	/* Must compile your program with "non-call-exception". */
	static __attribute__((used))
	void throw_on_litmus_signal(int signum)
	{
		/* We have to unblock the received signal to get more in the future
		 * because we are not calling siglongjmp(), which normally restores
		 * the mask for us.
		 */
		if (SIG_BUDGET == signum) {
			unblock_litmus_signals(SIG_BUDGET_MASK);
			throw sigbudget();
		}
		/* else if (...) */
		else {
			/* silently ignore */
		}
	}

}; /* end namespace 'litmus' */

#endif /* end __cplusplus */

#endif
