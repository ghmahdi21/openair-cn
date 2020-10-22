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
/*! \file s1ap_messages_types.h
  \brief
  \author Sebastien ROUX, Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#ifndef FILE_S1AP_MESSAGES_TYPES_SEEN
#define FILE_S1AP_MESSAGES_TYPES_SEEN

#include "3gpp_36.401.h"
#include "3gpp_36.413.h"
#include "3gpp_36.331.h"
#include "3gpp_23.003.h"
#include "TrackingAreaIdentity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define S1AP_ENB_DEREGISTERED_IND(mSGpTR)        ((itti_s1ap_eNB_deregistered_ind_t*)(mSGpTR)->itti_msg)
#define S1AP_ENB_INITIATED_RESET_REQ(mSGpTR)     ((itti_s1ap_enb_initiated_reset_req_t*)(mSGpTR)->itti_msg)
#define S1AP_ENB_INITIATED_RESET_ACK(mSGpTR)     ((itti_s1ap_enb_initiated_reset_ack_t*)(mSGpTR)->itti_msg)
#define S1AP_DEREGISTER_UE_REQ(mSGpTR)           ((itti_s1ap_deregister_ue_req_t*)(mSGpTR)->itti_msg)
#define S1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR)      ((itti_s1ap_ue_context_release_req_t*)(mSGpTR)->itti_msg)
#define S1AP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR)  ((itti_s1ap_ue_context_release_command_t*)(mSGpTR)->itti_msg)
#define S1AP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) ((itti_s1ap_ue_context_release_complete_t*)(mSGpTR)->itti_msg)
#define S1AP_E_RAB_SETUP_REQ(mSGpTR)             ((itti_s1ap_e_rab_setup_req_t*)(mSGpTR)->itti_msg)
#define S1AP_E_RAB_SETUP_RSP(mSGpTR)             ((itti_s1ap_e_rab_setup_rsp_t*)(mSGpTR)->itti_msg)
#define S1AP_INITIAL_UE_MESSAGE(mSGpTR)          ((itti_s1ap_initial_ue_message_t*)(mSGpTR)->itti_msg)
#define S1AP_NAS_DL_DATA_REQ(mSGpTR)             ((itti_s1ap_nas_dl_data_req_t*)(mSGpTR)->itti_msg)
#define S1AP_UE_CAPABILITIES_IND(mSGpTR)         ((itti_s1ap_ue_cap_ind_t*)(mSGpTR)->itti_msg)

// NOT a ITTI message
typedef struct s1ap_initial_ue_message_s {
  enb_ue_s1ap_id_t     enb_ue_s1ap_id:24;
  ecgi_t                e_utran_cgi;
} s1ap_initial_ue_message_t;

#define S1AP_UE_RADIOCAPABILITY_MAX_SIZE 1000

typedef struct itti_s1ap_ue_cap_ind_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  uint8_t           radio_capabilities[S1AP_UE_RADIOCAPABILITY_MAX_SIZE];
  size_t            radio_capabilities_length;
} itti_s1ap_ue_cap_ind_t;

#define S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE 400
typedef struct itti_s1ap_eNB_deregistered_ind_s {
  uint16_t         nb_ue_to_deregister;
  mme_ue_s1ap_id_t mme_ue_s1ap_id[S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE];
} itti_s1ap_eNB_deregistered_ind_t;

typedef struct itti_s1ap_deregister_ue_req_s {
  mme_ue_s1ap_id_t mme_ue_s1ap_id;
} itti_s1ap_deregister_ue_req_t;

typedef struct itti_s1ap_ue_context_release_req_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  S1ap_Cause_t      cause;             // Should translate this S1ap_Cause_t type
} itti_s1ap_ue_context_release_req_t;

typedef enum s1ap_reset_type_e {
  RESET_ALL = 0,
  RESET_PARTIAL
} s1ap_reset_type_t;

typedef struct s1_sig_conn_id_s {
  mme_ue_s1ap_id_t*  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t*  enb_ue_s1ap_id;
} s1_sig_conn_id_t;

typedef struct itti_s1ap_enb_initiated_reset_req_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  uint32_t          enb_id;
  s1ap_reset_type_t  s1ap_reset_type;
  uint32_t          num_ue;
  s1_sig_conn_id_t  *ue_to_reset_list;
} itti_s1ap_enb_initiated_reset_req_t;

typedef struct itti_s1ap_enb_initiated_reset_ack_s {
  uint32_t          sctp_assoc_id;
  uint16_t          sctp_stream_id;
  s1ap_reset_type_t  s1ap_reset_type;
  uint32_t          num_ue;
  s1_sig_conn_id_t  *ue_to_reset_list;
} itti_s1ap_enb_initiated_reset_ack_t;


// NOT a ITTI message
typedef struct s1ap_initial_ue_message_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  ecgi_t                e_utran_cgi;
} s1ap_initial_ue_message_t;

#define S1AP_UE_RADIOCAPABILITY_MAX_SIZE 3000

typedef struct itti_s1ap_ue_cap_ind_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  uint8_t           radio_capabilities[S1AP_UE_RADIOCAPABILITY_MAX_SIZE];
  size_t            radio_capabilities_length;
} itti_s1ap_ue_cap_ind_t;

#define S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE 400
typedef struct itti_s1ap_eNB_deregistered_ind_s {
  uint16_t         nb_ue_to_deregister;
  enb_ue_s1ap_id_t enb_ue_s1ap_id[S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE];
  mme_ue_s1ap_id_t mme_ue_s1ap_id[S1AP_ITTI_UE_PER_DEREGISTER_MESSAGE];
  uint32_t         enb_id;
} itti_s1ap_eNB_deregistered_ind_t;

typedef struct itti_s1ap_deregister_ue_req_s {
  mme_ue_s1ap_id_t mme_ue_s1ap_id;
} itti_s1ap_deregister_ue_req_t;

typedef struct itti_s1ap_ue_context_release_req_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  uint32_t          enb_id;
  S1ap_Cause_t      cause;             // Should translate this S1ap_Cause_t type
} itti_s1ap_ue_context_release_req_t;

typedef struct itti_s1ap_ue_context_release_command_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  uint32_t          enb_id;
  enum s1cause      cause;
} itti_s1ap_ue_context_release_command_t;

typedef struct itti_s1ap_ue_context_release_complete_s {
  mme_ue_s1ap_id_t  mme_ue_s1ap_id;
  enb_ue_s1ap_id_t  enb_ue_s1ap_id:24;
  uint32_t          enb_id;
  uint32_t          sctp_assoc_id;
} itti_s1ap_ue_context_release_complete_t;

typedef struct itti_s1ap_initial_ue_message_s {
  sctp_assoc_id_t     sctp_assoc_id; // key stored in MME_APP for MME_APP forward NAS response to S1AP
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  bstring             nas;
  tai_t               tai;               /* Indicating the Tracking Area from which the UE has sent the NAS message.                         */
  ecgi_t              ecgi;              /* Indicating the cell from which the UE has sent the NAS message.                         */
  rrc_establishment_cause_t      rrc_establishment_cause;          /* Establishment cause                     */

  bool                is_s_tmsi_valid;
  bool                is_csg_id_valid;
  bool                is_gummei_valid;
  s_tmsi_t            opt_s_tmsi;
  csg_id_t            opt_csg_id;
  gummei_t            opt_gummei;
  //void                opt_cell_access_mode;
  //void                opt_cell_gw_transport_address;
  //void                opt_relay_node_indicator;
  /* Transparent message from s1ap to be forwarded to MME_APP or
   * to S1AP if connection establishment is rejected by NAS.
   */
  s1ap_initial_ue_message_t transparent;
} itti_s1ap_initial_ue_message_t;

