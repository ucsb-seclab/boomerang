import sys

import angr
import pyvex
import claripy
import simuvex
import logging

l = logging.getLogger("analysis")
#l.setLevel("DEBUG")

glob_detected_addrs = set()
glob_detected_callstacks = set()
glob_curr_callstack = ()
glob_counter = 0

# this inserts fake rets as the return value for functions. Creates more paths but allows us to quickly skip unneeded functions
# for malloc in playready we need to either use this or explicitly replace malloc so it has fake return values, otherwise we see false positives
# needed to bypass angr errors in huawei
USE_FAKE_RETS=True

# this requires addresses to only depend on the input buffer and not on other uninitialized data such us the initial r9 value
# setting this to false removes multiple false positives seen where the address read is r9+user_val
ALLOW_MULTIPLE_DEPENDENCIES = False


def the_memhook(state, addr):
    """
    Checks if an address is tainted
    """
    if not ALLOW_MULTIPLE_DEPENDENCIES:
        if len(addr.variables) != 1:
            return False
    for v in addr.variables:
        if v.startswith("fake"):
            return False
    for v in addr.variables:
        if v.startswith("in_buf"):
            return True
    return False

def syscall_hook(state):
    """
    called at each syscall
    checks arguments to see if they are tainted
    """
    global glob_detected_callstacks
    global glob_curr_callstack

    r1 = state.regs.r1
    r2 = state.regs.r2
    r3 = state.regs.r3
    for addr in [r1, r2, r3]:
        if the_memhook(state, addr):
            ip = state.se.any_int(state.regs.ip_at_syscall)
            syscall = state.se.any_int(state.regs.r0)
            if glob_curr_callstack not in glob_detected_callstacks:
                l.warning("detected call %#x with arg we control at ip %#x", syscall, ip)
                l.warning("arg %s", addr)
                l.warning("callstack %s\n", map(hex, glob_curr_callstack))
                glob_detected_callstacks.add(glob_curr_callstack)
                # import ipdb; ipdb.set_trace()
    # avoid the syscall
    state.regs.lr = 0x41414141
    state.regs.ip_at_syscall = 0x41414141

def write_hook(state):
    """
    Called at each memory write
    checks if the address is tainted
    """
    global glob_detected_addrs
    addr = state.inspect.mem_write_address
    if the_memhook(state, addr):
        ip = state.se.any_int(state.regs.pc)
        fake_callstack = glob_curr_callstack + (ip,)
        if fake_callstack not in glob_detected_callstacks:
            l.warning("detected write to addr we control at ip %#x", ip)
            l.warning("addr %s", addr)
            l.warning("callstack %s\n", map(hex, glob_curr_callstack))
            glob_detected_callstacks.add(fake_callstack)
            # import ipdb; ipdb.set_trace()

def read_hook(state):
    """
    Called at each memory read
    checks if the address is tainted
    """
    global glob_detected_addrs
    addr = state.inspect.mem_read_address
    if the_memhook(state, addr):
        ip = state.se.any_int(state.regs.pc)
        fake_callstack = glob_curr_callstack + (ip,)
        if fake_callstack not in glob_detected_callstacks:
            l.warning("detected read from addr we control at ip %#x", ip)
            l.warning("addr %s", addr)
            l.warning("callstack %s\n", map(hex, glob_curr_callstack))
            glob_detected_callstacks.add(fake_callstack)
            # import ipdb; ipdb.set_trace()


