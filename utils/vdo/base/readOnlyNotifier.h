/*
 * Copyright (c) 2020 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 *
 * $Id: //eng/vdo-releases/aluminum/src/c++/vdo/base/readOnlyNotifier.h#3 $
 */

/*
 * A ReadOnlyNotifier is responsible for propogating the fact that the VDO
 * has encountered an unrecoverable error to all base threads. It also persists
 * the read-only state to the super block.
 *
 * The notifier also provides the ability to wait for any notifications to be
 * complete in order to not cause super block write races when shutting down
 * the VDO.
 */

#ifndef READ_ONLY_NOTIFIER_H
#define READ_ONLY_NOTIFIER_H

#include "completion.h"

/**
 * A function to notify a listener that the VDO has gone read-only.
 *
 * @param listener  The object to notify
 * @param parent    The completion to notify in order to acknowledge the
 *                  notification
 **/
typedef void ReadOnlyNotification(void *listener, VDOCompletion *parent);

/**
 * Create a read-only notifer.
 *
 * @param [in]  isReadOnly     Whether the VDO is already read-only
 * @param [in]  threadConfig   The thread configuration of the VDO
 * @param [in]  layer          The physical layer of the VDO
 * @param [out] notifierPtr    A pointer to receive the new notifier
 *
 * @return VDO_SUCCESS or an error
 **/
int makeReadOnlyNotifier(bool                 isReadOnly,
                         const ThreadConfig  *threadConfig,
                         PhysicalLayer       *layer,
                         ReadOnlyNotifier   **notifierPtr)
  __attribute__((warn_unused_result));

/**
 * Free a ReadOnlyNotifier and null out the reference to it.
 *
 * @param notifierPtr  The reference to the notifier to free
 **/
void freeReadOnlyNotifier(ReadOnlyNotifier **notifierPtr);

/**
 * Wait until no read-only notifications are in progress and prevent any
 * subsequent notifications. Notifications may be re-enabled by calling
 * allowReadOnlyModeEntry().
 *
 * @param notifier  The read-only notifier on which to wait
 * @param parent    The completion to notify when no threads are entering
 *                  read-only mode
 **/
void waitUntilNotEnteringReadOnlyMode(ReadOnlyNotifier *notifier,
                                      VDOCompletion    *parent);

/**
 * Allow the notifier to put the VDO into read-only mode, reversing the effects
 * of waitUntilNotEnteringReadOnlyMode(). If some thread tried to put the VDO
 * into read-only mode while notifications were disallowed, it will be done
 * when this method is called. If that happens, the parent will not be notified
 * until the VDO has actually entered read-only mode and attempted to save the
 * super block.
 *
 * <p>This method may only be called from the admin thread.
 *
 * @param notifier  The notifier
 * @param parent    The object to notify once the operation is complete
 **/
void allowReadOnlyModeEntry(ReadOnlyNotifier *notifier,
                            VDOCompletion    *parent);

/**
 * Put a VDO into read-only mode and save the read-only state in the super
 * block. This method is a no-op if the VDO is already read-only.
 *
 * @param notifier        The read-only notifier of the VDO
 * @param errorCode       The error which caused the VDO to enter read-only
 *                        mode
 **/
void enterReadOnlyMode(ReadOnlyNotifier *notifier, int errorCode);

/**
 * Check whether the VDO is read-only. This method may be called from any
 * thread, as opposed to examining the VDO's state field which is only safe
 * to check from the admin thread.
 *
 * @param notifier        The read-only notifier of the VDO
 *
 * @return <code>true</code> if the VDO is read-only
 **/
bool isReadOnly(ReadOnlyNotifier *notifier)
  __attribute__((warn_unused_result));

/**
 * Check whether the VDO is or will be read-only (i.e. some thread has started
 * the process of entering read-only mode, but not all threads have been
 * notified yet). This method should only be called in cases where the expense
 * of reading atomic state is not a problem. It was introduced in order to allow
 * suppresion of spurious error messages resulting from VIO cleanup racing with
 * read-only notification.
 *
 * @param notifier  The read-only notifier of the VDO
 *
 * @return <code>true</code> if the VDO has started (and possibly finished)
 *         the process of entering read-only mode
 **/
bool isOrWillBeReadOnly(ReadOnlyNotifier *notifier)
  __attribute__((warn_unused_result));

/**
 * Register a listener to be notified when the VDO goes read-only.
 *
 * @param notifier      The notifier to register with
 * @param listener      The object to notify
 * @param notification  The function to call to send the notification
 * @param threadID      The id of the thread on which to send the notification
 *
 * @return VDO_SUCCESS or an error
 **/
int registerReadOnlyListener(ReadOnlyNotifier     *notifier,
                             void                 *listener,
                             ReadOnlyNotification *notification,
                             ThreadID              threadID);

#endif /* READ_ONLY_NOTIFIER_H */
