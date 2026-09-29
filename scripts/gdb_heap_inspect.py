"""
GDB Python script to inspect active allocations in mini_libc.

Usage:
  In GDB:
    (gdb) source /path/to/gdb_heap_inspect.py
    (gdb) list-allocations

Functions performed:
  1. Checks if execution is currently inside a malloc-internal function;
     if so, executes 'finish' until returning to user code.
  2. Reads $TPIDR_EL0 (or $tpidr_el0) to retrieve the TLS structure and
     the thread's malloc_arena_t address.
  3. Traverses active allocations across small-class, buddy, and mmap tiers.
  4. Filters out internal allocations (small_class_block buddy allocations,
     buddy_chunk_state_group mmap allocations, and buddy_chunk mmap allocations).
  5. Decodes alignment shifts and the tagged flag (bit 2 of metadata 'y').
  6. Displays a formatted table of all active allocations.
"""

import gdb


def finish_malloc_internal():
    """
    If the inferior is currently paused inside any malloc-internal function,
    finish executing frames until returning to user code.
    """
    malloc_funs = (
        "allocate_mmap_record_group", "mmap_alloc", "mmap_free",
        "allocate_buddy_chunk_state_group", "allocate_buddy_chunk_state", "free_buddy_chunk_state", "allocate_chunk_and_block6",
        "buddy_alloc_6", "buddy_alloc_5", "buddy_alloc_4", "buddy_alloc_3", "buddy_alloc_2", "buddy_alloc_1", "buddy_alloc_0",
        "buddy_free_6", "buddy_free_5", "buddy_free_4", "buddy_free_3", "buddy_free_2", "buddy_free_1", "buddy_free_0",
        "allocate_class_block", "small_alloc", "small_free",
        "get_thread_malloc_arena", "malloc_init", "malloc_with_arena", "aligned_alloc_with_arena", "aligned_tagged_alloc_with_arena", "free_with_arena_internal", "insert_into_free_set_of_arena", "clear_free_set_of_arena", "free_with_arena",
        "malloc", "aligned_alloc", "aligned_tagged_alloc", "free", "clear_free_set"
    )
    while True:
        try:
            f = gdb.newest_frame()
        except gdb.error:
            break
        is_malloc = False
        while f is not None:
            name = f.name()
            if name and name in malloc_funs:
                is_malloc = True
                break
            f = f.older()

        if is_malloc:
            if f.older() is not None:
                gdb.execute("finish", to_string=True)
            else:
                break
        else:
            break


