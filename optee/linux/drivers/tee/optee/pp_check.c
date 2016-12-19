#include "pp_check.h"


static int boomerang_pin_user_pages(void *buffer, size_t size,
		unsigned long *pages_ptr, bool writable, bool *is_locked)
{
	int ret = 0;
	unsigned int nr_pages;
	struct page **pages = NULL;
	struct vm_area_struct *vma = NULL;
	unsigned int flags;
	unsigned long curr_buff_addr;
	int i;
	int index;
	size_t page_mask;
	bool is_locked_prev;

	page_mask = ~(PAGE_SIZE - 1);
	
	nr_pages = ((((long)buffer + size) & page_mask) - ((long) buffer & page_mask)) >> PAGE_SHIFT;
	nr_pages++;

	pages = kzalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	down_read(&current->mm->mmap_sem);
	ret = get_user_pages(current, current->mm, (unsigned long)buffer,
			nr_pages, writable,
			0, pages, NULL);

	up_read(&current->mm->mmap_sem);

	if (ret <= 0) {
		pr_err("%s: Error %d in get_user_pages for:%p for %d pages\n", __func__, ret, buffer, nr_pages);
		return ret;
	}
	

	*pages_ptr = (unsigned long) pages;
	nr_pages = ret;
	
	down_read(&current->mm->mmap_sem);

	is_locked_prev = false;
	vma = find_extend_vma(current->mm, (unsigned long)buffer);
	if(vma) {
		flush_cache_range(vma, buffer, buffer+size+1);
	}
	if (vma && (vma->vm_flags & VM_LOCKED))
		is_locked_prev = true;

	up_read(&current->mm->mmap_sem);

	/*
	 * Lock the pages if they are not already locked to ensure that
	 * AF bit is not set to zero.
	 */
	*is_locked = false;
	if (!is_locked_prev) {
		ret = sys_mlock((unsigned long)buffer, size);
		if (!ret)
			*is_locked = true;
		else
			/*
			 * Follow through even if mlock failed as it can be
			 * failed due to memory restrictions or invalid
			 * capabilities
			 */
			pr_warn("%s: Error %d in mlock, continuing session\n",
								__func__, ret);
	}

	down_read(&current->mm->mmap_sem);

	/* Fault pages to set the AF bit in PTE */
	flags = FAULT_FLAG_USER;
	if (writable)
		flags |= FAULT_FLAG_WRITE;
	for (i = 0; i < nr_pages; i++) {
		ret = fixup_user_fault(current, current->mm,
			(unsigned long)(buffer + (i * PAGE_SIZE)), flags, NULL);
		if (ret) {
			pr_err("%s: Error %d in fixup_user_fault\n",
							__func__, ret);
			break;
		}
	}

	up_read(&current->mm->mmap_sem);

	if (ret) {
		if (*is_locked)
			sys_munlock((unsigned long)buffer, size);
		return ret;
	}

	/* Return the number of pages pinned */
	return nr_pages;
}

int boomerang_prep_mem_buffer(void *buffer, size_t size, bool is_write, struct optee_session *session, void **buff_paaddr, void **buff_paaddr1)
{
	unsigned long pages = 0;
	struct boomerang_shmem_desc *shmem_desc = NULL;
	// by default we return success
	int ret = 0, nr_pages = 0;
	size_t i=0;
	bool is_locked = false;

	/* allocate new shmem descriptor */
	shmem_desc = kzalloc(sizeof(struct boomerang_shmem_desc), GFP_KERNEL);
	if (!shmem_desc) {
		pr_err("%s: te_add_shmem_desc failed\n", __func__);
		ret = -1;
		goto error;
	}
	
	//pr_warning("BOOMERANG: Trying to map user pages as %d, Address: %p, size 0x%x\n", is_write, buffer, size);

	/* pin pages */
	nr_pages = boomerang_pin_user_pages(buffer, size, &pages,
					is_write, &is_locked);
	if (nr_pages <= 0) {
		pr_err("%s: te_pin_user_pages failed (%d)\n", __func__,
			nr_pages);
		ret = -1;
		kfree(shmem_desc);
		goto error;
	}
	
	/* initialize shmem descriptor */
	INIT_LIST_HEAD(&(shmem_desc->list));
	shmem_desc->buffer = buffer;
	shmem_desc->size = size;
	shmem_desc->is_write = is_write;
	shmem_desc->nr_pages = nr_pages;
	shmem_desc->pages = (struct page **)(uintptr_t)pages;
	shmem_desc->is_locked = is_locked;
	*buff_paaddr = (void*)((unsigned long)page_to_phys(shmem_desc->pages[0]) + ((uintptr_t)buffer & (PAGE_SIZE -1)));
	if(nr_pages > 1) {
		*buff_paaddr1 = page_to_phys(shmem_desc->pages[1]);
	}
	//Verify that all physical pages are continuous
	if(nr_pages > 2) {
		panic("BOOMERANG: As of now, cannot handle more than 2-pages\n");
	}

	/* add shmem descriptor to proper list */
	list_add_tail(&shmem_desc->list, &session->boomerang_shmem_list);
	
error:
	return ret;
}


static void boomerang_release_mem_buffer(struct boomerang_shmem_desc *shmem_desc)
{
	uint32_t i;
	int status;
	struct vm_area_struct *vma = NULL;

	list_del(&shmem_desc->list);
	for (i = 0; i < shmem_desc->nr_pages; i++) {
		if(shmem_desc->is_write) {
			set_page_dirty_lock(shmem_desc->pages[i]);
		}
		page_cache_release(shmem_desc->pages[i]);
	}
	kfree(shmem_desc->pages);

	if (shmem_desc->is_locked && current->mm) {
		status = sys_munlock((unsigned long)shmem_desc->buffer,
							shmem_desc->size);
		if (status)
			pr_err("%s:Error %d in munlock\n", __func__, status);
	}

	kfree(shmem_desc);
}

void boomerang_release_mem_buffers(struct list_head *buflist)
{
	struct boomerang_shmem_desc *shmem_desc, *tmp_shmem_desc;

	list_for_each_entry_safe(shmem_desc, tmp_shmem_desc, buflist, list) {
		boomerang_release_mem_buffer(shmem_desc);
	}
}
