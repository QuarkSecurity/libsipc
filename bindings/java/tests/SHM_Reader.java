/*
 * SHM_Reader.java
 * Read character data from a shared memory backed communications channel.
 *
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

import com.tresys.sipc.*;

import java.nio.ByteBuffer;

/**
 * Read data from a SYSV shared memory backed IPC resource.
 * The IPC resource must be created by a privileged creation processs.
 *
 * @author David Windsor <dwindsor@tresys.com>
 * @author Norman Patrick <npatrick@tresys.com>
 */
public class SHM_Reader {
	private static final String sipc_key = "sipc_shm_key";	/* shared IPC key */

	private static final String data_end = "0xDEADBEEF";	/* end of transmission marker */

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		SipcShm shm = null;
		int ipc_len = 8192;

		/* Create and initialize the IPC handle */
		try {
			shm = new SipcShm(sipc_key, Sipc.RECEIVER, ipc_len);
		} catch (Exception e) {
			e.printStackTrace();
			System.err.println("Unable to connect shared memory!");
			System.exit(1);
		}

		/* Read data from the channel, until the end of transmission
		 * marker is encountered
		 */
		try {
			java.io.FileOutputStream fileOut = new java.io.FileOutputStream(
				"out.dat");
			/* Read data from the shared memory */
			byte[] bbuf;
			while(true) {
				bbuf = shm.ReadData();
				/* Check for the end of transmission marker */
				if (IsEndXmit(bbuf)) {
					shm.RecvDone();
					break;
				}
				fileOut.write(bbuf);
				shm.RecvDone();
			}

			/* Try a non-blocking read */
			shm.IOCtl(Sipc.NOBLOCK);
			bbuf = shm.ReadData();
			if (bbuf != null) {
				throw new Exception("Non-blocking read returned something.");
			}
		}
		catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
		shm.Close();
	}

	private static final boolean IsEndXmit(byte[] bbuf) {
		String data = new String(bbuf);
		if (data.length() >= data_end.length()) {
			String sub = data.substring(0, data_end.length());
			if (sub.equals(data_end))
				return true;
		}

		return false;
	}
}
