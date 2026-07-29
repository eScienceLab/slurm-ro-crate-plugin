#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <slurm/slurm.h>
#include <slurm/spank.h>

SPANK_PLUGIN(spank_crate, 1)

static int dump_argv(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
        slurm_debug("spank_crate:   argv[%d] = [%s]", i, argv[i]);
    return SLURM_SUCCESS;
}

int slurm_spank_init(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> init context=%d argc=%d", spank_context(), ac);
    dump_argv(ac, av);

    slurm_debug("spank_crate: task_init supported=%d",
        spank_symbol_supported("slurm_spank_task_init"));
    slurm_debug("spank_crate: exit supported=%d",
        spank_symbol_supported("slurm_spank_exit"));
    slurm_debug("spank_crate: job_epilog supported=%d",
        spank_symbol_supported("slurm_spank_job_epilog"));

    return SLURM_SUCCESS;
}

int slurm_spank_job_prolog(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> job_prolog remote=%d argc=%d", spank_remote(sp), ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_init_post_opt(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> init_post_opt remote=%d argc=%d", spank_remote(sp), ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_local_user_init(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> local_user_init argc=%d", ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_user_init(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> user_init remote=%d argc=%d", spank_remote(sp), ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_task_init_privileged(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> task_init_privileged argc=%d", ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_task_init(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> task_init argc=%d", ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_task_post_fork(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> task_post_fork argc=%d", ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_task_exit(spank_t sp, int ac, char *av[])
{
    int status;
    slurm_debug("spank_crate: -> task_exit argc=%d", ac);
    dump_argv(ac, av);
    if (spank_get_item(sp, S_TASK_EXIT_STATUS, &status) == ESPANK_SUCCESS)
        slurm_debug("spank_crate: exit_status=%d", status);
    return SLURM_SUCCESS;
}

int slurm_spank_exit(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> exit remote=%d argc=%d", spank_remote(sp), ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}

int slurm_spank_job_epilog(spank_t sp, int ac, char *av[])
{
    slurm_debug("spank_crate: -> job_epilog remote=%d argc=%d", spank_remote(sp), ac);
    dump_argv(ac, av);
    return SLURM_SUCCESS;
}
