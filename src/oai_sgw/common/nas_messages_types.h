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

/*! \file nas_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

#ifndef FILE_NAS_MESSAGES_TYPES_SEEN
#define FILE_NAS_MESSAGES_TYPES_SEEN

#include "nas_message.h"
#include "as_message.h"
#include "networkDef.h"
#include "3gpp_29.274.h"
#include "s1ap_messages_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NAS_UPLINK_DATA_IND(mSGpTR)                 (mSGpTR)->ittiMsg.nas_ul_data_ind
#define NAS_DOWNLINK_DATA_REQ(mSGpTR)               (mSGpTR)->ittiMsg.nas_dl_data_req
#define NAS_DL_DATA_CNF(mSGpTR)                     (mSGpTR)->ittiMsg.nas_dl_data_cnf
#define NAS_DL_DATA_REJ(mSGpTR)                     (mSGpTR)->ittiMsg.nas_dl_data_rej
#define NAS_PDN_CONFIG_REQ(mSGpTR)                  (mSGpTR)->ittiMsg.nas_pdn_config_req
#define NAS_PDN_CONFIG_RSP(mSGpTR)                  (mSGpTR)->ittiMsg.nas_pdn_config_rsp
#define NAS_PDN_CONFIG_FAIL(mSGpTR)                 (mSGpTR)->ittiMsg.nas_pdn_config_fail
#define NAS_PDN_CONNECTIVITY_REQ(mSGpTR)            (mSGpTR)->ittiMsg.nas_pdn_connectivity_req
#define NAS_PDN_CONNECTIVITY_RSP(mSGpTR)            (mSGpTR)->ittiMsg.nas_pdn_connectivity_rsp
#define NAS_PDN_CONNECTIVITY_FAIL(mSGpTR)           (mSGpTR)->ittiMsg.nas_pdn_connectivity_fail
#define NAS_INITIAL_UE_MESSAGE(mSGpTR)              (mSGpTR)->ittiMsg.nas_initial_ue_message
#define NAS_CONNECTION_ESTABLISHMENT_CNF(mSGpTR)    (mSGpTR)->ittiMsg.nas_conn_est_cnf
#define NAS_BEARER_PARAM(mSGpTR)                    (mSGpTR)->ittiMsg.nas_bearer_param
#define NAS_AUTHENTICATION_REQ(mSGpTR)              (mSGpTR)->ittiMsg.nas_auth_req
#define NAS_AUTHENTICATION_PARAM_REQ(mSGpTR)        (mSGpTR)->ittiMsg.nas_auth_param_req
#define NAS_DETACH_REQ(mSGpTR)                      (mSGpTR)->ittiMsg.nas_detach_req
#define NAS_ERAB_SETUP_REQ(mSGpTR)                  (mSGpTR)->ittiMsg.nas_erab_setup_req
#define NAS_ERAB_RELEASE_REQ(mSGpTR)                (mSGpTR)->ittiMsg.nas_erab_release_req
#define NAS_SIGNALLING_CONNECTION_REL_IND(mSGpTR)  (mSGpTR)->ittiMsg.nas_signalling_connection_rel_ind

// todo: context req_res..
// todo:
#define NAS_IMPLICIT_DETACH_UE_IND(mSGpTR)          (mSGpTR)->ittiMsg.nas_implicit_detach_ue_ind

/** Add PDN Disconnect request. */
#define NAS_PDN_DISCONNECT_REQ(mSGpTR)              (mSGpTR)->ittiMsg.nas_pdn_disconnect_req
#define NAS_PDN_DISCONNECT_RSP(mSGpTR)              (mSGpTR)->ittiMsg.nas_pdn_disconnect_rsp

#define NAS_CONTEXT_REQ(mSGpTR)                  (mSGpTR)->ittiMsg.nas_context_req
#define NAS_CONTEXT_RES(mSGpTR)                  (mSGpTR)->ittiMsg.nas_context_res
#define NAS_CONTEXT_FAIL(mSGpTR)                 (mSGpTR)->ittiMsg.nas_context_fail

typedef enum pdn_conn_rsp_cause_e {
  CAUSE_OK = 16,
  CAUSE_CONTEXT_NOT_FOUND = 64,
  CAUSE_INVALID_MESSAGE_FORMAT = 65,
  CAUSE_SERVICE_NOT_SUPPORTED = 68,
  CAUSE_SYSTEM_FAILURE = 72,
  CAUSE_NO_RESOURCES_AVAILABLE = 73,
  CAUSE_ALL_DYNAMIC_ADDRESSES_OCCUPIED = 84
} pdn_conn_rsp_cause_t;

