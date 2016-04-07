/*
 * SHM_Destroyer.java
 * Free a shared memory segment.  This is a provileged application.
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

/**
 * Destroyer process for the SIPC shared memory backend.
 *
 * @author David Windsor <dwindsor@tresys.com>
 */
public class SHM_Destroyer {
	private static final String sipc_key = "sipc_shm_key";	/* shared IPC key */

	public static void main(String[] args) {
		SipcShm.Unlink(sipc_key);
	}
}
