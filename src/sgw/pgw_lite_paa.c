/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under 
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file paa.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bstrlib.h"

#include "queue.h"
#include "dynamic_memory_check.h"
#include "assertions.h"
#include "conversions.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "common_defs.h"
#include "intertask_interface.h"
#include "msc.h"
#include "log.h"
#include "sgw_ie_defs.h"
#include "3gpp_23.401.h"
#include "sgw_defs.h"
#include "spgw_config.h"
#include "sgw.h"
#include "pgw_lite_paa.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define PGW_LITE_FREE_ADDR_POOL_CONFIG 1

extern pgw_app_t                        pgw_app;


// Load in PGW pool, configured PAA address pool
void
pgw_load_pool_ip_addresses (
  void)
{
  struct conf_ipv4_list_elm_s   *conf_ipv4_p = NULL;
  struct ipv4_list_elm_s        *ipv4_p = NULL;
  //struct conf_ipv6_list_elm_s   *conf_ipv6_p = NULL;
  //struct ipv6_list_elm_s        *ipv6_p = NULL;
  //char                           print_buffer[INET6_ADDRSTRLEN];

  STAILQ_INIT (&pgw_app.ipv4_list_free);
  STAILQ_INIT (&pgw_app.ipv4_list_allocated);
  STAILQ_FOREACH (conf_ipv4_p, &spgw_config.pgw_config.ipv4_pool_list, ipv4_entries) {
    ipv4_p = calloc (1, sizeof (struct ipv4_list_elm_s));
    ipv4_p->addr.s_addr = conf_ipv4_p->addr.s_addr;
    STAILQ_INSERT_TAIL (&pgw_app.ipv4_list_free, ipv4_p, ipv4_entries);
    //SPGW_APP_DEBUG("Loaded IPv4 PAA address in pool: %s\n",
    //        inet_ntoa(conf_ipv4_p->addr));
  }

//  STAILQ_FOREACH (conf_ipv6_p, &spgw_config.pgw_config.pgw_lite_ipv6_pool_list, ipv6_entries) {
//    ipv6_p = calloc (1, sizeof (struct pgw_lite_ipv6_list_elm_s));
//    ipv6_p->addr = conf_ipv6_p->addr;
//    ipv6_p->prefix_len = conf_ipv6_p->prefix_len;
//    ipv6_p->num_allocated = 0;
//    STAILQ_INSERT_TAIL (&pgw_app.pgw_lite_ipv6_list_free, ipv6_p, ipv6_entries);
//
//    if (inet_ntop (AF_INET6, &ipv6_p->addr, print_buffer, INET6_ADDRSTRLEN) == NULL) {
//      OAILOG_ERROR (LOG_SPGW_APP, "Could not Load IPv6 PAA address in pool: %s\n", strerror (errno));
//    }                           /*else {
//                                 *
//                                 * SPGW_APP_DEBUG("Loaded IPv6 PAA prefix in pool: %s\n",print_buffer);
//                                 * } */
//  }
#if PGW_LITE_FREE_ADDR_POOL_CONFIG

  while ((conf_ipv4_p = STAILQ_FIRST (&spgw_config.pgw_config.ipv4_pool_list))) {
    STAILQ_REMOVE_HEAD (&spgw_config.pgw_config.ipv4_pool_list, ipv4_entries);
    free_wrapper ((void **) &conf_ipv4_p);
  }

#endif
}



int
pgw_get_free_ipv4_paa_address (
  struct in_addr *const addr_pP)
{
  struct ipv4_list_elm_s        *ipv4_p = NULL;

  if (STAILQ_EMPTY (&pgw_app.ipv4_list_free)) {
    addr_pP->s_addr = INADDR_ANY;
    return RETURNerror;
  }

  ipv4_p = STAILQ_FIRST (&pgw_app.ipv4_list_free);
  STAILQ_REMOVE (&pgw_app.ipv4_list_free, ipv4_p, ipv4_list_elm_s, ipv4_entries);
  STAILQ_INSERT_TAIL (&pgw_app.ipv4_list_allocated, ipv4_p, ipv4_entries);
  addr_pP->s_addr = ipv4_p->addr.s_addr;
  return RETURNok;
}

int
pgw_release_free_ipv4_paa_address (
  const struct in_addr *const addr_pP)
{
  struct ipv4_list_elm_s        *ipv4_p = NULL;

  STAILQ_FOREACH (ipv4_p, &pgw_app.ipv4_list_allocated, ipv4_entries) {
    if (ipv4_p->addr.s_addr == addr_pP->s_addr) {
      STAILQ_REMOVE (&pgw_app.ipv4_list_allocated, ipv4_p, ipv4_list_elm_s, ipv4_entries);
      STAILQ_INSERT_HEAD (&pgw_app.ipv4_list_free, ipv4_p, ipv4_entries);
      return RETURNok;
    }
  }
  return RETURNerror;
}

//int get_assigned_ipv4_block(const int block, struct in_addr * const netaddr, uint32_t * const prefix)
//{
//  int rc = RETURNok;
//  if (RETURNok != (rc = pgw_get_free_ipv4_paa_address(netaddr))) {
//    return rc;
//  }
//   // Only one block supported now (have to process the release in right pool)
//   DevAssert(block == 0);
//   *prefix = spgw_config.pgw_config.ue_pool_mask[block];
//  return rc;
//}


int get_num_paa_ipv4_pool(void)
{
  return spgw_config.pgw_config.num_ue_pool;
}


int get_paa_ipv4_pool(const int block, struct in_addr * const range_low, struct in_addr * const range_high, struct in_addr * const netaddr, struct in_addr * const netmask, const struct ipv4_list_elm_s **out_of_nw)
{
  // Only one block supported now (have to process the release in right pool)
  DevAssert(block < spgw_config.pgw_config.num_ue_pool);
  if (range_low) {
    *range_low = spgw_config.pgw_config.ue_pool_range_low[block];
  }
  if (range_high) {
    *range_high = spgw_config.pgw_config.ue_pool_range_high[block];
  }
  if (netaddr) {
    *netaddr = spgw_config.pgw_config.ue_pool_network[block];
  }
  if (netmask) {
    *netmask = spgw_config.pgw_config.ue_pool_netmask[block];
  }
  if (out_of_nw) {
    *out_of_nw = spgw_config.pgw_config.ue_pool_excluded[block].stqh_first;
  }
  return RETURNok;
}


int get_paa_ipv4_pool_id(const struct in_addr ue_addr)
{
  for (int i = 0; i < spgw_config.pgw_config.num_ue_pool; i++) {
    if ((ntohl(ue_addr.s_addr) >= ntohl(spgw_config.pgw_config.ue_pool_range_low[i].s_addr)) &&
        (ntohl(ue_addr.s_addr) <= ntohl(spgw_config.pgw_config.ue_pool_range_high[i].s_addr))) {
      return i;
    }
  }
  return RETURNerror;
}

#ifdef __cplusplus
}
#endif