def _arm_thumb_filter_jump_successors(project, cfg, addr, successors):
    """
    Filter successors for THUMB mode basic blocks, and remove those successors that won't be taken normally.

    :param int addr: Address of the basic block / SimIRSB.
    :param list successors: A list of successors.
    :return: A new list of successors after filtering.
    :rtype: list
    """

    if not successors:
        return []

    if addr % 2 == 0:
        return successors

    node = cfg.get_any_node(addr)
    if node is not None:
        non_filterable = [n.addr for n in node.successors]
    else:
        non_filterable = set()

    it_counter = 0
    conc_temps = {}
    can_produce_exits = set()
    bb = project.factory.block(addr, thumb=True, opt_level=0)

    for stmt in bb.vex.statements:
        if stmt.tag == 'Ist_IMark':
            if it_counter > 0:
                it_counter -= 1
                can_produce_exits.add(stmt.addr)
        elif stmt.tag == 'Ist_WrTmp':
            val = stmt.data
            if val.tag == 'Iex_Const':
                conc_temps[stmt.tmp] = val.con.value
        elif stmt.tag == 'Ist_Put':
            if stmt.offset == project.arch.registers['itstate'][0]:
                val = stmt.data
                if val.tag == 'Iex_RdTmp':
                    if val.tmp in conc_temps:
                        # We found an IT instruction!!
                        # Determine how many instructions are conditional
                        it_counter = 0
                        itstate = conc_temps[val.tmp]
                        while itstate != 0:
                            it_counter += 1
                            itstate >>= 8

    #if it_counter != 0:
        #l.debug('Basic block ends before calculated IT block (%#x)', addr)

    THUMB_BRANCH_INSTRUCTIONS = ('beq', 'bne', 'bcs', 'bhs', 'bcc', 'blo', 'bmi', 'bpl', 'bvs',
                                 'bvc', 'bhi', 'bls', 'bge', 'blt', 'bgt', 'ble', 'cbz', 'cbnz')
    for cs_insn in bb.capstone.insns:
        if cs_insn.mnemonic in THUMB_BRANCH_INSTRUCTIONS:
            can_produce_exits.add(cs_insn.address)

    curr_addr = -1
    for stmt in bb.vex.statements:
        if stmt.tag == 'Ist_IMark':
            curr_addr = stmt.addr
        if isinstance(stmt, pyvex.stmt.Exit) and curr_addr not in can_produce_exits and stmt.dst.value not in non_filterable:
            if len([s for s in successors if s.addr == stmt.dst.value]) <= 1:
                successors = [s for s in successors if s.addr != stmt.dst.value]
            else:
                unsat_ones = [s for s in successors if s.addr == stmt.dst.value and not s.state.satisfiable()]
                sat_ones = [s for s in successors if s.addr == stmt.dst.value and s.state.satisfiable()]
                if len(unsat_ones+sat_ones) > 0:
                    successors = [s for s in successors if s != (unsat_ones+sat_ones)[0]]

    return successors


