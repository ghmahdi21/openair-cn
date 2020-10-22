/*
 * Generated by asn1c-0.9.24 (http://lionet.info/asn1c)
 * From ASN.1 module "S1AP-IEs"
 * 	found in "S1AP-IEs.asn"
 */

#ifndef	_S1ap_BroadcastCompletedAreaList_H_
#define	_S1ap_BroadcastCompletedAreaList_H_


#include <asn_application.h>

/* Including external dependencies */
#include "S1ap-CellID-Broadcast.h"
#include "S1ap-TAI-Broadcast.h"
#include "S1ap-EmergencyAreaID-Broadcast.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum S1ap_BroadcastCompletedAreaList_PR {
	S1ap_BroadcastCompletedAreaList_PR_NOTHING,	/* No components present */
	S1ap_BroadcastCompletedAreaList_PR_cellID_Broadcast,
	S1ap_BroadcastCompletedAreaList_PR_tAI_Broadcast,
	S1ap_BroadcastCompletedAreaList_PR_emergencyAreaID_Broadcast,
	/* Extensions may appear below */
	
} S1ap_BroadcastCompletedAreaList_PR;

/* S1ap-BroadcastCompletedAreaList */
typedef struct S1ap_BroadcastCompletedAreaList {
	S1ap_BroadcastCompletedAreaList_PR present;
	union S1ap_BroadcastCompletedAreaList_u {
		S1ap_CellID_Broadcast_t	 cellID_Broadcast;
		S1ap_TAI_Broadcast_t	 tAI_Broadcast;
		S1ap_EmergencyAreaID_Broadcast_t	 emergencyAreaID_Broadcast;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} S1ap_BroadcastCompletedAreaList_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_S1ap_BroadcastCompletedAreaList;

#ifdef __cplusplus
}
#endif

#endif	/* _S1ap_BroadcastCompletedAreaList_H_ */
#include <asn_internal.h>
