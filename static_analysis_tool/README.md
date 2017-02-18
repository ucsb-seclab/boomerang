# Finding Boomerang with angr

In this directory are some scripts that help you find BOOMERANG vulnerabilities in Trusted Applications (TAs)
These leverage angr (http://www.angr.io/) to perform static analysis and symbolic execution

The main goal here is to find a place where input from the un-trusted OS (Android) can cause the Trusted OS and its Trusted Applications to read and/or write data that belongs to the untrusted OS, to do terrible things.
For example, you might leverage the signing function of the KeyMaster TA included in most TrustZone implementations to sign a byte that happens to sign to 0 over some juicy Linux kernel structures, or use it with a ret2dir-style attack to get a root shell.

Here, we include two versions.  The symbolic version is the real deal, but we include both for educational purposes:

* ***Static Version*** Uses angr to trace the callgraph backward from a sensitive function (qsee_register_shared_buffer, in this example) and can therefore help an analyst quickly figure out what's going on.
This ends up being a great benchmark for angr's CFG, function recovery, and callgraph completeness, as well as a nice succinct example of how to do simple static analysis on real, messy binaries.
* ***Symbolic Version*** Uses angr to perform "blanket execution" to find paths forward from the entry point to a write to memory or syscall (that is known to write to memory)  If user-provided data can influence where memory is written, or are used to tell a syscall where to read/write, this is super bad.