def blanket_execution(project, cfg, starts):
    """
    the blanket execution function with data dependency tracking
    """
    global glob_counter
    global glob_curr_callstack
    curr_paths = []
    seen_callstack_addrs = set()
    processed_callstack_addr_jk = set()

    # create the initial paths with options to not use the symbolic stuff in angr
    for s in starts:
        s = s.copy()
        s.options = b.factory.blank_state(mode="fastpath").options
        s.options.add(simuvex.o.AVOID_MULTIVALUED_READS)
        s.options.add(simuvex.o.AVOID_MULTIVALUED_WRITES)
        s.options.add(simuvex.o.NO_SYMBOLIC_JUMP_RESOLUTION)
        s.options.add(simuvex.o.NO_SYMBOLIC_SYSCALL_RESOLUTION)
        s.mode = "fastpath"
        callstack = (s.se.any_int(s.regs.pc),)
        curr_paths.append((callstack, project.factory.path(s)))
        seen_callstack_addrs.add((callstack, s.se.any_int(s.regs.pc)))

        s.inspect.b(
            'mem_write',
            simuvex.BP_BEFORE,
            action=write_hook
        )
        s.inspect.b(
            'mem_read',
            simuvex.BP_BEFORE,
            action=read_hook
        )

        s.inspect.b(
            'syscall',
            simuvex.BP_BEFORE,
            action=syscall_hook
        )

    # keep stepping the blanket execution in a breadth-first order
    # this probably won't terminate but should work for binaries of the size we are analyzing
    while curr_paths:
        callstack, p = curr_paths.pop(0)

        # don't re-analyze the same (callstack, address, jumpkind)
        if (callstack, p.addr, p.jumpkind) in processed_callstack_addr_jk:
            continue

        processed_callstack_addr_jk.add((callstack, p.addr, p.jumpkind))

        glob_curr_callstack = callstack

        current_path = p

        if p.addr == 0x41414141:
            continue
        l.debug("processing %#x", p.addr)

        # do the step
        node = cfg.get_any_node(p.addr)

        if node is not None:
            current_path.step(max_size=node.size)
        else:
            current_path.step()

        # handle imports which angr seems to call the same address (just run the syscall detection)
        if any(p.addr == current_path.addr and p.jumpkind == "Ijk_Call" for p in current_path.successors):
            syscall_hook(p.state)
            continue

        added_succ = []
        # add fake rets at calls
        if any(p.jumpkind == "Ijk_Call" for p in (current_path.successors + current_path.unsat_successors + current_path.unconstrained_successors)):
            if not any(p.jumpkind == "Ijk_FakeRet" for p in (current_path.successors + current_path.unsat_successors + current_path.unconstrained_successors)):

                new_s = [p for p in (current_path.successors + current_path.unsat_successors + current_path.unconstrained_successors)][0].state.copy()
                new_s.scratch.jumpkind = "Ijk_FakeRet"
                new_s.regs.pc = new_s.regs.lr
                new_s.regs.lr = current_path.state.regs.lr
                new_p = project.factory.path(new_s)
                added_succ.append(new_p)

        # find successors in the cfg that aren't in the current successors
        if node is not None:
            filtered_succ = [x for x in node.successors if x.name is None or x.name != "UnresolvableTarget"]
            missing = set(x.addr for x in filtered_succ)
        else:
            missing = set()
        missing -= set(x.addr for x in (current_path.successors + current_path.unsat_successors))

        if len(missing) > 0:
            pass

        to_append_succs = []
        non_no_decode = [p for p in (current_path.successors+current_path.unsat_successors+added_succ) if p.jumpkind != "Ijk_NoDecode"]

        # if no valid successors skip this path
        if len(non_no_decode) == 0:
            continue

        non_no_decode_orig = non_no_decode
        # filter fake successors
        filtered_succ = _arm_thumb_filter_jump_successors(project, cfg, current_path.addr, non_no_decode)
        non_no_decode = filtered_succ

        # more filtering of fake successors (vex is weird)
        if any(p.jumpkind == "Ijk_Ret" for p in non_no_decode):
            non_no_decode = [p for p in non_no_decode if p.jumpkind == "Ijk_Ret"]

        to_append_succs.extend(non_no_decode)

        # force jumps to missing
        if len(non_no_decode) > 0:
            any_succ = non_no_decode[0]
            for m in missing:
                new_succ = any_succ.copy()
                new_succ.state.ip = m
                to_append_succs.append(new_succ)
                # fixme do I need to handle jumpkinds?

        append_succs = []
        # append successors to the worklist as needed
        for p in to_append_succs:
            try:
                if p.jumpkind == "Ijk_FakeRet" and not USE_FAKE_RETS:
                    continue

                if p.jumpkind == "Ijk_Call":
                    ret_addr = p.state.se.any_int(p.state.regs.lr)
                    if ret_addr in callstack:
                        continue
                    new_callstack = callstack + (ret_addr,)
                elif p.jumpkind == "Ijk_Ret" and "Ijk_Sys" not in current_path.jumpkind:
                    if len(callstack) == 0:
                        pass
                    new_callstack = callstack[:-1]
                elif len(callstack) > 0 and p.addr == callstack[-1]:
                    # handle cases where it doesn't look like a call to vex
                    if len(callstack) == 0:
                        pass
                    new_callstack = callstack[:-1]
                else:
                    new_callstack = callstack

                if (new_callstack, p.addr) in seen_callstack_addrs:
                    continue

                # don't let fakerets prevent normal rets
                if p.jumpkind != 'Ijk_FakeRet':
                    seen_callstack_addrs.add((new_callstack, p.addr))
                # make sure fakerets write over r0
                if p.jumpkind == "Ijk_FakeRet":
                    p.state.regs.r0 = claripy.BVS("fakeret", 32)

                # discard constraints
                p.state.release_plugin('solver_engine')
                p.state.downsize()
                append_succs.append((new_callstack, p))
            except simuvex.SimUnsatError:
                pass

        glob_counter += 1
        if glob_counter == 3000000:
            import ipdb; ipdb.set_trace()
        if glob_counter % 1 == 0:
            l.debug("appending %s", map(lambda x:hex(x[1].addr), append_succs))
            l.debug("callstack lengths %s", map(lambda x:len(x[0]), append_succs))
        curr_paths.extend(append_succs)