typedef struct itti_nas_pdn_connectivity_req_s {
  proc_tid_t             pti;   // nas ref  Identity of the procedure transaction executed to activate the PDN connection entry
  mme_ue_s1ap_id_t       ue_id; // nas ref
//  char                   imsi[16];
//  uint8_t                imsi_length;
  imsi64_t               imsi;
  imsi_t                 _imsi;
  bearer_qos_t           bearer_qos;
  protocol_configuration_options_t pco;
  bstring                apn;
  ebi_t                  default_ebi;
  pdn_cid_t              pdn_cid;
  bstring                pdn_addr;
  int                    pdn_type;
  int                    request_type;
} itti_nas_pdn_connectivity_req_t;


typedef struct itti_nas_pdn_connectivity_rsp_s {
  pdn_cid_t               pdn_cid;
  proc_tid_t              pti;   // nas ref  Identity of the procedure transaction executed to activate the PDN connection entry
  network_qos_t           qos;
  protocol_configuration_options_t pco;
  bstring                 pdn_addr;
  int                     pdn_type;
  int                     request_type;

  mme_ue_s1ap_id_t        ue_id;

  /* Key eNB */
  //uint8_t                 kenb[32];

  ambr_t                  ambr;
  ambr_t                  apn_ambr;

  /* EPS bearer ID */
  unsigned                ebi:4;

  /* QoS */
  qci_t                   qci;
  priority_level_t        prio_level;
  pre_emption_vulnerability_t pre_emp_vulnerability;
  pre_emption_capability_t    pre_emp_capability;

  /* S-GW TEID for user-plane */
  /* S-GW IP address for User-Plane */
  fteid_t                  sgw_s1u_fteid;
} itti_nas_pdn_connectivity_rsp_t;

typedef struct itti_nas_pdn_connectivity_fail_s {
  mme_ue_s1ap_id_t        ue_id;
  int                     pti;
  pdn_conn_rsp_cause_t    cause;
} itti_nas_pdn_connectivity_fail_t;

typedef struct itti_nas_pdn_config_req_s {
  mme_ue_s1ap_id_t       ue_id; // nas ref
  char                   imsi[16];
  uint8_t                imsi_length;
  bstring                apn;
  bstring                pdn_addr;
  pdn_type_t             pdn_type;
  int                    request_type;
} itti_nas_pdn_config_req_t;

typedef struct itti_nas_pdn_config_rsp_s {
  mme_ue_s1ap_id_t        ue_id; // nas ref
  imsi64_t                imsi64;
} itti_nas_pdn_config_rsp_t;

typedef struct itti_nas_pdn_config_fail_s {
  mme_ue_s1ap_id_t        ue_id; // nas ref
} itti_nas_pdn_config_fail_t;


typedef struct itti_nas_initial_ue_message_s {
  nas_establish_ind_t nas;

  /* Transparent message from s1ap to be forwarded to MME_APP or
   * to S1AP if connection establishment is rejected by NAS.
   */
  s1ap_initial_ue_message_t transparent;
} itti_nas_initial_ue_message_t;


typedef struct itti_nas_conn_est_rej_s {
  mme_ue_s1ap_id_t ue_id;         /* UE lower layer identifier   */
  s_tmsi_t         s_tmsi;        /* UE identity                 */
  nas_error_code_t err_code;      /* Transaction status          */
  bstring          nas_msg;       /* NAS message to transfer     */
  uint32_t         nas_ul_count;  /* UL NAS COUNT                */
  uint16_t         selected_encryption_algorithm;
  uint16_t         selected_integrity_algorithm;
} itti_nas_conn_est_rej_t;


typedef struct itti_nas_conn_est_cnf_s {
  mme_ue_s1ap_id_t        ue_id;            /* UE lower layer identifier   */
  nas_error_code_t        err_code;         /* Transaction status          */
  bstring                 nas_msg;          /* NAS message to transfer     */

  uint8_t                 kenb[32];


  uint32_t                ul_nas_count;
  uint16_t                encryption_algorithm_capabilities;
  uint16_t                integrity_algorithm_capabilities;
} itti_nas_conn_est_cnf_t;

typedef struct itti_nas_conn_rel_ind_s {

} itti_nas_conn_rel_ind_t;


typedef struct itti_nas_info_transfer_s {
  mme_ue_s1ap_id_t  ue_id;          /* UE lower layer identifier        */
  //nas_error_code_t err_code;     /* Transaction status               */
  bstring           nas_msg;        /* Uplink NAS message           */
} itti_nas_info_transfer_t;


typedef struct itti_nas_ul_data_ind_s {
  mme_ue_s1ap_id_t  ue_id;          /* UE lower layer identifier        */
  bstring           nas_msg;        /* Uplink NAS message           */
  tai_t             tai;            /* Indicating the Tracking Area from which the UE has sent the NAS message.  */
  ecgi_t            cgi;            /* Indicating the cell from which the UE has sent the NAS message.   */
} itti_nas_ul_data_ind_t;


typedef struct itti_nas_dl_data_req_s {
  enb_ue_s1ap_id_t  enb_ue_s1ap_id; /* UE lower layer identifier        */
  mme_ue_s1ap_id_t  ue_id;          /* UE lower layer identifier        */
  //nas_error_code_t err_code;      /* Transaction status               */
  nas_error_code_t  transaction_status;  /* Transaction status               */
  uint32_t          enb_id;

  bstring           nas_msg;        /* Uplink NAS message           */
} itti_nas_dl_data_req_t;

