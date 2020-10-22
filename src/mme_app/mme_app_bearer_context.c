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


/*! \file mme_app_bearer_context.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "common_defs.h"
#include "esm_cause.h"
#include "mme_app_bearer_context.h"
#include "mme_app_esm_procedures.h"

/**
 * Create a bearer context pool, not to reallocate for each new UE.
 * todo: remove the pool with shutdown?
 */
static bearer_context_t                 *bearerContextPool = NULL;

static esm_cause_t
mme_app_esm_bearer_context_finalize_tft(mme_ue_s1ap_id_t ue_id, bearer_context_t * bearer_context, traffic_flow_template_t * tft);

//------------------------------------------------------------------------------
bstring bearer_state2string(const mme_app_bearer_state_t bearer_state)
{
  bstring bsstr = NULL;
  if  (BEARER_STATE_NULL == bearer_state) {
    bsstr = bfromcstr("BEARER_STATE_NULL");
    return bsstr;
  }
  bsstr = bfromcstr(" ");
  if  (BEARER_STATE_SGW_CREATED & bearer_state) bcatcstr(bsstr, "SGW_CREATED ");
  if  (BEARER_STATE_MME_CREATED & bearer_state) bcatcstr(bsstr, "MME_CREATED ");
  if  (BEARER_STATE_ENB_CREATED & bearer_state) bcatcstr(bsstr, "ENB_CREATED ");
  if  (BEARER_STATE_ACTIVE & bearer_state) bcatcstr(bsstr, "ACTIVE");
  return bsstr;
}

//------------------------------------------------------------------------------
bearer_context_t *mme_app_new_bearer(){
  bearer_context_t * thiz = NULL;
  if (bearerContextPool) {
    thiz = bearerContextPool;
    bearerContextPool = bearerContextPool->next_bc;
  } else {
    thiz = calloc (1, sizeof (bearer_context_t));
  }
  return thiz;
}

//------------------------------------------------------------------------------
void mme_app_bearer_context_initialize(bearer_context_t *bearer_context)
{
  /** Remove the EMS-EBR context of the bearer-context. */
  bearer_context->esm_ebr_context.status   = ESM_EBR_INACTIVE;
  if(bearer_context->esm_ebr_context.tft){
    free_traffic_flow_template(&bearer_context->esm_ebr_context.tft);
  }
//  if(bearer_context->esm_ebr_context.pco){
//    free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
//  }
  /** Remove the remaining contexts of the bearer context. */
  memset(&bearer_context->esm_ebr_context, 0, sizeof(esm_ebr_context_t)); /**< Sets the SM status to ESM_STATUS_INVALID. */
  ebi_t ebi = bearer_context->ebi;
  memset(bearer_context, 0, sizeof(*bearer_context));
  bearer_context->ebi = ebi;
}

//------------------------------------------------------------------------------
void mme_app_free_bearer_context (bearer_context_t ** const bearer_context)
{
//  free_esm_bearer_context(&(*bearer_context)->esm_ebr_context);
//  free_wrapper((void**)bearer_context);
  memset(*bearer_context, 0, sizeof(bearer_context_t));
  (*bearer_context)->next_bc = bearerContextPool;
  bearerContextPool = (*bearer_context);
}

//------------------------------------------------------------------------------
bearer_context_t* mme_app_get_session_bearer_context(pdn_context_t * const pdn_context, const ebi_t ebi)
{
  bearer_context_t    bc_key = {.ebi = ebi};
  return RB_FIND(SessionBearers, &pdn_context->session_bearers, &bc_key);
}

//------------------------------------------------------------------------------
void mme_app_get_free_bearer_context(ue_context_t * const ue_context, const ebi_t ebi, bearer_context_t ** bc_pp)
{
  bearer_context_t    bc_key = {.ebi = ebi};
  (*bc_pp) = RB_FIND(BearerPool, &ue_context->bearer_pool, &bc_key);
}

//------------------------------------------------------------------------------
void mme_app_get_session_bearer_context_from_all(ue_context_t * const ue_context, const ebi_t ebi, bearer_context_t ** bc_pp)
{
  bearer_context_t  * bearer_context = NULL;
  pdn_context_t     * pdn_context = NULL;
  *bc_pp = NULL;
  RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
    RB_FOREACH (bearer_context, SessionBearers, &pdn_context->session_bearers) {
      // todo: better error handling
      if(bearer_context->ebi == ebi){
        *bc_pp = bearer_context;
      }
    }
  }
}

