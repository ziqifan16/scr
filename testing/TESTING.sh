# set up initial enviroment for testing
export scrbin=${SCR_INSTALL}/bin
export jobid=`${scrbin}/scr_env --jobid`
echo $jobid
export downnode=`${scrbin}/scr_glob_hosts -n 1 -h "$SLURM_NODELIST"`
echo $downnode
export prefix_files=".scr/flush.scr .scr/halt.scr .scr/nodes.scr"

export LD_LIBRARY_PATH=${SCR_INSTALL}/lib:${SCR_PKG}/deps/install/lib
export SCR_PREFIX=`pwd`
export SCR_FETCH=0
export SCR_FLUSH=0
export SCR_DEBUG=1
export SCR_LOG_ENABLE=0
export SCR_JOB_NAME=testing_job
export SCR_CACHE_SIZE=2

#export SCR_CONF_FILE=~/myscr.conf

#export BG_PERSISTMEMRESET=0
#export BG_PERSISTMEMSIZE=10

# clean out any cruft from previous runs
# deletes files from cache and any halt, flush, nodes files
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
rm -f ${prefix_files}


# check that a run works
srun -n4 -N4 ./test_api


# check that the next run bails out due to SCR_FINALIZE_CALLED
srun -n4 -N4 ./test_api


# remove halt file and run again, check that checkpoints continue where last run left off
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api


# delete all files from /ssd on rank 0, remove halt file, run again, check that rebuild works
rm -rf /tmp/${USER}/scr.$jobid
rm -rf /ssd/${USER}/scr.$jobid
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api


# delete latest checkpoint directory from two nodes, remove halt file, run again,
# check that rebuild works for older checkpoint
srun -n2 -N2 /bin/rm -rf /ssd/${USER}/scr.$jobid/scr.dataset.18
srun -n2 -N2 /bin/rm -rf /tmp/${USER}/scr.$jobid/scr.dataset.18
srun -n2 -N2 /bin/rm -rf /tmp/${USER}/scr.$jobid/scr.dataset.18
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api


# delete all files from all nodes, remove halt file, run again, check that run starts over
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api


# clear the cache and control directory
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
rm -f ${prefix_files}


# check that scr_list_dir returns good values
${scrbin}/scr_list_dir control
${scrbin}/scr_list_dir --base control
${scrbin}/scr_list_dir cache
${scrbin}/scr_list_dir --base cache


# check that scr_list_down_nodes returns good values
${scrbin}/scr_list_down_nodes
${scrbin}/scr_list_down_nodes --down $downnode
${scrbin}/scr_list_down_nodes --reason --down $downnode
export SCR_EXCLUDE_NODES=$downnode
${scrbin}/scr_list_down_nodes
${scrbin}/scr_list_down_nodes --reason
unset SCR_EXCLUDE_NODES


# check that scr_halt seems to work
${scrbin}/scr_halt --list `pwd`; sleep 5
${scrbin}/scr_halt --before '3pm today' `pwd`; sleep 5
${scrbin}/scr_halt --after '4pm today' `pwd`; sleep 5
${scrbin}/scr_halt --seconds 1200 `pwd`; sleep 5
${scrbin}/scr_halt --unset-before `pwd`; sleep 5
${scrbin}/scr_halt --unset-after `pwd`; sleep 5
${scrbin}/scr_halt --unset-seconds `pwd`; sleep 5
${scrbin}/scr_halt `pwd`; sleep 5
${scrbin}/scr_halt --checkpoints 5 `pwd`; sleep 5
${scrbin}/scr_halt --unset-checkpoints `pwd`; sleep 5
${scrbin}/scr_halt --unset-reason `pwd`; sleep 5
${scrbin}/scr_halt --remove `pwd`


# check that scr_env seems to work
${scrbin}/scr_env --user
${scrbin}/scr_env --jobid
${scrbin}/scr_env --nodes
${scrbin}/scr_env --down


# check that scr_prerun works
${scrbin}/scr_prerun


# check that scr_postrun works (w/ empty cache)
${scrbin}/scr_postrun


# clear the cache, make a new run, and check that scr_postrun scavenges successfully (no rebuild)
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
srun -n4 -N4 ./test_api
${scrbin}/scr_postrun
${scrbin}/scr_index --list


# fake a down node via EXCLUDE_NODES and redo above test (check that rebuild during scavenge works)
export SCR_EXCLUDE_NODES=$downnode
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api
${scrbin}/scr_postrun
unset SCR_EXCLUDE_NODES
${scrbin}/scr_index --list


# delete all files, enable fetch, run again, check that fetch succeeds
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
${scrbin}/scr_halt -r `pwd`
export SCR_FETCH=1
srun -n4 -N4 ./test_api
${scrbin}/scr_index --list


# delete all files from 2 nodes, remove halt file, run again, check that distribute fails but fetch succeeds
srun -n2 -N2 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n2 -N2 /bin/rm -rf /ssd/${USER}/scr.$jobid
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api
${scrbin}/scr_index --list


# delete all files, corrupt file on disc, run again, check that fetch of current fails but old succeeds
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
#vi -b ${SCR_INSTALL}/share/scr/examples/scr.dataset.12/rank_2.ckpt
sed -i 's/\?/i/' ${SCR_INSTALL}/share/scr/examples/scr.dataset.12/rank_2.ckpt
# change some characters and save file (:wq)
${scrbin}/scr_halt -r `pwd`
srun -n4 -N4 ./test_api
${scrbin}/scr_index --list


# remove halt file, enable flush, run again and check that flush succeeds and that postrun realizes that
${scrbin}/scr_halt -r `pwd`
export SCR_FLUSH=10
srun -n4 -N4 ./test_api
${scrbin}/scr_postrun
${scrbin}/scr_index --list


# clear cache and check that scr_srun works
srun -n4 -N4 /bin/rm -rf /tmp/${USER}/scr.$jobid
srun -n4 -N4 /bin/rm -rf /ssd/${USER}/scr.$jobid
rm -f ${prefix_files}
${scrbin}/scr_srun -n4 -N4 ./test_api
${scrbin}/scr_index --list