typedef struct itti_nas_dl_data_cnf_s {
  mme_ue_s1ap_id_t ue_id;      /* UE lower layer identifier        */
  nas_error_code_t err_code;   /* Transaction status               */
} itti_nas_dl_data_cnf_t;

typedef struct itti_nas_dl_data_rej_s {
  mme_ue_s1ap_id_t ue_id;            /* UE lower layer identifier   */
  bstring          nas_msg;          /* Uplink NAS message           */
  int              err_code;
} itti_nas_dl_data_rej_t;

typedef struct itti_nas_erab_setup_req_s {
  mme_ue_s1ap_id_t ue_id;            /* UE lower layer identifier   */
  ebi_t            ebi;              /* EPS bearer id        */
  bstring          nas_msg;          /* NAS erab bearer context activation message           */
  bitrate_t        mbr_dl;
  bitrate_t        mbr_ul;
  bitrate_t        gbr_dl;
  bitrate_t        gbr_ul;
} itti_nas_erab_setup_req_t;

typedef struct itti_nas_erab_release_req_s {
  mme_ue_s1ap_id_t ue_id;            /* UE lower layer identifier   */
  ebi_t            ebi;              /* EPS bearer id        */
  bstring          nas_msg;          /* NAS erab bearer context deactivation message           */
} itti_nas_erab_release_req_t;


typedef struct itti_nas_attach_req_s {
  /* TODO: Set the correct size */
  char apn[100];
  char imsi[16];
#define INITIAL_REQUEST (0x1)
  unsigned initial:1;
  s1ap_initial_ue_message_t transparent;
} itti_nas_attach_req_t;


typedef struct itti_nas_auth_req_s {
  /* UE imsi */
  char imsi[16];

#define NAS_FAILURE_OK  0x0
#define NAS_FAILURE_IND 0x1
  unsigned failure:1;
  int cause;
} itti_nas_auth_req_t;


typedef struct itti_nas_auth_rsp_s {
  char imsi[16];
} itti_nas_auth_rsp_t;

typedef struct itti_nas_auth_param_req_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;

  /* Imsi of the UE (In case of initial request) */
  char     imsi[16];
  uint8_t  imsi_length;

  /* Indicates whether the procedure corresponds to a new connection or not */
  uint8_t  initial_req:1;

  uint8_t  re_synchronization:1;
  uint8_t  auts[14];
  uint8_t  num_vectors;
} itti_nas_auth_param_req_t;

typedef struct itti_nas_detach_req_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
} itti_nas_detach_req_t;

typedef struct itti_nas_signalling_connection_rel_ind_s {
  /* UE identifier */
  mme_ue_s1ap_id_t                  ue_id;
} itti_nas_signalling_connection_rel_ind_t;

typedef struct itti_nas_implicit_detach_ue_ind_s {
  /* UE identifier */
  mme_ue_s1ap_id_t ue_id;
  uint8_t emm_cause;
  uint8_t detach_type;
} itti_nas_implicit_detach_ue_ind_t;

/** NAS Context request and response. */
typedef struct itti_nas_context_req_s {
  mme_ue_s1ap_id_t        ue_id;
  guti_t                  old_guti;
  rat_type_t              rat_type;
  tai_t                   originating_tai;
  bstring                 nas_msg;
  plmn_t                  visited_plmn;
} itti_nas_context_req_t;

typedef struct itti_nas_context_res_s {
  mme_ue_s1ap_id_t        ue_id;
  uint64_t                imsi;
  imsi_t                  _imsi;
  imei_t                  _imei;
  uint8_t                 n_pdns;
  uint8_t                 n_bearers;
  bool                    is_emergency;
} itti_nas_context_res_t;

typedef struct itti_nas_context_fail_s {
  mme_ue_s1ap_id_t        ue_id;
  gtpv2c_cause_value_t    cause;
//  uint64_t                imsi;
} itti_nas_context_fail_t;

typedef struct itti_nas_pdn_disconnect_req_s {
  mme_ue_s1ap_id_t        ue_id;
  pti_t                   pti;
  pdn_cid_t               pdn_cid;
  ebi_t                   default_ebi;
  struct in_addr          saegw_s11_ip_addr;
  teid_t                  saegw_s11_teid;
  bool                    noDelete;
  bstring                 apn;
} itti_nas_pdn_disconnect_req_t;

typedef struct itti_nas_pdn_disconnect_rsp_s {
  mme_ue_s1ap_id_t        ue_id;
  int                     cause;
//  unsigned int            pdn_ctx_id;
} itti_nas_pdn_disconnect_rsp_t;
#ifdef __cplusplus
}
#endif
#endif /* FILE_NAS_MESSAGES_TYPES_SEEN */
