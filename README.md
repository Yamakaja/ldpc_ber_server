# LDPC BER Server

This project contains all userspace application code responsible for controlling the SDFEC cores, scheduling tasks, user interaction, web APIs, etc.

Subprojects:

* `sdfec_python`: A python module that can be used to interact with the SDFEC driver API.
* `worker`: The worker application server that runs on the application processor of the individual boards. It is responsible for managing the hardware cores and accepting tasks from the master.
* `master`: The main applicaiton that coordinates workers and interacts with the users through a Web UI / API.