//------------------------------------------------------------------------------
esm_cause_t
mme_app_register_dedicated_bearer_context(const mme_ue_s1ap_id_t ue_id, const esm_ebr_state esm_ebr_state, pdn_cid_t pdn_cid, ebi_t linked_ebi,
    bearer_context_to_be_created_t * const bc_tbc, const ebi_t ded_ebi)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t               *pdn_context = NULL;
  bearer_context_t            *pBearerCtx  = NULL; /**< Define a bearer context key. */

  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT " to create a new dedicated bearer context. \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }
  mme_app_get_pdn_context(ue_context->mme_ue_s1ap_id, pdn_cid, linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_MME_APP, "No PDN context for UE: " MME_UE_S1AP_ID_FMT " could be found (cid=%d,ebi=%d) to create a new dedicated bearer context. \n", ue_id, pdn_cid, linked_ebi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
  }

  /* Check that it is active. */
  bearer_context_t * default_bc = mme_app_get_session_bearer_context(pdn_context, linked_ebi);
  if(!default_bc || default_bc->esm_ebr_context.status != ESM_EBR_ACTIVE){
    OAILOG_ERROR (LOG_MME_APP, "Default bearer (ebi=%d) of PDN context for UE: " MME_UE_S1AP_ID_FMT " (cid=%d) is not existing or not in active state. \n",
        linked_ebi, ue_id, pdn_context->context_identifier);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    /** This should be enough, we don't need to additionally check for the states. */
  }
  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
  if(ded_ebi == EPS_BEARER_IDENTITY_UNASSIGNED)
	  pBearerCtx = RB_MIN(BearerPool, &ue_context->bearer_pool);
  else {
	  mme_app_get_free_bearer_context(ue_context, ded_ebi, &pBearerCtx);
  }
  if(!pBearerCtx){
    OAILOG_ERROR(LOG_MME_APP,  "Could not find a free bearer context with for ue_id " MME_UE_S1AP_ID_FMT" for ded_ebi=%d! \n", ue_id, ded_ebi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  if(validateEpsQosParameter(bc_tbc->bearer_level_qos.qci, bc_tbc->bearer_level_qos.pvi, bc_tbc->bearer_level_qos.pci, bc_tbc->bearer_level_qos.pl,
      bc_tbc->bearer_level_qos.gbr.br_dl, bc_tbc->bearer_level_qos.gbr.br_ul, bc_tbc->bearer_level_qos.mbr.br_dl, bc_tbc->bearer_level_qos.mbr.br_ul) == RETURNerror){
    OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "EPS bearer context of CBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous EPS QoS.\n",
        ue_id, pdn_cid, linked_ebi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_EPS_QOS_NOT_ACCEPTED);
  }
  /** Validate the TFT and packet filters don't have semantical errors.. */
  if(bc_tbc->tft->tftoperationcode != TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE_NEW_TFT){
    OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "EPS bearer context of CBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous TFT code %d. \n",
        ue_id, pdn_cid, linked_ebi, bc_tbc->tft->tftoperationcode);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION);
  }
  DevAssert(!pBearerCtx->esm_ebr_context.tft);
  esm_cause_t esm_cause = verify_traffic_flow_template (bc_tbc->tft, pBearerCtx->esm_ebr_context.tft);
  if(esm_cause != ESM_CAUSE_SUCCESS){
    OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "EPS bearer context of CBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous TFT. EsmCause %d. \n",
        ue_id, pdn_cid, linked_ebi, esm_cause);
    OAILOG_FUNC_RETURN (LOG_MME_APP, esm_cause);
  }
  OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "ESM QoS and TFT could be verified of CBR received for UE " MME_UE_S1AP_ID_FMT".\n", ue_id);

  // todo: LOCK_UE_CONTEXT(
  // todo: EBI must match
  pBearerCtx = RB_REMOVE(BearerPool, &ue_context->bearer_pool, pBearerCtx);
  DevAssert(pBearerCtx);
  AssertFatal((EPS_BEARER_IDENTITY_LAST >= pBearerCtx->ebi) && (EPS_BEARER_IDENTITY_FIRST <= pBearerCtx->ebi), "Bad ebi %u", pBearerCtx->ebi);
  /* Check that there is no collision when adding the bearer context into the PDN sessions bearer pool. */
  pBearerCtx->pdn_cx_id              = pdn_context->context_identifier;
  pBearerCtx->esm_ebr_context.status = esm_ebr_state;
  pBearerCtx->linked_ebi             = pdn_context->default_ebi;
  /* Set the TFT, PCC and QoS features */
  pBearerCtx->esm_ebr_context.tft    = bc_tbc->tft;
  bc_tbc->tft = NULL;
  memcpy((void*)&pBearerCtx->bearer_level_qos, &bc_tbc->bearer_level_qos, sizeof(bearer_qos_t));
  /* Insert the bearer context. */
  DevAssert(!RB_INSERT (SessionBearers, &pdn_context->session_bearers, pBearerCtx)); /**< Collision Check. */
  /** No dedicated bearer level PCO supported. */
  bc_tbc->eps_bearer_id = pBearerCtx->ebi;

  /* Set the TEIDs. */
  memcpy((void*)&pBearerCtx->s_gw_fteid_s1u,      &bc_tbc->s1u_sgw_fteid,     sizeof(fteid_t));
  memcpy((void*)&pBearerCtx->p_gw_fteid_s5_s8_up, &bc_tbc->s5_s8_u_pgw_fteid, sizeof(fteid_t));
  pBearerCtx->bearer_state   |= BEARER_STATE_SGW_CREATED;
  pBearerCtx->bearer_state   |= BEARER_STATE_MME_CREATED;
  OAILOG_INFO (LOG_MME_APP, "Successfully set dedicated bearer context (ebi=%d,cid=%d) and for UE " MME_UE_S1AP_ID_FMT "\n",
      pBearerCtx->ebi, pdn_context->context_identifier, ue_id);
  // todo: UNLOCK_UE_CONTEXT
  OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_SUCCESS);
}

////------------------------------------------------------------------------------
//int mme_app_deregister_bearer_context(ue_context_t * const ue_context, ebi_t ebi, const pdn_context_t *pdn_context)
//{
//  AssertFatal((EPS_BEARER_IDENTITY_LAST >= ebi) && (EPS_BEARER_IDENTITY_FIRST <= ebi), "Bad ebi %u", ebi);
//  bearer_context_t            *pBearerCtx = NULL; /**< Define a bearer context key. */
//
//  /** Check that the PDN session exists. */
//  // todo: add a lot of locks..
//  bearer_context_t bc_key = { .ebi = ebi}; /**< Define a bearer context key. */ // todo: just setting one element, and maybe without the key?
//  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
//  pBearerCtx = RB_FIND(BearerPool, &pdn_context->session_bearers, &bc_key);
//  if(!pBearerCtx){
//    OAILOG_ERROR(LOG_MME_APP,  "Could not find a session bearer context with ebi %d in pdn context %d for ue_id " MME_UE_S1AP_ID_FMT"! \n", ebi, pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
//    OAILOG_FUNC_RETURN (LOG_MME_APP, NULL);
//  }
//  /** Removed a bearer context from the UE contexts bearer pool and adds it into the PDN sessions bearer pool. */
//  bearer_context_t *pBearerCtx_removed = NULL;
//  pBearerCtx_removed = RB_REMOVE(SessionBearers, &pdn_context->session_bearers, pBearerCtx);
//  if(!pBearerCtx_removed){
//    OAILOG_ERROR(LOG_MME_APP,  "Could not find an session bearer context with ebi %d for ue_id " MME_UE_S1AP_ID_FMT " inside pdn context with context id %d! \n",
//        ebi, ue_context->mme_ue_s1ap_id, pdn_context->context_identifier);
//    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
//  }
//  /*
//   * Delete the TFT,
//   * todo: check any other allocated fields..
//   *
//   */
//  // TODO Look at "free_traffic_flow_template"
//  //free_traffic_flow_template(&pdn->bearer[i]->tft);
//
//  /*
//   * We don't have one pool where tunnels are allocated. We allocate a fixed number of bearer contexts at the beginning inside the UE context.
//   * So the delete function is unlike to GTPv2c tunnels.
//   */
//
//  /** Initialize the new bearer context. */
//  mme_app_bearer_context_init(pBearerCtx_removed);
//
//  /** Insert the bearer context into the free bearer of the ue context. */
//  RB_INSERT (BearerPool, &ue_context->bearer_pool, pBearerCtx_removed);
//
//  OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u and for ue_id " MME_UE_S1AP_ID_FMT "\n",
//      pBearerCtx->ebi, pdn_context->context_identifier, ue_context->mme_ue_s1ap_id);
//  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
//}

