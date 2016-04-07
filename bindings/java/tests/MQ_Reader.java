/*
 * MQ_Reader.java
 * Read character data from a message queue backed communications channel.
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

import java.nio.*;

/**
 * Read data from a SYSV message queue backed IPC resource.
 * The IPC resource must be created by a privileged creation processs.
 *
 * @author David Windsor <dwindsor@tresys.com>
 * @author Norman Patrick <npatrick@tresys.com>
 * @author L. Ross Raszewski <lraszewski@tresys.com>
 */
public class MQ_Reader {
	private static final String sipc_key = "sipc_mq_key";	/* shared IPC key */

	private static final String data_end = "0xDEADBEEF";	/* end of transmission marker */

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		SipcMqueue queue = null;
		int ipc_len = 8196;

		/* Create and initialize the IPC handle */
		try {
			queue = new SipcMqueue(sipc_key, Sipc.RECEIVER, ipc_len);
		}
		catch (Exception e) {
			e.printStackTrace();
			System.err.println("Unable to connect message queue!");
			System.exit(1);
		}

		try {
			java.io.FileOutputStream fileOut = new java.io.FileOutputStream(
				"out.dat");
			/* Read data from the queue */
			byte[] bbuf;
			while (true) {
				bbuf = queue.ReadData();
				/* Check for an end of transmission marker */
				if (bbuf.length == 11) {
					String data = new String(bbuf);
					if(data.startsWith(data_end)
						&& ((int) data.charAt(10)) == 0)
						break;
				}
				fileOut.write(bbuf);
			}

			/* Try a non-blocking read */
			queue.IOCtl(Sipc.NOBLOCK);
			bbuf = queue.ReadData();
			if (bbuf != null) {
				throw new Exception("Non-blocking read returned something.");
			}
			fileOut.close();
		}
		catch (Exception e) {
			e.printStackTrace();
			System.exit(1);
		}
	}
}