if __name__ == "__main__":

    filename = sys.argv[1]
    b = angr.Project(filename, load_options={"main_opts":{"custom_base_addr":0x0}})

    # hardcode the test binaries so I don't have to look it up every time
    if filename == "qcom/keymaster.elf":
        identified_func_addr = 0xD45
    elif filename == "qcom/playready.elf":
        identified_func_addr = 0x49F0
    elif filename == "huawei_tlets/keymaster.elf":
        identified_func_addr = 0xE48
    else:
        if len(sys.argv) < 3:
            print "need to give the address of the dispatcher as the 3rd argument"
            sys.exit(1)
        identified_func_addr = int(sys.argv[2], 16)

    if "huawei" in filename:
        style = "huawei"
    elif "qcom" in filename:
        style = "qcom"

    extra_funcs=[identified_func_addr]
    args = [claripy.Concat(claripy.BVS("switch_num", 32), claripy.BVS("in_buf", 0x1000 * 8)), claripy.BVS("in_len", 32),
            claripy.BVS("out_buf", 0x1000 * 8), claripy.BVS("out_len", 32)]
    starts = []

    # identify jump table targets
    s = b.factory.blank_state()
    s.regs.pc = identified_func_addr
    s.memory.store(0x1000000, args[0])
    s.memory.store(0x2000000, args[2])
    s.regs.r0 = 0x1000000
    s.regs.r1 = args[1]
    s.regs.r2 = 0x2000000
    s.regs.r3 = args[3]
    s.regs.lr = 0x41414141
    if style == "huawei":
        s.add_constraints(s.regs.r1 < 0x60)

    pg = b.factory.path_group(s)
    pg.explore(find=lambda x:x.jumpkind == "Ijk_Call", num_find=100)

    for p in pg.found:
        starts.append(p.state)

    extra_funcs.extend([p.addr for p in pg.found])
    cfg = b.analyses.CFGFast(resolve_indirect_jumps=True, function_starts=extra_funcs, show_progressbar=True)

    if identified_func_addr is not None and style != "huawei":
        func = cfg.functions[identified_func_addr]

        last_state = b.factory.blank_state(addr=identified_func_addr)
        last_state.memory.store(0x1000000, args[0])
        last_state.memory.store(0x2000000, args[2])
        last_state.regs.r0 = 0x1000000
        last_state.regs.r1 = args[1]
        last_state.regs.r2 = 0x2000000
        last_state.regs.r3 = args[3]
        last_state.regs.lr = 0x41414141

        starts.append(last_state)
    if style == "huawei":
        for a in list(starts):
            s = b.factory.blank_state(addr=s.se.any_int(s.ip))
            s.memory.store(0x1000000, claripy.BVS("in_buf", 0x100 * 8))
            # only every other one is a pointer
            for i in range(80):
                s.memory.store(0x1000004 + i*2*4, claripy.BVS("foo", 32))
            s.regs.r0 = claripy.BVS("r0", 32)
            s.regs.r1 = 0x1000000
            s.regs.lr = 0x41414141

            starts.append(s)

    l.warning("Running on %s", filename)
    blanket_execution(b, cfg, starts)