typedef struct itti_s1ap_e_rab_setup_req_s {
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;

  // Applicable for non-GBR E-RABs
  bool                            ue_aggregate_maximum_bit_rate_present;
  ue_aggregate_maximum_bit_rate_t ue_aggregate_maximum_bit_rate;

  // E-RAB to Be Setup List
  e_rab_to_be_setup_list_t        e_rab_to_be_setup_list;

} itti_s1ap_e_rab_setup_req_t;

typedef struct itti_s1ap_e_rab_release_req_s {
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;

  // E-RAB to Be Setup List
  e_rab_list_t        e_rab_to_be_release_list;
  /** NAS message. */
  bstring                          nas_pdu;

} itti_s1ap_e_rab_release_req_t;

typedef struct itti_s1ap_e_rab_setup_rsp_s {
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;

  // E-RAB to Be Setup List
  e_rab_setup_list_t                  e_rab_setup_list;

  // Optional
  e_rab_list_t        e_rab_failed_to_setup_list;

} itti_s1ap_e_rab_setup_rsp_t;

typedef struct itti_s1ap_e_rab_release_rsp_s {
  mme_ue_s1ap_id_t    mme_ue_s1ap_id;
  enb_ue_s1ap_id_t    enb_ue_s1ap_id;

  // E-RAB to Be Setup List
  e_rab_list_t                  e_rab_release_list;

//   Optional
//  e_rab_list_t        e_rab_failed_to_setup_list;

} itti_s1ap_e_rab_release_rsp_t;


