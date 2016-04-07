/*
 * Copyright (C) 2006 - 2008 Tresys Technology, LLC
 * Developed Under US JFCOM Sponsorship
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

package com.tresys.sipc;

import com.tresys.sipc.sipcwrapperConstants.*;

/**
 * SYSV message queue backend implementation.
 *
 * @author David Windsor <dwindsor@tresys.com>
 * @author Norman Patrick <npatrick@tresys.com>
 */
public class SipcMqueue extends Sipc {

	/**
	 * Create an IPC handle representing a SYSV message
	 * queue backed communications channel.
	 *
	 * @param key  Pathname of IPC key, as understood by ftok(1).
	 * @param role One of Sipc.CREATOR, Sipc.SENDER, or Sipc.RECEIVER.
	 * @param ipcLength  Size in bytes of the system's IPC data buffer.
	 *                   This should not exceed any known system limits.
	 */
	public SipcMqueue(String key, int role, long ipcLength) throws Exception {
		super(key, role, sipcwrapperConstants.SIPC_SYSV_MQUEUES, ipcLength);
	}

	/**
	 * Destroys the IPC resources associated with the message
	 * queue handle.  Existing handles can continue operating
	 * until they call Close().
	 */
	public static void Unlink(String key) {
		sipcwrapper.sipc_unlink(key, sipcwrapperConstants.SIPC_SYSV_MQUEUES);
	}
}