class ListAllocations(gdb.Command):
    """List all active allocations in mini_libc malloc arena, filtering internal structures."""

    def __init__(self):
        super(ListAllocations, self).__init__("list-allocations", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        finish_malloc_internal()

        try:
            tpidr = int(gdb.parse_and_eval("$TPIDR_EL0"))
        except gdb.error:
            try:
                tpidr = int(gdb.parse_and_eval("$tpidr_el0"))
            except gdb.error as e:
                print("Error: Could not read $TPIDR_EL0 register: %s" % e)
                return

        if tpidr == 0:
            print("TPIDR_EL0 is 0. Malloc arena is not initialized.")
            return

        tls = gdb.parse_and_eval("*(struct tls_struct *)%d" % tpidr)
        arena_ptr = tls['malloc_arena']
        if int(arena_ptr) == 0:
            print("malloc_arena pointer in TLS is NULL.")
            return

        arena = arena_ptr.dereference()
        print("Malloc Arena Address: 0x%016x" % int(arena_ptr))
        print("-" * 75)
        print("%-18s %-12s %-10s %-14s" % ("User Address", "Class Size", "Tagged", "Allocator"))
        print("-" * 75)

        total_allocations = 0
        total_bytes = 0

        # -------------------------------------------------------------
        # 1. Small Class Allocations
        # -------------------------------------------------------------
        small_arena = arena['small_class_arena']
        block_sizes = {}

        # Scan available lists to identify sizes of non-full blocks
        avail_lists = small_arena['small_class_avail_lists']
        for idx in range(16):
            cur = avail_lists[idx]
            size = (idx + 1) * 32
            while int(cur) != 0:
                block_sizes[int(cur)] = size
                cur = cur.dereference()['next_avail_block']

        cur = small_arena['class1024_avail_list']
        while int(cur) != 0:
            block_sizes[int(cur)] = 1024
            cur = cur.dereference()['next_avail_block']

        cur = small_arena['class2048_avail_list']
        while int(cur) != 0:
            block_sizes[int(cur)] = 2048
            cur = cur.dereference()['next_avail_block']

        small_class_blocks = set()
        cur = small_arena['block_list']
        while int(cur) != 0:
            b_addr = int(cur)
            small_class_blocks.add(b_addr)
            block = cur.dereference()

            # If block has avail_num == 0, it is not in any avail_list.
            # Deduce its class size from slot 0 metadata.
            if b_addr not in block_sizes:
                slot0 = b_addr + 304
                first_word = int(gdb.parse_and_eval("*(uint64_t *)%d" % slot0))
                if (first_word & 0x8000000000000000) != 0:
                    shift = first_word & 0x7fffffffffffffff
                else:
                    shift = 0
                x = int(gdb.parse_and_eval("*(uint64_t *)%d" % (slot0 + shift + 8)))
                y = int(gdb.parse_and_eval("*(uint64_t *)%d" % (slot0 + shift)))
                s = (x & 0x07) | ((y & 0x03) << 3)
                if 1 <= s <= 16:
                    size = s * 32
                elif s == 17:
                    size = 1024
                elif s == 18:
                    size = 2048
                else:
                    size = 0
                block_sizes[b_addr] = size

            size = block_sizes.get(b_addr, 0)
            if size > 0:
                avail_num_total = (65536 - 304) // size
                bitmaps = [int(block['bitmap'][i]) for i in range(32)]
                for i in range(avail_num_total):
                    w_idx = i // 64
                    b_idx = i % 64
                    is_free = (bitmaps[w_idx] >> b_idx) & 1
                    if not is_free:
                        slot_addr = b_addr + 304 + i * size
                        first_word = int(gdb.parse_and_eval("*(uint64_t *)%d" % slot_addr))
                        if (first_word & 0x8000000000000000) != 0:
                            shift = first_word & 0x7fffffffffffffff
                        else:
                            shift = 0
                        y = int(gdb.parse_and_eval("*(uint64_t *)%d" % (slot_addr + shift)))
                        user_ptr = slot_addr + shift + 16
                        is_tagged = (y & 0x04) != 0
                        print("0x%016x %-12d %-10s %-14s" % (
                            user_ptr, size, "Tagged" if is_tagged else "No", "small_class"
                        ))
                        total_allocations += 1
                        total_bytes += size

            cur = block['next_block']

        # -------------------------------------------------------------
        # 2. Buddy Allocations
        # -------------------------------------------------------------
        buddy_arena = arena['buddy_arena']
        group_cur = buddy_arena['group_list_head']
        buddy_groups = set()
        buddy_chunks = set()
        buddy_group_size = (4096 - 24) // 200

        while int(group_cur) != 0:
            g_addr = int(group_cur)
            buddy_groups.add(g_addr)
            group = group_cur.dereference()
            for r_idx in range(buddy_group_size):
                st = group['state_records'][r_idx]
                if int(st['in_use']) == 1:
                    buddy_chunks.add(int(st['chunk']))
            group_cur = group['next_group']

        group_cur = buddy_arena['group_list_head']
        while int(group_cur) != 0:
            group = group_cur.dereference()
            for r_idx in range(buddy_group_size):
                st = group['state_records'][r_idx]
                if int(st['in_use']) == 1:
                    chunk = int(st['chunk'])
                    bm = [
                        [int(st['bitmap0'][0]), int(st['bitmap0'][1])],
                        int(st['bitmap1']),
                        int(st['bitmap2']),
                        int(st['bitmap3']),
                        int(st['bitmap4']),
                        int(st['bitmap5']),
                        int(st['bitmap6'])
                    ]
                    sp = [
                        0,
                        int(st['split1']),
                        int(st['split2']),
                        int(st['split3']),
                        int(st['split4']),
                        int(st['split5']),
                        int(st['split6'])
                    ]

                    def walk_buddy(order, idx):
                        nonlocal total_allocations, total_bytes
                        if order == 0:
                            is_free = (bm[0][0] >> idx) & 1 if idx < 64 else (bm[0][1] >> (idx - 64)) & 1
                            is_split = 0
                        else:
                            is_free = (bm[order] >> idx) & 1
                            is_split = (sp[order] >> idx) & 1

                        if is_free:
                            return
                        if is_split:
                            walk_buddy(order - 1, 2 * idx)
                            walk_buddy(order - 1, 2 * idx + 1)
                        else:
                            block_addr = chunk + (idx << (12 + order))
                            block_size = 1 << (12 + order)
                            # Exclude internal small_class_block allocations (order 4)
                            if order == 4 and block_addr in small_class_blocks:
                                return
                            first_word = int(gdb.parse_and_eval("*(uint64_t *)%d" % block_addr))
                            if (first_word & 0x8000000000000000) != 0:
                                shift = first_word & 0x7fffffffffffffff
                            else:
                                shift = 0
                            y = int(gdb.parse_and_eval("*(uint64_t *)%d" % (block_addr + shift)))
                            user_ptr = block_addr + shift + 16
                            is_tagged = (y & 0x04) != 0
                            print("0x%016x %-12d %-10s %-14s" % (
                                user_ptr, block_size, "Tagged" if is_tagged else "No", "buddy (ord %d)" % order
                            ))
                            total_allocations += 1
                            total_bytes += block_size

                    walk_buddy(6, 0)
                    walk_buddy(6, 1)

            group_cur = group['next_group']

        # -------------------------------------------------------------
        # 3. Mmap Allocations
        # -------------------------------------------------------------
        mmap_arena = arena['mmap_arena']
        rec_cur = mmap_arena['active_list_head']
        while int(rec_cur) != 0:
            rec = rec_cur.dereference()
            ptr = int(rec['ptr'])
            length = int(rec['len'])
            # Exclude internal allocations (buddy state groups and buddy chunks)
            if ptr not in buddy_groups and ptr not in buddy_chunks:
                first_word = int(gdb.parse_and_eval("*(uint64_t *)%d" % ptr))
                if (first_word & 0x8000000000000000) != 0:
                    shift = first_word & 0x7fffffffffffffff
                else:
                    shift = 0
                y = int(gdb.parse_and_eval("*(uint64_t *)%d" % (ptr + shift)))
                user_ptr = ptr + shift + 16
                is_tagged = (y & 0x04) != 0
                print("0x%016x %-12d %-10s %-14s" % (
                    user_ptr, length, "Tagged" if is_tagged else "No", "mmap"
                ))
                total_allocations += 1
                total_bytes += length

            rec_cur = rec['next']

        print("-" * 75)
        print("Total active allocations: %d (%d bytes)" % (total_allocations, total_bytes))

ListAllocations()
