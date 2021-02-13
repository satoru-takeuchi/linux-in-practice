#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
 
#define NLOOP_FOR_ESTIMATION 1000000000UL
#define NSECS_PER_MSEC 1000000UL
#define NSECS_PER_SEC 1000000000UL

static unsigned long nloop_per_resol;
static struct timespec start;

static inline long diff_nsec(struct timespec before, struct timespec after)
{
        return ((after.tv_sec * NSECS_PER_SEC + after.tv_nsec)
                - (before.tv_sec * NSECS_PER_SEC + before.tv_nsec));
	
}

static unsigned long estimate_loops_per_msec()
{
        struct timespec before, after;
        clock_gettime(CLOCK_MONOTONIC, &before);

        unsigned long i;
        for (i = 0; i < NLOOP_FOR_ESTIMATION; i++)
		;

        clock_gettime(CLOCK_MONOTONIC, &after);

	int ret;
        return  NLOOP_FOR_ESTIMATION * NSECS_PER_MSEC / diff_nsec(before, after);
}
 
static inline void load(void)
{
        unsigned long i;
        for (i = 0; i < nloop_per_resol; i++)
                ;
}

static void child_fn(int id, struct timespec *buf, int nrecord)
{
        int i;
        for (i = 0; i < nrecord; i++) {
                struct timespec ts;

                load();
                clock_gettime(CLOCK_MONOTONIC, &ts);
                buf[i] = ts;
        }
        for (i = 0; i < nrecord; i++) {
                printf("%d\t%ld\t%d\n", id, diff_nsec(start, buf[i]) / NSECS_PER_MSEC, (i + 1) * 100 / nrecord);
        }
        exit(EXIT_SUCCESS);
}
 
static pid_t *pids;

int main(int argc, char *argv[])
{
        int ret = EXIT_FAILURE;

        if (argc < 4) {
                fprintf(stderr, "usage: %s <nproc> <total[ms]> <resolution[ms]>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        int nproc = atoi(argv[1]);
        int total = atoi(argv[2]);
        int resol = atoi(argv[3]);

        if (nproc < 1) {
                fprintf(stderr, "<nproc>(%d) should be >= 1\n", nproc);
                exit(EXIT_FAILURE);
        }

        if (total < 1) {
                fprintf(stderr, "<total>(%d) should be >= 1\n", total);
                exit(EXIT_FAILURE);
        }

        if (resol < 1) {
                fprintf(stderr, "<resol>(%d) should be >= 1\n", resol);
                exit(EXIT_FAILURE);
        }

        if (total % resol) {
                fprintf(stderr, "<total>(%d) should be multiple of <resolution>(%d)\n", total, resol);
                exit(EXIT_FAILURE);
        }
        int nrecord = total / resol;

        struct timespec *logbuf = malloc(nrecord * sizeof(struct timespec));
	if (!logbuf)
		err(EXIT_FAILURE, "failed to allocate log buffer");

	puts("estimating the workload which takes just one milli-second...");
        nloop_per_resol = estimate_loops_per_msec() * resol;
	puts("end estimation");
	fflush(stdout);

        pids = malloc(nproc * sizeof(pid_t));
        if (pids == NULL)
                err(EXIT_FAILURE, "failed to allocate pid table");

        clock_gettime(CLOCK_MONOTONIC, &start);

	ret = EXIT_SUCCESS;
        int i, ncreated;
        for (i = 0, ncreated = 0; i < nproc; i++, ncreated++) {
                pids[i] = fork();
                if (pids[i] < 0) {
			int j;
                	for (j = 0; j < ncreated; j++)
                        	kill(pids[j], SIGKILL);
			ret = EXIT_FAILURE;
                        break;
                } else if (pids[i] == 0) {
                        // children
                        child_fn(i, logbuf, nrecord);
                        /* shouldn't reach here */
			abort();
                }
        }
        // parent
        for (i = 0; i < ncreated; i++)
                if (wait(NULL) < 0)
                        warn("wait() failed.");

        exit(ret);
}
