/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * lib/lib-md.c
 * Memory Descriptor management routines
 *
 *  Copyright (c) 2001-2003 Cluster File Systems, Inc.
 *
 *   This file is part of Lustre, http://www.lustre.org
 *
 *   Lustre is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   Lustre is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Lustre; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define DEBUG_SUBSYSTEM S_LNET

#include <lnet/lib-lnet.h>

/* must be called with LNET_LOCK held */
void
lnet_md_unlink(lnet_libmd_t *md)
{
        if ((md->md_flags & LNET_MD_FLAG_ZOMBIE) == 0) {
                /* first unlink attempt... */
                lnet_me_t *me = md->md_me;

                md->md_flags |= LNET_MD_FLAG_ZOMBIE;

                /* Disassociate from ME (if any), and unlink it if it was created
                 * with LNET_UNLINK */
                if (me != NULL) {
                        md->md_me = NULL;
                        me->me_md = NULL;
                        if (me->me_unlink == LNET_UNLINK)
                                lnet_me_unlink(me);
                }

                /* ensure all future handle lookups fail */
                lnet_invalidate_handle(&md->md_lh);
        }

        if (md->md_refcount != 0) {
                CDEBUG(D_NET, "Queueing unlink of md %p\n", md);
                return;
        }

        CDEBUG(D_NET, "Unlinking md %p\n", md);

        if (md->md_eq != NULL) {
                md->md_eq->eq_refcount--;
                LASSERT (md->md_eq->eq_refcount >= 0);
        }

        list_del (&md->md_list);
        lnet_md_free(md);
}

/* must be called with LNET_LOCK held */
static int
lib_md_build(lnet_libmd_t *lmd, lnet_md_t *umd, int unlink)
{
        lnet_eq_t   *eq = NULL;
        int          i;
        unsigned int niov;
        int          total_length = 0;

        /* NB we are passed an allocated, but uninitialised/active md.
         * if we return success, caller may lnet_md_unlink() it.
         * otherwise caller may only lnet_md_free() it.
         */

        if (!LNetHandleIsEqual (umd->eq_handle, LNET_EQ_NONE)) {
                eq = lnet_handle2eq(&umd->eq_handle);
                if (eq == NULL)
                        return -ENOENT;
        }

        /* This implementation doesn't know how to create START events or
         * disable END events.  Best to LASSERT our caller is compliant so
         * we find out quickly...  */
        /*  TODO - reevaluate what should be here in light of 
         * the removal of the start and end events
         * maybe there we shouldn't even allow LNET_EQ_NONE!)
        LASSERT (eq == NULL);
         */

        lmd->md_me = NULL;
        lmd->md_start = umd->start;
        lmd->md_offset = 0;
        lmd->md_max_size = umd->max_size;
        lmd->md_options = umd->options;
        lmd->md_user_ptr = umd->user_ptr;
        lmd->md_eq = eq;
        lmd->md_threshold = umd->threshold;
        lmd->md_refcount = 0;
        lmd->md_flags = (unlink == LNET_UNLINK) ? LNET_MD_FLAG_AUTO_UNLINK : 0;

        if ((umd->options & LNET_MD_IOVEC) != 0) {

                if ((umd->options & LNET_MD_KIOV) != 0) /* Can't specify both */
                        return -EINVAL;

                lmd->md_niov = niov = umd->length;
                memcpy(lmd->md_iov.iov, umd->start,
                       niov * sizeof (lmd->md_iov.iov[0]));

                for (i = 0; i < niov; i++) {
                        /* We take the base address on trust */
                        if (lmd->md_iov.iov[i].iov_len <= 0) /* invalid length */
                                return -EINVAL;

                        total_length += lmd->md_iov.iov[i].iov_len;
                }

                lmd->md_length = total_length;

                if ((umd->options & LNET_MD_MAX_SIZE) != 0 && /* max size used */
                    (umd->max_size < 0 ||
                     umd->max_size > total_length)) // illegal max_size
                        return -EINVAL;

        } else if ((umd->options & LNET_MD_KIOV) != 0) {
#ifndef __KERNEL__
                return -EINVAL;
#else
                lmd->md_niov = niov = umd->length;
                memcpy(lmd->md_iov.kiov, umd->start,
                       niov * sizeof (lmd->md_iov.kiov[0]));

                for (i = 0; i < niov; i++) {
                        /* We take the page pointer on trust */
                        if (lmd->md_iov.kiov[i].kiov_offset +
                            lmd->md_iov.kiov[i].kiov_len > CFS_PAGE_SIZE )
                                return -EINVAL; /* invalid length */

                        total_length += lmd->md_iov.kiov[i].kiov_len;
                }

                lmd->md_length = total_length;

                if ((umd->options & LNET_MD_MAX_SIZE) != 0 && /* max size used */
                    (umd->max_size < 0 ||
                     umd->max_size > total_length)) // illegal max_size
                        return -EINVAL;
#endif
        } else {   /* contiguous */
                lmd->md_length = umd->length;
                lmd->md_niov = niov = 1;
                lmd->md_iov.iov[0].iov_base = umd->start;
                lmd->md_iov.iov[0].iov_len = umd->length;

                if ((umd->options & LNET_MD_MAX_SIZE) != 0 && /* max size used */
                    (umd->max_size < 0 ||
                     umd->max_size > umd->length)) // illegal max_size
                        return -EINVAL;
        }

        if (eq != NULL)
                eq->eq_refcount++;

        /* It's good; let handle2md succeed and add to active mds */
        lnet_initialise_handle (&lmd->md_lh, LNET_COOKIE_TYPE_MD);
        list_add (&lmd->md_list, &the_lnet.ln_active_mds);

        return 0;
}

