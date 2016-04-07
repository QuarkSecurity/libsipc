/*
 * SHM_Sender.java
 * Send character data to a shared memory backed communications channel.
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

import java.io.*;
import com.tresys.sipc.*;

import java.nio.ByteBuffer;

/**
 * Write data to a SYSV shared memory backed IPC resource.
 * The IPC resource must be created by a privileged creation processs.
 *
 * @author David Windsor <dwindsor@tresys.com>
 * @author Norman Patrick <npatrick@tresys.com>
 */
public class SHM_Sender {
	private static final String sipc_key = "sipc_shm_key"; /* shared IPC key */

	private static final String data_end = "0xDEADBEEF"; /* end of transmission marker */

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		SipcShm shm = null;
		int ipc_len = 8192;
		String filename = args[0];
		FileInputStream stream = null;

		try {
			/* Create and initialize the IPC handle */
			shm = new SipcShm(sipc_key, Sipc.SENDER, ipc_len);

			/* Read data from the file and send it across the channel */
			stream = new FileInputStream(filename);
			SendData(shm, stream);
			stream.close();
			shm.Close();
		} catch (IOException ioe) {
			ioe.printStackTrace();
			System.err.println("Unable to read from file " + filename);
			System.exit(1);
		} catch (Exception e) {
			e.printStackTrace();
			System.err.println("Unable to connect shared memory!");
			System.exit(1);
		}
	}

	private static void SendData(SipcShm shm, FileInputStream stream) throws Exception {
		java.nio.ByteBuffer buffer = shm.GetDataPtr();
		java.nio.channels.FileChannel fc = stream.getChannel();
		int read = fc.read(buffer);
		while (read >= 0) {
			/* Explicit-length version of SendData:
			   Send the number of bytes we read */
			shm.SendData(read);
			buffer.clear();
			read = fc.read(buffer);
		}

		buffer.clear();
		//Send the end of transmission marker
		buffer.put(SHM_Sender.data_end.getBytes());
		buffer.put((byte) 0);
		/* Inferred-length version of SendData:
		   flip the buffer to set its limit, and
		   Sipc will use that as the data length
		*/
		buffer.flip();
		shm.SendData();
	}
}
