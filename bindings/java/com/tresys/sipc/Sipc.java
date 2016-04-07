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
import com.tresys.sipc.libsipc_t;
import com.tresys.sipc.sipcwrapper;

/**
 * Abstract representation of a secure IPC resource.
 *
 * @author David Windsor <dwindsor@tresys.com>
 * @author Norman Patrick <npatrick@tresys.com>
 * @author L. Ross Raszewski <lraszewski@tresys.com>
 */
public abstract class Sipc {
	public final static int CREATOR = sipcwrapperConstants.SIPC_CREATOR;
	public final static int SENDER =  sipcwrapperConstants.SIPC_SENDER;
	public final static int RECEIVER =  sipcwrapperConstants.SIPC_RECEIVER;
	public final static int BLOCK = sipcwrapperConstants.SIPC_BLOCK;
	public final static int NOBLOCK = sipcwrapperConstants.SIPC_NOBLOCK;

	protected libsipc_t handle; /* internal handle */
	protected boolean isConnected = false; /* internal state flag */
	private java.nio.ByteBuffer dataPtr = null; /* shared buffer for sending data */

	/** shared IPC key */
	private final String key;

	/** the type of IPC, either SIPC_SYSV_SHM or SIPC_SYSV_MQUEUES */
	private int ipc_type;

	/**
	 * Clean up a Sipc handle's resources.
	 */
	protected void finalize() {
		this.Close();
	}

	/**
	 * Create a Sipc handle with specified key, role, and system
	 * IPC buffer size.
	 *
	 * @param key  Path name representing an IPC key.
	 * @param role One of Sipc.CREATOR, Sipc.SENDER, or Sipc.RECEIVER.
	 * @param ipc_type One of SIPC_SYSV_SHM or SIPC_SYSV_MQUEUES.
	 * @param ipcLength  Size in bytes of the system's IPC data buffer.
	 *                   This should not exceed any known system limits.
	 */
	protected Sipc(String key, int role, int ipc_type, long ipcLength) throws Exception {
		if (role != Sipc.CREATOR && role != Sipc.SENDER && role != Sipc.RECEIVER) {
			throw new Error("Role must be one of Sipc.CREATOR, Sipc.SENDER, or Sipc.RECEIVER");
		}
		if (ipc_type != sipcwrapperConstants.SIPC_SYSV_MQUEUES &&
		    ipc_type != sipcwrapperConstants.SIPC_SYSV_SHM) {
			throw new Error("IPC type must be one of sipcwrapperConstants.SIPC_SYSV_MQUEUES or sipcwrapperConstants.SIPC_SYSV_SHM");
		}
		handle = new libsipc_t(key, role, ipc_type, ipcLength);
		this.key = key;
		this.ipc_type = ipc_type;
		isConnected = true;
	}

	/**
	 * Close the IPC handle, if it is still open.  Then destroy
	 * the system IPC resource.
	 */
	public synchronized void DestroyHandle() {
		this.Close();
		sipcwrapper.sipc_unlink(key, ipc_type);
	}

	/**
	 * Determine whether an IPC handle is in a connected state.
	 *
	 * @return  true if the IPC handle is connected to its backend,
	 *			false otherwise
	 */
	public synchronized boolean IsConnected() {
		return this.isConnected;
	}

	/**
	 * Disconnect an IPC handle from its backend. This function has no
	 * effect if the handle is already in a disconnected state.
	 */
	public synchronized void Close() {
		if (isConnected) {
			handle.close();
			handle = null;
			dataPtr = null;
			isConnected = false;
		}
	}

	/**
	 * Signal the sending application that a transmission has been
	 * successfully received and handled.  This operation is not
	 * implemented by all backends.
	 */
	public synchronized void RecvDone() throws Exception {}

	/**
	 * Receive data from a communications channel.  The IPC handle must
	 * be in a connected state for this operation to succeed.
	 *
	 * @return A reference to the internal data pointer holding
	 * the received data, or NULL if the channel is set to
	 * non-blocking read and there was nothing available to be
	 * read.
	 */
	public synchronized byte[] ReadData() throws Exception {
		if (isConnected) {
			return handle.recv_data();
		}
		else {
			throw new ConnectionException("Handle was closed.");
		}
	}

	/**
	 * Get a reference to the IPC handle's internal data buffer.
	 * This buffer's contents will sent data to an IPC channel
	 * upon calls to SendData().  The IPC handle must be in a
	 * connected state for this to succeed.
	 *
	 * @return  A reference to the IPC handle's internal data buffer.
	 */
	public synchronized java.nio.ByteBuffer GetDataPtr() throws Exception {
		if (isConnected) {
			/* cache the dataPtr, to avoid re-creating it for
			 * each call -- this pointer is supposed to
			 * persistent across all calls, and there is
			 * supposed to be exactly one that is ever
			 * created */
			if (dataPtr == null) {
				dataPtr = handle.get_data_ptr();
			}
			return dataPtr;
		}
		else {
			throw new ConnectionException("Handle was closed.");
		}
	}

	/**
	 * Send data to a communications channel.  The data came from
	 * the most recent call to GetDataPtr().  The IPC handle must
	 * be in a connected state for this to succeed.
	 *
	 * @param len Length of the message to be sent.
	 */
	public synchronized void SendData(int len) throws Exception {
		if (isConnected) {
			java.nio.ByteBuffer b = GetDataPtr();
			if (len > b.limit()) {
				throw new IndexOutOfBoundsException("Len set to " + len + ", but buffer's limit was " + b.limit());
			}
			handle.send_data(len);
		}
		else {
			throw new ConnectionException("Handle was closed.");
		}
	}

	/**
	 * Send data to a communications channel.  The data came from
	 * the most recent call to GetDataPtr(); the entire buffer's
	 * contents will be sent.  The IPC handle must be in a
	 * connected state for this to succeed.
	 */
	public synchronized void SendData() throws Exception {
		if (isConnected) {
			handle.send_data(dataPtr.limit());
		}
		else {
			throw new ConnectionException("Handle was closed.");
		}
	}

	/**
	 * Modify the behavior of an open communications channel.
	 *
	 * @param request If Sipc.BLOCK, set the channel to block when
	 * reading.  (This is the default).  If Sipc.NONBLOCK, then
	 * set the channel to not block when reading.
	 */
	public synchronized void IOCtl(int request) throws Exception {
		if (isConnected) {
			handle.ioctl(request);
		}
		else {
			throw new ConnectionException("Handle was closed.");
		}
	}
}
