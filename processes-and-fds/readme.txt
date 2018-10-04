This code is designed to demonstrate forking processes and communicating via pipes using different system calls.

You can build/run/test this code in four modes: sequential, select, poll and epoll.
The commands to run these modes are: make run_sequential, make run_select, make run_poll and make run_epoll.

There should be no limitiations pertaining this code, except for the number of parallel workers allowed due to the limit on the number of open file descriptors allowed by the system. However, the code will automatically limit the number of workers allowed.