/* must be called with LNET_LOCK held */
void
lnet_md_deconstruct(lnet_libmd_t *lmd, lnet_md_t *umd)
{
        /* NB this doesn't copy out all the iov entries so when a
         * discontiguous MD is copied out, the target gets to know the
         * original iov pointer (in start) and the number of entries it had
         * and that's all.
         */
        umd->start = lmd->md_start;
        umd->length = ((lmd->md_options & (LNET_MD_IOVEC | LNET_MD_KIOV)) == 0) ?
                      lmd->md_length : lmd->md_niov;
        umd->threshold = lmd->md_threshold;
        umd->max_size = lmd->md_max_size;
        umd->options = lmd->md_options;
        umd->user_ptr = lmd->md_user_ptr;
        lnet_eq2handle(&umd->eq_handle, lmd->md_eq);
}

int
LNetMDAttach(lnet_handle_me_t meh, lnet_md_t umd,
             lnet_unlink_t unlink, lnet_handle_md_t *handle)
{
        lnet_me_t     *me;
        lnet_libmd_t  *md;
        int            rc;

        LASSERT (the_lnet.ln_init);
        LASSERT (the_lnet.ln_refcount > 0);
        
        if ((umd.options & (LNET_MD_KIOV | LNET_MD_IOVEC)) != 0 &&
            umd.length > LNET_MAX_IOV) /* too many fragments */
                return -EINVAL;

        md = lnet_md_alloc(&umd);
        if (md == NULL)
                return -ENOMEM;

        LNET_LOCK();

        me = lnet_handle2me(&meh);
        if (me == NULL) {
                rc = -ENOENT;
        } else if (me->me_md != NULL) {
                rc = -EBUSY;
        } else {
                rc = lib_md_build(md, &umd, unlink);
                if (rc == 0) {
                        me->me_md = md;
                        md->md_me = me;

                        lnet_md2handle(handle, md);

                        /* check if this MD matches any blocked msgs */
                        lnet_match_blocked_msg(md);   /* expects LNET_LOCK held */

                        LNET_UNLOCK();
                        return (0);
                }
        }

        lnet_md_free (md);

        LNET_UNLOCK();
        return (rc);
}

int
LNetMDBind(lnet_md_t umd, lnet_unlink_t unlink, lnet_handle_md_t *handle)
{
        lnet_libmd_t  *md;
        int            rc;

        LASSERT (the_lnet.ln_init);
        LASSERT (the_lnet.ln_refcount > 0);
        
        if ((umd.options & (LNET_MD_KIOV | LNET_MD_IOVEC)) != 0 &&
            umd.length > LNET_MAX_IOV) /* too many fragments */
                return -EINVAL;

        md = lnet_md_alloc(&umd);
        if (md == NULL)
                return -ENOMEM;

        LNET_LOCK();

        rc = lib_md_build(md, &umd, unlink);

        if (rc == 0) {
                lnet_md2handle(handle, md);

                LNET_UNLOCK();
                return (0);
        }

        lnet_md_free (md);

        LNET_UNLOCK();
        return (rc);
}

int
LNetMDUnlink (lnet_handle_md_t mdh)
{
        lnet_event_t     ev;
        lnet_libmd_t    *md;

        LASSERT (the_lnet.ln_init);
        LASSERT (the_lnet.ln_refcount > 0);
        
        LNET_LOCK();

        md = lnet_handle2md(&mdh);
        if (md == NULL) {
                LNET_UNLOCK();
                return -ENOENT;
        }

        /* If the MD is busy, lnet_md_unlink just marks it for deletion, and
         * when the NAL is done, the completion event flags that the MD was
         * unlinked.  Otherwise, we enqueue an event now... */

        if (md->md_eq != NULL &&
            md->md_refcount == 0) {
                memset(&ev, 0, sizeof(ev));

                ev.type = LNET_EVENT_UNLINK;
                ev.status = 0;
                ev.unlinked = 1;
                lnet_md_deconstruct(md, &ev.md);
                lnet_md2handle(&ev.md_handle, md);

                lnet_enq_event_locked(md->md_eq, &ev);
        }

        lnet_md_unlink(md);

        LNET_UNLOCK();
        return 0;
}

