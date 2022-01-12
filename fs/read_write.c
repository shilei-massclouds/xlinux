// SPDX-License-Identifier: GPL-2.0

#include <fs.h>
#include <errno.h>
#include <export.h>
#include <uaccess.h>
#include <thread_info.h>

static ssize_t
new_sync_read(struct file *filp, char *buf, size_t len, loff_t *ppos)
{
    ssize_t ret;
    struct kiocb kiocb;
    struct iov_iter iter;
    struct iovec iov = { .iov_base = buf, .iov_len = len };
    init_sync_kiocb(&kiocb, filp);
    kiocb.ki_pos = (ppos ? *ppos : 0);
    iov_iter_init(&iter, READ, &iov, 1, len);

    ret = call_read_iter(filp, &kiocb, &iter);
    BUG_ON(ret == -EIOCBQUEUED);
    if (ppos)
        *ppos = kiocb.ki_pos;
    panic("%s: !", __func__);
    return ret;
}

ssize_t
__kernel_read(struct file *file, void *buf, size_t count, loff_t *pos)
{
    ssize_t ret;
    mm_segment_t old_fs = get_fs();

    BUG_ON(!(file->f_mode & FMODE_READ));
    if (!(file->f_mode & FMODE_CAN_READ))
        panic("no FMODE_CAN_READ!");

    if (count > MAX_RW_COUNT)
        count = MAX_RW_COUNT;
    set_fs(KERNEL_DS);
    if (file->f_op->read)
        ret = file->f_op->read(file, (void *)buf, count, pos);
    else if (file->f_op->read_iter)
        ret = new_sync_read(file, (void *)buf, count, pos);
    else
        panic("bad read op!");
    /*
    set_fs(old_fs);
    if (ret > 0) {
        fsnotify_access(file);
        add_rchar(current, ret);
    }
    inc_syscr(current);
    return ret;
    */
    panic("%s: !", __func__);
}

ssize_t
kernel_read(struct file *file, void *buf, size_t count, loff_t *pos)
{
    return __kernel_read(file, buf, count, pos);
}
EXPORT_SYMBOL(kernel_read);