// handover messaging
typedef struct itti_s1ap_path_switch_request_s {
  uint32_t                mme_ue_s1ap_id;
  uint32_t                enb_ue_s1ap_id;
  sctp_assoc_id_t         sctp_assoc_id;
  sctp_stream_id_t        sctp_stream;
  uint32_t                enb_id;
  uint8_t                 no_of_e_rabs;
  ebi_t                   e_rab_id[BEARERS_PER_UE];
  bstring                 transport_layer_address[BEARERS_PER_UE];
  s1u_teid_t              gtp_teid[BEARERS_PER_UE];

//  /* Key eNB */
//  uint8_t                 kenb[32];
//
//  ambr_t                  ambr;
//  ambr_t                  apn_ambr;
//
//  /* EPS bearer ID */
//  unsigned                ebi:4;
//
//  /* QoS */
//  qci_t                   qci;
//  priority_level_t        prio_level;
//  pre_emp_vulnerability_t pre_emp_vulnerability;
//  pre_emp_capability_t    pre_emp_capability;
//
//  /* S-GW TEID for user-plane */
//  teid_t                  teid;
//  /* S-GW IP address for User-Plane */
//  ip_address_t            s_gw_address;
} itti_s1ap_path_switch_request_t;

/** Path Switch Request Acknowledgment sent from MME_APP to S1AP layer. */
typedef struct itti_s1ap_path_switch_request_ack_s {
  ebi_t                   eps_bearer_id;
  mme_ue_s1ap_id_t        ue_id;            /* UE lower layer identifier   */

  /** Bearer Contexts to be switched List. */
  bearer_contexts_to_be_created_t  *bearer_ctx_to_be_switched_list;

  /* Key eNB */
  uint8_t                 nh[AUTH_NH_SIZE];
  uint8_t                 ncc:3;

  uint16_t                security_capabilities_encryption_algorithms;
  uint16_t                security_capabilities_integrity_algorithms;
} itti_s1ap_path_switch_request_ack_t;

typedef struct itti_s1ap_handover_required_s {
  uint32_t                mme_ue_s1ap_id;
  uint32_t                enb_ue_s1ap_id;
  sctp_assoc_id_t         sctp_assoc_id;
  /** Target Id. */
  tai_t                   selected_tai;
  ecgi_t                  global_enb_id;
  uint8_t                 target_enb_type;

  /** Cause. */
  S1ap_Cause_PR           f_cause_type;
  long                    f_cause_value;

  /** Source-To-Target Transparent Container. */
  // todo: if this is an buffer, how is it freed? does everything needs to be stacked
  bstring                 eutran_source_to_target_container;
} itti_s1ap_handover_required_t;

typedef struct itti_s1ap_handover_command_s {
  ebi_t                   eps_bearer_id;

  /** Since no inner NAS structure is present (message does not travel NAS layer) set the UE_IDs manually. */
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;

  uint32_t                enb_id;
  /** F-Container. */
  bstring                 eutran_target_to_source_container;
  bearer_contexts_to_be_created_t *bearer_ctx_to_be_forwarded_list;

  // todo: handover type will always be set as intra_lte in s1ap layer..

  // todo: add the enb_ue_s1ap_id and the mme_ue_s1ap_id.. No
} itti_s1ap_handover_command_t;

