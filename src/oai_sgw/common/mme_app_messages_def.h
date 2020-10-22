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

/*! \file mme_app_messages_def.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
//WARNING: Do not include this header directly. Use intertask_interface.h instead.


MESSAGE_DEF(MME_APP_CONNECTION_ESTABLISHMENT_CNF  , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_INITIAL_CONTEXT_SETUP_RSP     , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_INITIAL_CONTEXT_SETUP_FAILURE , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_DELETE_SESSION_RSP            , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_CREATE_DEDICATED_BEARER_REQ   , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_CREATE_DEDICATED_BEARER_RSP   , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_CREATE_DEDICATED_BEARER_REJ   , MESSAGE_PRIORITY_MED)
MESSAGE_DEF(MME_APP_S1AP_MME_UE_ID_NOTIFICATION	  , MESSAGE_PRIORITY_MED)

