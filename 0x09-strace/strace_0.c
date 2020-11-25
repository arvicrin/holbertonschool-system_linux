#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <x86_64-linux-gnu/sys/user.h>

/**
 * await_syscall - waits for a syscall
 * @child_pid: child pid
 * Return: 0 if child stopped, 1 if exited
 */
int await_syscall(pid_t child_pid)
{
        int status;

        while (1)
        {
                ptrace(PTRACE_SYSCALL, child_pid, 0, 0);
                waitpid(child_pid, &status, 0);
                if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
                        return (0);
                if (WIFEXITED(status))
                        return (1);
        }
}

/**
 * trace_parent - tracer
 * @child_pid: child pid
 */
void trace_parent(pid_t child_pid)
{
        int status;
        struct user_regs_struct uregs;

        waitpid(child_pid, &status, 0);
        ptrace(PTRACE_SETOPTIONS, child_pid, 0, PTRACE_O_TRACESYSGOOD);
        while (1)
        {
                if (await_syscall(child_pid))
                        break;
                memset(&uregs, 0, sizeof(uregs));
                ptrace(PTRACE_GETREGS, child_pid, 0, &uregs);
                printf("%lu\n", (long)uregs.orig_rax);
                if (await_syscall(child_pid))
                        break;
        }
}

/**
 * trace_child - tracee
 * @av: list of arguments
 * @envp: list of environ variables
 */
void trace_child(char **av, char **envp)
{
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        kill(getpid(), SIGSTOP);
        if (execve(av[1], av + 1, envp) == -1)
        {
                dprintf(STDERR_FILENO, "Exec failed: %d\n", errno);
                exit(-1);
        }
}

/**
 * main - entry point
 * @ac: number of arguments
 * @av: list of arguments
 * @envp: list of environ variables
 * Return: 0 on success, -1 otherwise
 */
int main(int ac, char **av, char **envp)
{
	pid_t child_pid;

	if (ac < 2)
	{
		printf("Usage: %s command [args...]\n", av[0]);
		return (EXIT_FAILURE);
	}
	setbuf(stdout, NULL);
	child_pid = fork();
	if (child_pid == -1)
	{
		dprintf(STDERR_FILENO, "Fork failed: %d\n", errno);
		exit(-1);
	}
	else if (!child_pid)
		trace_child(av, envp);
	else
		trace_parent(child_pid);
	return (0);
}