typedef struct itti_s1ap_handover_request_s {
  mme_ue_s1ap_id_t        ue_id;            /* UE lower layer identifier   */

  /* Key eNB */
  uint8_t                 nh[AUTH_NH_SIZE];
  uint8_t                 ncc:3;

  uint16_t                security_capabilities_encryption_algorithms;
  uint16_t                security_capabilities_integrity_algorithms;

  /** UE AMBR. */
  ambr_t                          ambr;
  /** Bearer Contexts to be Setup List. */
  bearer_contexts_to_be_created_t *bearer_ctx_to_be_setup_list;

  /** F-Container. */
  bstring                   source_to_target_eutran_container;
  /** Target Id. */
//  tai_t                           target_tai;
  unsigned                        macro_enb_id:20;    /* Macro-Enb-Id.                        */
} itti_s1ap_handover_request_t;

/** Handover Request Acknowledge. */
typedef struct itti_s1ap_handover_request_acknowledge_s {
  uint32_t                mme_ue_s1ap_id;
  uint32_t                enb_ue_s1ap_id;
//  sctp_assoc_id_t         sctp_assoc_id;
//  sctp_stream_id_t        sctp_stream;
  uint8_t                 no_of_e_rabs;
  ebi_t                   e_rab_id[BEARERS_PER_UE];
  bstring                 transport_layer_address[BEARERS_PER_UE];
  s1u_teid_t              gtp_teid[BEARERS_PER_UE];

  bstring                 target_to_source_eutran_container; /**< Target-ToSource Transparent Container. */
} itti_s1ap_handover_request_acknowledge_t;


/** Handover Failure. */
typedef struct itti_s1ap_handover_failure_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
//  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  uint32_t                enb_id;
  sctp_assoc_id_t         assoc_id;
  enum s1cause            cause;
} itti_s1ap_handover_failure_t;

/** Handover Preparation Failure. */
typedef struct itti_s1ap_handover_preparation_failure_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  sctp_assoc_id_t         assoc_id;
  enum s1cause            cause;
} itti_s1ap_handover_preparation_failure_t;

/** S1AP Error Indication. */
typedef struct itti_s1ap_error_indication_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  sctp_assoc_id_t         assoc_id;
  uint32_t                enb_id;
  enum s1cause            cause;
}itti_s1ap_error_indication_t;

/** Path Switch Request Failure. */
typedef struct itti_s1ap_path_switch_request_failure_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  sctp_assoc_id_t         assoc_id;
  enum s1cause            cause;
} itti_s1ap_path_switch_request_failure_t;

/** Handover Cancel. */
typedef struct itti_s1ap_handover_cancel_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  sctp_assoc_id_t         assoc_id;
} itti_s1ap_handover_cancel_t;

/** Handover Cancel Acknowledge. */
typedef struct itti_s1ap_handover_cancel_acknowledge_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  sctp_assoc_id_t         assoc_id;
} itti_s1ap_handover_cancel_acknowledge_t;

/** S1AP ENB/MME Status Transfer. */
typedef struct itti_s1ap_status_transfer_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  enb_ue_s1ap_id_t        enb_ue_s1ap_id:24;
  /** enb_id to find the correct UE_Reference. */
  uint32_t                enb_id;
  /** F-Container. */
  bstring                 bearerStatusTransferList_buffer; /**< Target-ToSource Transparent Container. */

} itti_s1ap_status_transfer_t;

typedef struct itti_s1ap_handover_notify_s {
  uint32_t                mme_ue_s1ap_id;
  uint32_t                enb_ue_s1ap_id;
  sctp_assoc_id_t         assoc_id;

  tai_t                   tai;               /* Indicating the Tracking Area from which the UE has sent the NAS message.                         */
  ecgi_t                  cgi;

} itti_s1ap_handover_notify_t;

typedef struct itti_s1ap_paging_s {
  mme_ue_s1ap_id_t        mme_ue_s1ap_id;
  sctp_assoc_id_t         sctp_assoc_id_key; // link with eNB id

  uint16_t                ue_identity_index;
  tmsi_t                  tmsi;

  tai_t                   tai;               /* Indicating the Tracking Area from which the UE has sent the NAS message.                         */

} itti_s1ap_paging_t;
#ifdef __cplusplus
}
#endif
#endif /* FILE_S1AP_MESSAGES_TYPES_SEEN */