//------------------------------------------------------------------------------
void mme_app_bearer_context_s1_release_enb_informations(bearer_context_t * const bc)
{
  if (bc) {
    bc->bearer_state = BEARER_STATE_S1_RELEASED;
    memset(&bc->enb_fteid_s1u, 0, sizeof(bc->enb_fteid_s1u));
    bc->enb_fteid_s1u.teid = INVALID_TEID;
  }
}

// todo: do this with macros, such that it is always locked..
//------------------------------------------------------------------------------
int
mme_app_modify_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, bearer_contexts_to_be_modified_t *bcs_to_be_modified){
  OAILOG_FUNC_IN(LOG_MME_APP);

  ue_context_t      * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  pdn_context_t     * pdn_context = NULL;
  bearer_context_t  * bearer_context = NULL;

  if(!ue_context){
    OAILOG_INFO(LOG_MME_APP, "No UE context is found" MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  // todo: LOCK_UE_CONTEXT(ue_context);
  // todo: checking on procedures of the function.. mme_app_is_ue_context_clean(ue_context)?!?

  /** Get the PDN Context. */
  for(int nb_bearer = 0; nb_bearer < bcs_to_be_modified->num_bearer_context; nb_bearer++) {
    bearer_context_to_be_modified_t *bc_to_be_modified = &bcs_to_be_modified->bearer_contexts[nb_bearer];
    /** Get the bearer context. */
    bearer_context_t * bearer_context = NULL;
    mme_app_get_session_bearer_context_from_all(ue_context, bc_to_be_modified->eps_bearer_id, &bearer_context);
    if(!bearer_context){
      OAILOG_ERROR(LOG_MME_APP, "No bearer context (ebi=%d) could be found for " MME_UE_S1AP_ID_FMT ". Skipping.. \n", bc_to_be_modified->eps_bearer_id, mme_ue_s1ap_id);
      continue;
    }
    /** Set all bearers, not in the failed list, to inactive. */
    bearer_context->bearer_state &= (~BEARER_STATE_ACTIVE);
    /** Update the FTEID of the bearer context and uncheck the established state. */
    bearer_context->enb_fteid_s1u.teid = bc_to_be_modified->s1_eNB_fteid.teid;
    bearer_context->enb_fteid_s1u.interface_type      = S1_U_ENODEB_GTP_U;
    /** Set the IP address from the FTEID. */
    if (bc_to_be_modified->s1_eNB_fteid.ipv4) {
      bearer_context->enb_fteid_s1u.ipv4 = 1;
      bearer_context->enb_fteid_s1u.ipv4_address.s_addr = bc_to_be_modified->s1_eNB_fteid.ipv4_address.s_addr;
    }
    if (bc_to_be_modified->s1_eNB_fteid.ipv6) {
      bearer_context->enb_fteid_s1u.ipv6 = 1;
      memcpy(&bearer_context->enb_fteid_s1u.ipv6_address, &bc_to_be_modified->s1_eNB_fteid.ipv6_address, sizeof(bc_to_be_modified->s1_eNB_fteid.ipv6_address));
    }
    bearer_context->bearer_state |= BEARER_STATE_ENB_CREATED;
    bearer_context->bearer_state |= BEARER_STATE_MME_CREATED; // todo: remove this flag.. unnecessary
  }
  // todo: No change in the APN..

  /*
   * The APN-AMBR value will already be set.
   * Independently from the ESM (not checking the ESM_EBR_STATE), send the MBR to the PGW.
   * If the ESM procedure is rejected or gets into timeout, we must remove the session PGW session via ESM separately.
   */
//  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
void
mme_app_release_bearers(const mme_ue_s1ap_id_t mme_ue_s1ap_id, e_rab_list_t * e_rab_list, ebi_list_t * const ebi_list) {
  OAILOG_FUNC_IN(LOG_MME_APP);

  ue_context_t                  * ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, mme_ue_s1ap_id);
  pdn_context_t                 * pdn_context = NULL;
  bearer_context_t              * bearer_context = NULL;
  nas_esm_proc_bearer_context_t * esm_proc_bearer_context = NULL;

  if(!ue_context){
    OAILOG_INFO(LOG_MME_APP, "No UE context is found" MME_UE_S1AP_ID_FMT ". \n", mme_ue_s1ap_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  /** Check that no ESM procedures are running. */
  esm_proc_bearer_context = mme_app_nas_esm_get_bearer_context_procedure(mme_ue_s1ap_id, PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED, EPS_BEARER_IDENTITY_UNASSIGNED);
  if(esm_proc_bearer_context){
    OAILOG_ERROR(LOG_MME_APP, "ESM bearer context procedure (pti=%d,ebi=%d) exists for ue_id" MME_UE_S1AP_ID_FMT ". Not releasing bearer. \n",
        esm_proc_bearer_context->esm_base_proc.pti, esm_proc_bearer_context->bearer_ebi, mme_ue_s1ap_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  // todo: LOCK_UE_CONTEXT(ue_context);
  // todo: checking on procedures of the function.. mme_app_is_ue_context_clean(ue_context)?!?

  /** Set all FTEIDs, also those not in the list to 0. */
  if(e_rab_list){
    for(int num_ebi = 0; num_ebi < e_rab_list->no_of_items; num_ebi++){
      bearer_context = NULL;
      mme_app_get_session_bearer_context_from_all(ue_context, e_rab_list->item[num_ebi].e_rab_id, &bearer_context);
      if(bearer_context){
        if( (bearer_context->bearer_state & BEARER_STATE_ACTIVE)
            && (bearer_context->bearer_state & BEARER_STATE_ENB_CREATED)) {
          ebi_list->ebis[ebi_list->num_ebi] = bearer_context->ebi;
          bearer_context->bearer_state &= (~BEARER_STATE_ACTIVE);
          bearer_context->bearer_state &= (~BEARER_STATE_ENB_CREATED);
          memset(&bearer_context->enb_fteid_s1u, 0, sizeof(fteid_t));
          OAILOG_INFO(LOG_MME_APP, "Set ebi=%d as released for ue_id " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, mme_ue_s1ap_id);
          ebi_list->num_ebi++;
        } else {
          OAILOG_WARNING(LOG_MME_APP, "Skipping ebi=%d with invalid state %d for ue_id " MME_UE_S1AP_ID_FMT " from implicit removal. \n",
              bearer_context->ebi, bearer_context->bearer_state, mme_ue_s1ap_id);
        }
      }
    }
  } else {
    /** X2 Case. */
    OAILOG_WARNING(LOG_MME_APP, "No EBI list has been received for ue_id " MME_UE_S1AP_ID_FMT ". Setting all bearers to released. \n", mme_ue_s1ap_id);
    RB_FOREACH (pdn_context, PdnContexts, &ue_context->pdn_contexts) {
    	RB_FOREACH (bearer_context, SessionBearers, &pdn_context->session_bearers) {
    	      if( (bearer_context->bearer_state & BEARER_STATE_ACTIVE)
    	          && (bearer_context->bearer_state & BEARER_STATE_ENB_CREATED)) {
    	        ebi_list->ebis[ebi_list->num_ebi] = bearer_context->ebi;
    	        bearer_context->bearer_state &= (~BEARER_STATE_ACTIVE);
//    	        bearer_context->bearer_state &= (~BEARER_STATE_ENB_CREATED);
    	        memset(&bearer_context->enb_fteid_s1u, 0, sizeof(fteid_t));
    	        DevAssert(!(bearer_context->bearer_state & BEARER_STATE_ACTIVE));

    	        OAILOG_INFO(LOG_MME_APP, "Set ebi=%d (%p) as released for ue_id " MME_UE_S1AP_ID_FMT ". \n",
    	        		bearer_context->ebi, bearer_context, mme_ue_s1ap_id);
    	        ebi_list->num_ebi++;
    	      } else {
    	        OAILOG_WARNING(LOG_MME_APP, "Skipping ebi=%d with invalid state %d for ue_id " MME_UE_S1AP_ID_FMT " from implicit removal. \n",
    	            bearer_context->ebi, bearer_context->bearer_state, mme_ue_s1ap_id);
    	      }
    	}
    }
  }
  // UNLOCK_UE_CONTEXT
  OAILOG_INFO(LOG_MME_APP, "Returning %d bearer ready to be released for ue_id " MME_UE_S1AP_ID_FMT ". \n", ebi_list->num_ebi, mme_ue_s1ap_id);
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_cn_update_bearer_context(mme_ue_s1ap_id_t ue_id, const ebi_t ebi,
    struct e_rab_setup_item_s * s1u_erab_setup_item, struct fteid_s * s1u_saegw_fteid){
  ue_context_t * ue_context         = NULL;
  bearer_context_t * bearer_context = NULL;
  int rc                            = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
  if(!bearer_context){
    OAILOG_ERROR (LOG_MME_APP, "No bearer context (ebi=%d) for UE: " MME_UE_S1AP_ID_FMT ". \n", ebi, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }

  //  LOCK_UE_CONTEXT(ue_context);
  /** Update the FTEIDs and the bearers CN state. */
  if(s1u_erab_setup_item){
    /** Set the TEID of the tbc and the bearer context. */
    bearer_context->enb_fteid_s1u.interface_type = S1_U_ENODEB_GTP_U;
    bearer_context->enb_fteid_s1u.teid           = s1u_erab_setup_item->gtp_teid;
    /** Set the IP address. */
    if (4 == blength(s1u_erab_setup_item->transport_layer_address)) {
      bearer_context->enb_fteid_s1u.ipv4         = 1;
      memcpy(&bearer_context->enb_fteid_s1u.ipv4_address,
          s1u_erab_setup_item->transport_layer_address->data, blength(s1u_erab_setup_item->transport_layer_address));
    } else if (16 == blength(s1u_erab_setup_item->transport_layer_address)) {
      bearer_context->enb_fteid_s1u.ipv6         = 1;
      memcpy(&bearer_context->enb_fteid_s1u.ipv6_address,
          s1u_erab_setup_item->transport_layer_address->data,
          blength(s1u_erab_setup_item->transport_layer_address));
    } else {
      AssertFatal(0, "TODO IP address %d bytes", blength(s1u_erab_setup_item->transport_layer_address));
    }
    bearer_context->bearer_state |= BEARER_STATE_ENB_CREATED;
  }
  /* Set S1U SAEGW TEID. */
  if(s1u_saegw_fteid){
    memcpy((void*)&bearer_context->s_gw_fteid_s1u, s1u_saegw_fteid, sizeof(fteid_t));
    //      memcpy((void*)&bearer_context->p_gw_fteid_s5_s8_up , fteid_set->s5_fteid, sizeof(fteid_t));
    bearer_context->bearer_state |= BEARER_STATE_SGW_CREATED;
  }
  /** Check, if the ESM context is active, set the bearer as active. */
  if(bearer_context->esm_ebr_context.status == ESM_EBR_ACTIVE){
	  OAILOG_INFO(LOG_MME_APP, "Bearer context (ebi=%d) for UE: " MME_UE_S1AP_ID_FMT " is in ACTIVE ESM state. "
			  "Setting bearer context state as active. \n", ebi, ue_id);
	  bearer_context->bearer_state |= BEARER_STATE_ACTIVE;
  }
  /** Set the MME_APP states (todo: may be with Activate Dedicated Bearer Response). */
  // todo:     bearer_context->bearer_state   |= BEARER_STATE_MME_CREATED;
  //  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

/*
 * Just verifying but not setting the actual parameters.
 * Only setting the ESM EBR state as modification pending.
 */
//------------------------------------------------------------------------------
esm_cause_t
mme_app_esm_modify_bearer_context(mme_ue_s1ap_id_t ue_id, const ebi_t ebi, ebi_list_t * const ded_ebis, const esm_ebr_state esm_ebr_state, struct bearer_qos_s * bearer_level_qos, traffic_flow_template_t * tft, ambr_t *apn_ambr){
  ue_context_t * ue_context         = NULL;
  bearer_context_t * bearer_context = NULL;
  pdn_context_t * pdn_context       = NULL;
  int rc                            = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }
  /** Update the FTEIDs and the bearers CN state. */
  mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
  if(!bearer_context){
    OAILOG_ERROR (LOG_MME_APP, "No bearer context for ebi=%d context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ebi, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }
  mme_app_get_pdn_context(ue_id, bearer_context->pdn_cx_id, bearer_context->linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_MME_APP, "No PDN context for (cid=%d,linked_ebi=%d) could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", bearer_context->pdn_cx_id, bearer_context->linked_ebi, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }
  if(!(bearer_context->bearer_state & (BEARER_STATE_ACTIVE | BEARER_STATE_ENB_CREATED))){
	  OAILOG_WARNING (LOG_MME_APP, "Bearer context for ebi=%d not in correct bearer state %d for UE: " MME_UE_S1AP_ID_FMT ". \n", ebi, bearer_context->bearer_state, ue_id);
	  OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /** We can set the FTEIDs right before the CBResp is set. */
  if(bearer_context->esm_ebr_context.status != ESM_EBR_ACTIVE){
    OAILOG_ERROR(LOG_MME_APP, "ESM-PROC  - ESM Bearer Context for ebi %d is not ACTIVE for ue " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  // todo: LOCK_UE_CONTEXT
  // todo: not locking the context here (maybe to set flags)

  /** Verify the TFT, QoS, etc. */
  /** Before assigning the bearer context, validate the fields of the requested bearer context to be created. */
  if(bearer_level_qos){
    if(validateEpsQosParameter(bearer_level_qos->qci, bearer_level_qos->pvi, bearer_level_qos->pci, bearer_level_qos->pl,
        bearer_level_qos->gbr.br_dl, bearer_level_qos->gbr.br_ul, bearer_level_qos->mbr.br_dl, bearer_level_qos->mbr.br_ul) == RETURNerror){
      OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "EPS bearer context of UBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous EPS QoS.\n", ue_context->mme_ue_s1ap_id);
      OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_EPS_QOS_NOT_ACCEPTED);
    }
  }
  /** Validate the TFT and packet filters don't have semantical errors.. */
  if(tft) {
    if(!((tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT)
        ||(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT)
        ||(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT))){
      OAILOG_ERROR(LOG_NAS_EMM, "EMMCN-SAP  - " "EPS bearer context of UBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous TFT code %d. \n",
          ue_id, tft->tftoperationcode);
      OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION);
    }
    /** Verify the TFT together with the original TFT. */
    esm_cause_t esm_cause = verify_traffic_flow_template(tft, bearer_context->esm_ebr_context.tft);
    if(esm_cause != ESM_CAUSE_SUCCESS){
      OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "EPS bearer context of UBR received for UE " MME_UE_S1AP_ID_FMT" could not be verified due erroneous TFT. EsmCause %d. \n", ue_id, esm_cause);
      OAILOG_FUNC_RETURN (LOG_MME_APP, esm_cause);
    }
  }
  if(apn_ambr){
    /* Check the APN-AMBR, that it does not exceed the subscription. */
      if(apn_ambr->br_dl && apn_ambr->br_ul){
        // todo: update apn-ambr
      }
  }
  bearer_context->esm_ebr_context.status = esm_ebr_state;
  if(esm_ebr_state == ESM_EBR_INACTIVE_PENDING && pdn_context->default_ebi == bearer_context->ebi){
    DevAssert(ded_ebis);
    memset(ded_ebis, 0, sizeof(ebi_list_t));
    RB_FOREACH (bearer_context, SessionBearers, &pdn_context->session_bearers) {
      // todo: better error handling
      if(bearer_context->ebi != pdn_context->default_ebi){
        ded_ebis->ebis[ded_ebis->num_ebi] = bearer_context->ebi;
        ded_ebis->num_ebi++;
      }
    }
  }
  // todo: UNLOCK_UE_CONTEXT
  OAILOG_INFO(LOG_NAS_EMM, "EMMCN-SAP  - " "ESM QoS and TFT could be verified of UBR received for UE " MME_UE_S1AP_ID_FMT".\n", ue_id);
  /** Not updating the parameters yet. Updating later when a success is received. State will be updated later. */
  OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SUCCESS);
}

//------------------------------------------------------------------------------
esm_cause_t
mme_app_finalize_bearer_context(mme_ue_s1ap_id_t ue_id, const pdn_cid_t pdn_cid, const ebi_t linked_ebi, const ebi_t ebi, ambr_t *ambr, bearer_qos_t * bearer_level_qos, traffic_flow_template_t * tft,
    protocol_configuration_options_t * pco){
  OAILOG_FUNC_IN (LOG_MME_APP);

  ue_context_t * ue_context         = NULL;
  bearer_context_t * bearer_context = NULL;
  pdn_context_t    * pdn_context    = NULL;
  int rc                            = RETURNok;

  ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED);
  }

  if(pdn_cid != PDN_CONTEXT_IDENTIFIER_UNASSIGNED){
    mme_app_get_pdn_context(ue_context->mme_ue_s1ap_id, pdn_cid, linked_ebi, NULL, &pdn_context);
    if(!pdn_context){
      OAILOG_ERROR (LOG_MME_APP, "No PDN context for UE: " MME_UE_S1AP_ID_FMT " could be found (cid=%d,ebi=%d) to create a new dedicated bearer context. \n", ue_id, pdn_cid, linked_ebi);
      OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME);
    }
    bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
  }else{
    mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
  }
  if(!bearer_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP bearer context could be found for UE: " MME_UE_S1AP_ID_FMT " for (def_ebi=%d, ded_ebi=%d). \n",
    		ue_id, linked_ebi, ebi);
    OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }
  /** Get the pdn context. */
  if(!pdn_context){
	  mme_app_get_pdn_context(ue_context->mme_ue_s1ap_id, bearer_context->pdn_cx_id, bearer_context->linked_ebi, NULL, &pdn_context);
	  if(!pdn_context){
		  OAILOG_ERROR (LOG_MME_APP, "No PDN context could be found for UE: " MME_UE_S1AP_ID_FMT " for (cid=%d,linked_ebi=%d). \n",
				  ue_id, bearer_context->pdn_cx_id, bearer_context->linked_ebi);
		  OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
	  }
  }
  /** We can set the FTEIDs right before the CBResp is set. */
  // todo: LOCK_UE_CONTEXT
  bearer_context->esm_ebr_context.status = ESM_EBR_ACTIVE;
  /** Set the TFT, if exists. */
  if(tft){
    esm_cause_t esm_cause = mme_app_esm_bearer_context_finalize_tft(ue_id, bearer_context, tft);
    if(esm_cause != ESM_CAUSE_SUCCESS){
      OAILOG_FUNC_RETURN (LOG_MME_APP, esm_cause);
    }
    OAILOG_DEBUG(LOG_MME_APP, "Finalized the TFT of bearer (ebi=%d) for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id, bearer_context->ebi);
  }
  if(bearer_level_qos){
    /** Update the QCI. */
    bearer_context->bearer_level_qos.qci = bearer_level_qos->qci;

    /** Update the GBR and MBR values (if existing). */
    bearer_context->bearer_level_qos.gbr.br_dl= bearer_level_qos->gbr.br_dl;
    bearer_context->bearer_level_qos.gbr.br_ul = bearer_level_qos->gbr.br_ul;
    bearer_context->bearer_level_qos.gbr.br_dl = bearer_level_qos->mbr.br_dl;
    bearer_context->bearer_level_qos.gbr.br_ul = bearer_level_qos->mbr.br_ul;

    /** Set the priority values. */
    bearer_context->bearer_level_qos.pci = bearer_level_qos->pci;
    bearer_context->bearer_level_qos.pvi = bearer_level_qos->pvi;
    bearer_context->bearer_level_qos.pvi = bearer_level_qos->pl;
    OAILOG_DEBUG(LOG_MME_APP, "Finalized the bearer level QoS of bearer (ebi=%d) for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id, bearer_context->ebi);
  }
  if(ambr){
    if(ambr->br_dl && ambr->br_ul){
    	/** The APN-AMBR is checked at the beginning of the S11 CBR and the possible UE AMBR is also calculated there. */
    	pdn_context->subscribed_apn_ambr.br_dl = ambr->br_dl;
    	pdn_context->subscribed_apn_ambr.br_ul = ambr->br_ul;
    }
  }
//  if(pco){
//     if (bearer_context->esm_ebr_context.pco) {
//       free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
//     }
//     /** Make it with memcpy, don't use bearer.. */
//     bearer_context->esm_ebr_context.pco = (protocol_configuration_options_t *) calloc (1, sizeof (protocol_configuration_options_t));
//     memcpy(bearer_context->esm_ebr_context.pco, pco, sizeof (protocol_configuration_options_t));  /**< Should have the processed bitmap in the validation . */
//   }

  /** Update the bearer context state. */
  if(bearer_context->bearer_state & BEARER_STATE_ENB_CREATED){
	  OAILOG_INFO(LOG_MME_APP, "Bearer state of (ebi=%d) is ESM_ACTIVE for UE: " MME_UE_S1AP_ID_FMT ". "
			  "Activating bearer context state.. \n",bearer_context->ebi,  ue_id);
	  bearer_context->bearer_state |= BEARER_STATE_ACTIVE;
  }

  // todo: UNLOCK_UE_CONTEXT
  OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SUCCESS);
}

//------------------------------------------------------------------------------
void
mme_app_release_bearer_context(mme_ue_s1ap_id_t ue_id, const pdn_cid_t *pdn_cid, const ebi_t linked_ebi, const ebi_t ebi){
  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t                       *pdn_context = NULL;
  bearer_context_t                    *bearer_context = NULL;
  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }
  if (ebi != EPS_BEARER_IDENTITY_UNASSIGNED) {
    if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
  }else{
    OAILOG_FUNC_OUT(LOG_MME_APP);
  }

  if(pdn_cid != NULL && *pdn_cid!= MAX_APN_PER_UE){
    mme_app_get_pdn_context(ue_id, *pdn_cid, linked_ebi, NULL, &pdn_context);
    if(!pdn_context){
      OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - PDN connection identifier %d " "is not valid\n", *pdn_cid);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
    /* Get the bearer context. */
    bearer_context = mme_app_get_session_bearer_context(pdn_context, ebi);
    if(!bearer_context) {
      OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - No bearer context for (ebi=%d) " "could be found. \n", ebi);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
  }else{
    /** Get the bearer context from all session bearers. */
    mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
    if(!bearer_context){
      OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Could not find bearer context from ebi=%d for UE " MME_UE_S1AP_ID_FMT". \n", ebi, ue_id);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
    /** Get the PDN context. */
    mme_app_get_pdn_context(ue_id, bearer_context->pdn_cx_id, bearer_context->linked_ebi, NULL, &pdn_context);
    if(!pdn_context){
      OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Could not find pdn context from bearer context with ebi=%d for UE " MME_UE_S1AP_ID_FMT". \n", ebi, ue_id);
      OAILOG_FUNC_OUT(LOG_MME_APP);
    }
  }

  /*
   * Check if it is a default ebi, if so release all the bearer contexts.
   * If not, just release a single bearer context.
   */
  if (ebi == bearer_context->linked_ebi || ebi == ESM_SAP_ALL_EBI || (pdn_context && pdn_context->default_ebi == ebi)) {
    DevMessage("Default Bearer should not be released via this method!");
  }

  // TODO: LOCK_UE_CONTEXT!
  /*
   * Release all session bearers of the PDN context back into the UE pool.
   */
  DevAssert(RB_REMOVE(SessionBearers, &pdn_context->session_bearers, bearer_context));
  /*
   * We don't have one pool where tunnels are allocated. We allocate a fixed number of bearer contexts at the beginning inside the UE context.
   * So the delete function is unlike to GTPv2c tunnels.
   */
  // no timers to stop, no DSR to be sent..
  /** Initialize the new bearer context. Nothing needs to be done in the ESM layer. */
  mme_app_bearer_context_initialize(bearer_context);
  /** Insert the bearer context into the free bearer of the ue context. */
  RB_INSERT (BearerPool, &ue_context->bearer_pool, bearer_context);
  OAILOG_INFO(LOG_MME_APP, "Successfully deregistered the bearer context with ebi %d from PDN id %u and for ue_id " MME_UE_S1AP_ID_FMT "\n",
      bearer_context->ebi, bearer_context->pdn_cx_id, ue_id);
  // TODO: UNLOCK_UE_CONTEXT!
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
esm_cause_t
mme_app_validate_bearer_resource_modification(mme_ue_s1ap_id_t ue_id, ebi_t ebi, ebi_t * linked_ebi, traffic_flow_aggregate_description_t *tad, flow_qos_t * flow_qos){
  OAILOG_FUNC_IN (LOG_MME_APP);
  pdn_context_t                       *pdn_context = NULL;
  bearer_context_t                    *bearer_context = NULL;
  ue_context_t                        *ue_context = mme_ue_context_exists_mme_ue_s1ap_id (&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }
  if (ebi != EPS_BEARER_IDENTITY_UNASSIGNED) {
    if ((ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX)) {
      OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
    }
  } else{
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_INVALID_MANDATORY_INFO);
  }

  /** Get the bearer context from all session bearers. */
  mme_app_get_session_bearer_context_from_all(ue_context, ebi, &bearer_context);
  if(!bearer_context){
    OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Could not find bearer context from ebi %d for ue_id "MME_UE_S1AP_ID_FMT ". \n", ebi, ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /** Find the PDN context. */
  mme_app_get_pdn_context(ue_id, bearer_context->pdn_cx_id, bearer_context->linked_ebi, NULL, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Could not find pdn context from ebi %d for (linked_ebi=%d,pdn_cid=%d) for UE " MME_UE_S1AP_ID_FMT". \n", ebi, bearer_context->linked_ebi, bearer_context->pdn_cx_id, ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_PDN_CONNECTION_DOES_NOT_EXIST);
  }

  *linked_ebi = pdn_context->default_ebi;

  /*
   * Check if it is a default ebi, if so release all the bearer contexts.
   * If not, just release a single bearer context.
   */
  if (ebi == bearer_context->linked_ebi || ebi == ESM_SAP_ALL_EBI || (pdn_context && pdn_context->default_ebi == ebi)) {
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }

  /** Check that the bearer has a TFT. */
  if(!bearer_context->esm_ebr_context.tft){
    OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Bearer context to be modified (ebi=%d) does not contain  for (linked_ebi=%d,pdn_cid=%d) for ue_id" MME_UE_S1AP_ID_FMT". \n", ebi, bearer_context->linked_ebi, bearer_context->pdn_cx_id, ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }

  /*
   * Verify the received TAD for any syntactical errors!
   */
  if(!((tad->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT )
    || (tad->tftoperationcode != TRAFFIC_FLOW_TEMPLATE_OPCODE_NO_TFT_OPERATION))) {
    OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - Received an invalid TFT operation (%d) (not implemented) for bearer (ebi=%d, pdn_cid=%d) for ue_id" MME_UE_S1AP_ID_FMT". \n",
        tad->tftoperationcode, ebi, bearer_context->pdn_cx_id, ue_id);
    OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
  }

  /** Verify the TFT. */
  esm_cause_t esm_cause = verify_traffic_flow_template(tad, bearer_context->esm_ebr_context.tft);
  if(esm_cause != ESM_CAUSE_SUCCESS){
    OAILOG_ERROR(LOG_MME_APP , "ESM-PROC  - ESM error (esm_cause=%d) for the to be modified bearer (ebi=%d) for ue_id " MME_UE_S1AP_ID_FMT". \n", esm_cause, ebi, ue_id);
    /** If the operation code was DELETE from TFT, check if the bearer is rendered empty. */
    if(tad->tftoperationcode != TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT){
      /** Return error. */
      OAILOG_FUNC_RETURN(LOG_MME_APP, esm_cause);
    }
    if(esm_cause == ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION) {
      /**
       * If a Semantic Error has been received, it means, that the current TFT has been rendered empty.
       * Trigger a bearer removal. We don't care about packet filters being removed not in the original TFT.
       */
      if(!bearer_context->esm_ebr_context.tft->numberofpacketfilters){
        OAILOG_FUNC_RETURN(LOG_MME_APP, esm_cause);
      }
    } else if(esm_cause == ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION) {
      /*
       * A packet filter is to be removed, which is not in the current TFT. Still forward it to the SAE-GW (with others).
       * Save the TAD, to remove it from the UE, even it is not received by the SAE-GW.
       */
    } else {
      OAILOG_FUNC_RETURN(LOG_MME_APP, esm_cause);
    }
  }

  if(esm_cause != ESM_CAUSE_SUCCESS) {
    OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "The received TAD for ue_id " MME_UE_S1AP_ID_FMT" (ebi=%d) resulted in esm_error=%d. "
        "Continuing to process it and ignoring the error.\n", ue_id, ebi, esm_cause);
  }
  esm_cause_t flow_esm_cause = ESM_CAUSE_SUCCESS;
  if(flow_qos && flow_qos->qci){
    if((5 <= flow_qos->qci && flow_qos->qci <= 9) || (69 <= flow_qos->qci && flow_qos->qci <= 70) || (79 == flow_qos->qci)){
      /** Return error, no modification on non-GBR is allowed. */
      flow_esm_cause = ESM_CAUSE_EPS_QOS_NOT_ACCEPTED;
    }
    /** Verify the received gbr qos, if any is received. */
    else if(validateEpsQosParameter(flow_qos->qci, PRE_EMPTION_VULNERABILITY_DISABLED, PRE_EMPTION_CAPABILITY_DISABLED, PRIORITY_LEVEL_MIN,
        flow_qos->gbr.br_dl, flow_qos->gbr.br_ul, flow_qos->mbr.br_dl, flow_qos->mbr.br_ul) == RETURNerror){
      OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "BRM-Request for UE " MME_UE_S1AP_ID_FMT" and ebi=%d could not be verified due erroneous EPS QoS.\n", ue_id, ebi);
      flow_esm_cause = ESM_CAUSE_EPS_QOS_NOT_ACCEPTED;
    } else if (flow_qos->qci != bearer_context->bearer_level_qos.qci) {
      OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "BRM-Request for UE " MME_UE_S1AP_ID_FMT" and ebi=%d has a qci=%d unequal to the current qci=%d of the bearer context.\n", ue_id, ebi,
          flow_qos->qci, bearer_context->bearer_level_qos.qci);
      flow_esm_cause = ESM_CAUSE_EPS_QOS_NOT_ACCEPTED;
    }
  }
  /** If there is a flow-qos error, set the QCI to 0. */
  if(!flow_qos || flow_qos != ESM_CAUSE_SUCCESS){
    if(flow_qos)
    	flow_qos->qci = 0;
    if(tad->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_NO_TFT_OPERATION){
      OAILOG_ERROR(LOG_MME_APP, "EMMCN-SAP  - " "BRM Request for UE " MME_UE_S1AP_ID_FMT" and ebi=%d has no qos element and no TFT operation. Rejecting.\n", ue_id, ebi);
      OAILOG_FUNC_RETURN(LOG_MME_APP, ESM_CAUSE_REQUEST_REJECTED_BY_GW);
    }
  }
  OAILOG_FUNC_RETURN(LOG_MME_APP, esm_cause); /**< Return the TFT cause. */
}

/**
 * The bearer context should be locked when using this..
 */
//------------------------------------------------------------------------------
esm_cause_t
static mme_app_esm_bearer_context_finalize_tft(mme_ue_s1ap_id_t ue_id, bearer_context_t * bearer_context, traffic_flow_template_t * tft){
  OAILOG_FUNC_IN (LOG_MME_APP);
  int                                     found = false;

  /** Update TFT. */
  if(tft){
    OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Updating TFT for EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n", bearer_context->ebi, ue_id);
    if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_ADD_PACKET_FILTER_TO_EXISTING_TFT){
      /**
       * todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors).
       * & check that the bearer context in question has tft (and if delete, at least one TFT remains.
        */
       for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
         /**
          * Assume that the identifier will be used as the ordinal.
          */
         packet_filter_t *new_packet_filter = NULL;
         int num_pf1 = 0;
         for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
           /** Update the packet filter with the correct identifier. */
           if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].length == 0){
             new_packet_filter = &bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1];
             break;
           }
         }
         if(!new_packet_filter){
        	 OAILOG_ERROR(LOG_MME_APP, "ESM-PROC  - Could extend the packet filter for UE " MME_UE_S1AP_ID_FMT". "
        			 "Current number of packet filters: %d.", ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
             OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION);
         }
         DevAssert(new_packet_filter); /**< todo: make synctactical check before. */
         /** Clean up the packet filter. */
         memset((void*)new_packet_filter, 0, sizeof(*new_packet_filter));
         // todo: any variability in size?
         memcpy((void*)new_packet_filter, (void*)&tft->packetfilterlist.addpacketfilter[num_pf], sizeof(tft->packetfilterlist.addpacketfilter[num_pf]));
         OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Added new packet filter with packet filter id %d to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
             tft->packetfilterlist.addpacketfilter[num_pf].identifier, bearer_context->ebi, ue_id);
         bearer_context->esm_ebr_context.tft->numberofpacketfilters++;
         /** Update the flags. */
         bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap |= tft->packet_filter_identifier_bitmap;
         DevAssert(!bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence]);
         bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence] = new_packet_filter->identifier + 1;
         OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Set precedence %d as pfId %d to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
             new_packet_filter->eval_precedence, bearer_context->esm_ebr_context.tft->precedence_set[new_packet_filter->eval_precedence], bearer_context->ebi, ue_id);
       }
       OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Successfully added %d new packet filters to EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
           "Current number of packet filters of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
     } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_DELETE_PACKET_FILTERS_FROM_EXISTING_TFT){
       // todo: check that at least one TFT exists, if not signal error !
       /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
       for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
         /**
          * Assume that the identifier will be used as the ordinal.
          */
         int num_pf1 = 0;
         for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
           /** Update the packet filter with the correct identifier. */
           if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.deletepacketfilter[num_pf].identifier)
             break;
         }
         DevAssert(num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX); /**< todo: make syntactical check before. */
         /** Remove the packet filter. */
         OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
             tft->packetfilterlist.deletepacketfilter[num_pf].identifier, bearer_context->ebi, ue_id);
         /** Remove from precedence list. */
         bearer_context->esm_ebr_context.tft->precedence_set[bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].eval_precedence] = 0;
         memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0,
             sizeof(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1]));
         bearer_context->esm_ebr_context.tft->numberofpacketfilters--;
       }
       OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Successfully deleted %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
           "Current number of packet filters of bearer context %d. \n", tft->numberofpacketfilters, bearer_context->ebi, ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
       /** Update the flags. */
       bearer_context->esm_ebr_context.tft->packet_filter_identifier_bitmap &= ~tft->packet_filter_identifier_bitmap;
     } else if(tft->tftoperationcode == TRAFFIC_FLOW_TEMPLATE_OPCODE_REPLACE_PACKET_FILTERS_IN_EXISTING_TFT){
       /** todo: (6.1.3.3.4) check at the beginning that all the packet filters exist, before continuing (check syntactical errors). */
       for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
         /**
          * Assume that the identifier will be used as the ordinal.
          */
         int num_pf1 = 0;
         for(; num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX; num_pf1++) {
           /** Update the packet filter with the correct identifier. */
           if(bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].identifier == tft->packetfilterlist.replacepacketfilter[num_pf].identifier)
             break;
         }
         DevAssert(num_pf1 < TRAFFIC_FLOW_TEMPLATE_NB_PACKET_FILTERS_MAX); /**< todo: make syntactical check before. */
         /** Clean the old packet filter and the precedence map entry. The identifier will stay.. */
         bearer_context->esm_ebr_context.tft->precedence_set[bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1].eval_precedence] = 0;
         memset((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], 0, sizeof(create_new_tft_t));
         OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Removed the packet filter with packet filter id %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
             tft->packetfilterlist.replacepacketfilter[num_pf].identifier, bearer_context->ebi, ue_id);
         // todo: use length?
         memcpy((void*)&bearer_context->esm_ebr_context.tft->packetfilterlist.createnewtft[num_pf1], &tft->packetfilterlist.replacepacketfilter[num_pf], sizeof(replace_packet_filter_t));
       }
       OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Successfully replaced %d existing packet filters of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". "
           "Current number of packet filter of bearer %d. \n", tft->numberofpacketfilters, bearer_context->ebi, ue_id, bearer_context->esm_ebr_context.tft->numberofpacketfilters);
       /** Removed the precedences, set them again, should be all zero. */
       for(int num_pf = 0 ; num_pf < tft->numberofpacketfilters; num_pf++){
         DevAssert(!bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence]);
         OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Successfully set the new precedence of packet filter id (%d +1) to %d of EPS bearer context (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT "\n. ",
                   tft->packetfilterlist.replacepacketfilter[num_pf].identifier, tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence, bearer_context->ebi, ue_id);
         bearer_context->esm_ebr_context.tft->precedence_set[tft->packetfilterlist.replacepacketfilter[num_pf].eval_precedence] = tft->packetfilterlist.replacepacketfilter[num_pf].identifier + 1;
       }
       /** No need to update the identifier bitmap. */
     } else {
       // todo: check that no other operation code is permitted, bearers without tft not permitted and default bearer may not have tft
       OAILOG_INFO (LOG_MME_APP, "ESM-PROC  - Received invalid TFT operation code %d for bearer with (ebi=%d) for UE with ueId " MME_UE_S1AP_ID_FMT ". \n",
           tft->tftoperationcode, bearer_context->ebi, ue_id);
       OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION);
    }
  }
  OAILOG_FUNC_RETURN (LOG_MME_APP, ESM_CAUSE_SUCCESS);
}
