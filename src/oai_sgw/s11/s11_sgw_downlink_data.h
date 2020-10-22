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

/*! \file s11_sgw_downlink_data.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_S11_SGW_DOWNLINK_DATA_SEEN
#define FILE_S11_SGW_DOWNLINK_DATA_SEEN

#include "s11_messages_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int
s11_sgw_handle_downlink_data_notification (
  nw_gtpv2c_stack_handle_t * stack_p,
  itti_s11_downlink_data_notification_t * downlink_data_notification_p);

int
s11_sgw_handle_downlink_data_notification_ack (
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi);

int
s11_sgw_handle_downlink_data_notification_failure_ind (
  nw_gtpv2c_stack_handle_t * stack_p,
  nw_gtpv2c_ulp_api_t * pUlpApi);

#ifdef __cplusplus
}
#endif

#endif /* FILE_S11_SGW_DOWNLINK_DATA_SEEN */
