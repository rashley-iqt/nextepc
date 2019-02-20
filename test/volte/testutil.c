/*************************************************************************** 

    Copyright (C) 2019 NextEPC Inc. All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***************************************************************************/


/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core_general.h"
#include "core_debug.h"
#include "core_semaphore.h"
#include "core_file.h"
#include "fd/fd_lib.h"

#include "common/application.h"
#include "common/context.h"

#include "testpacket.h"
#include "testapp.h"

#include "abts.h"
#include "testutil.h"

#include "pcscf_fd_path.h"

abts_suite *test_volte(abts_suite *suite);

const struct testlist alltests[] = {
    {test_volte},
    {NULL},
};

static int connected_count = 0;
static void test_fd_logger_handler(enum fd_hook_type type, struct msg * msg, 
    struct peer_hdr * peer, void * other, struct fd_hook_permsgdata *pmd, 
    void * regdata)
{
    if (type == HOOK_PEER_CONNECT_SUCCESS)
    {
        connected_count++;
    }
}

void test_terminate(void)
{
    pcscf_fd_final();
    testpacket_final();
    test_app_terminate();

    d_trace_global_on();
    core_terminate();
}

status_t test_initialize(int argc, const char *const argv[], char *config_path)
{
    status_t rv;
    char dir[C_PATH_MAX/2];
    char conf[C_PATH_MAX];

    if (config_path)
    {
        strcpy(conf, config_path);
    }
    else
    {
        path_remove_last_component(dir, argv[0]);
        if (strstr(dir, ".libs"))
            path_remove_last_component(dir, dir);
        sprintf(conf, "%s/sample-volte.conf", dir);
    }

    fd_logger_register(test_fd_logger_handler);

    atexit(test_terminate);

    core_initialize();
    rv = test_app_initialize(conf, NULL, NULL);
    if (rv != CORE_OK)
    {
        d_error("test_app_initialize() failed");
        return CORE_ERROR;
    }
    rv = pcscf_fd_init();
    if (rv != CORE_OK)
    {
        d_error("pcscf_fd_init() failed");
        return CORE_ERROR;
    }
    rv = testpacket_init();
    if (rv != CORE_OK)
    {
        d_error("testpacket() failed");
        return CORE_ERROR;
    }

    while(1)
    {
        if (connected_count == 1) break;
        core_sleep(time_from_msec(50));
    }

    return rv;
}

