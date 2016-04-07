/* Author: David Windsor <dwindsor@tresys.com>
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <CUnit/Basic.h>
#include "mqueue.h"
#include "shm.h"

int main(void)
{
	if (CU_initialize_registry() != CUE_SUCCESS)
		return CU_get_error();

	/* Add suites to registry */
	CU_SuiteInfo suites[] = {
		{"mq", test_mqueue_init, test_mqueue_cleanup, mqueue_tests}
		,
		{"shm", test_shm_init, test_shm_cleanup, shm_tests}
		,
		CU_SUITE_INFO_NULL
	};
	CU_register_suites(suites);

	/* Run tests */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

	printf("# suites run: %d, # tests run: %d\n", CU_get_number_of_suites_run(), CU_get_number_of_tests_run());

	CU_cleanup_registry();
	return CU_get_error();
}
