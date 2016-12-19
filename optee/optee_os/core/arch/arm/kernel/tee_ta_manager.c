/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <tee_api_types.h>
#include <kernel/tee_ta_manager.h>
#include <mm/tee_mmu_types.h>
#include <mm/core_memprot.h>
#include <kernel/thread.h>
#include <sm/optee_smc.h>
#include <optee_msg.h>
#include <trace.h>
#include <kernel/tee_time.h>
//#define PPDEBUG
//#define PPTIME

/**
 * tee_ta_verify_param() - check that the 4 "params" match security
 */
TEE_Result tee_ta_verify_param(struct tee_ta_session *sess,
			       struct tee_ta_param *param)
{
	tee_paddr_t p;
	size_t l;
	int n;
	#ifdef PPDEBUG
 	uint32_t ret;
 	#endif
 	#ifdef PPTIME
 	TEE_Time old_time;
 	TEE_Time new_time;
 	#endif
 	unsigned long buff;
 	unsigned old_param_type;
 	struct optee_msg_param my_params[2];
 	
 	memset(my_params, 0, sizeof(my_params));

	for (n = 0; n < TEE_NUM_PARAMS; n++) {
		switch (TEE_PARAM_TYPE_GET(param->types, n)) {
		case TEE_PARAM_TYPE_MEMREF_OUTPUT:
		case TEE_PARAM_TYPE_MEMREF_INOUT:
		case TEE_PARAM_TYPE_MEMREF_INPUT:

			if (param->param_attr[n] & TEE_MATTR_VIRTUAL) {
				p = virt_to_phys(
					param->params[n].memref.buffer);
				if (!p)
					return TEE_ERROR_SECURITY;
			} else {
				p = (tee_paddr_t)param->params[n].memref.buffer;
			}
			l = param->params[n].memref.size;

			if (core_pbuf_is(CORE_MEM_NSEC_SHM, p, l))
				break;
			if ((sess->ctx->flags & TA_FLAG_UNSAFE_NW_PARAMS) &&
				core_pbuf_is(CORE_MEM_MULTPURPOSE, p, l))
				break;
			if ((sess->clnt_id.login == TEE_LOGIN_TRUSTED_APP) &&
				core_pbuf_is(CORE_MEM_TA_RAM, p, l))
				break;

			return TEE_ERROR_SECURITY;
		case TEE_PARAM_TYPE_PP_MEMREF_OUTPUT:
 		case TEE_PARAM_TYPE_PP_MEMREF_INOUT:
 		case TEE_PARAM_TYPE_PP_MEMREF_INPUT:
 				old_param_type = TEE_PARAM_TYPE_GET(param->types, n);
 				buff = old_param_type - TEE_PARAM_TYPE_PP_MEMREF_INPUT;
 				param->types &= ~(0xf << (n*4));
 						//DMSG("BOOMERANG:AFTER PRUNING:0x%x\n", param->types);
 				param->types |= ((buff + TEE_PARAM_TYPE_MEMREF_INPUT) << (n*4));
 				if(param->params[n].memref.buffer) {
 					// This is input request to verify.
 					my_params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INOUT;
 					buff = (unsigned long)param->params[n].memref.buffer;
 					my_params[0].u.value.a = buff;
 					my_params[0].u.value.b = (uint64_t)param->params[n].memref.size;
 					my_params[0].u.value.c = (uint64_t)sess->clnt_id.ns_client_pid;
 					buff = (unsigned long)sess;
 					my_params[1].u.value.b = buff;
 					// This is where we get physical address from Linux kernel.
 					my_params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INOUT;
 					// This is where the result comes.
 					// 0 => Success else Failure
 					// Set this to -1, so that if kernel returns success,
 					// we will know
 					my_params[1].u.value.a = -1;
 					my_params[1].u.value.c = old_param_type - TEE_PARAM_TYPE_PP_MEMREF_INPUT;
 					#ifdef PPDEBUG
 					DMSG("BOOMERANG SENDING RPC param[0].a=0x%llx, param[0].b=0x%llx, param[0].c=0x%llx\n", my_params[0].u.value.a, my_params[0].u.value.b, my_params[0].u.value.c); 
 					#endif
 					#ifdef PPTIME
 					tee_time_get_sys_time(&old_time);
 					#endif
 					#ifdef PPDEBUG
 					ret = thread_rpc_cmd(OPTEE_SMC_RETURN_PP_CHECK, 2, my_params);
 					#else
 					thread_rpc_cmd(OPTEE_SMC_RETURN_PP_CHECK, 2, my_params);
 					#endif
 					if(!my_params[1].u.value.a) {
 						#ifdef PPDEBUG
 						DMSG("BOOMERANG RPC RET=0x%x, param[1].a=0x%llx, param[1].b=0x%llx, param[1].c=0x%llx\n", ret, my_params[1].u.value.a, my_params[1].u.value.b, my_params[1].u.value.c); 
 						#endif
 						#ifdef PPTIME
 						tee_time_get_sys_time(&new_time);
 						DMSG("BOOMERANG RPC PP CHECK TIME, seconds=%d, milliseconds=%d, clockcyclesdiff=0x%x, clockcyclespersecond=0x%x\n",new_time.seconds-old_time.seconds, new_time.millis-old_time.millis, new_time.millif-old_time.millif, new_time.clk_freq);
 						#endif
 						
 						buff = my_params[1].u.value.b;
 						param->params[n].memref.buffer = (void*)buff;
 						buff = my_params[1].u.value.c;
 						if(my_params[1].u.value.c) 
 							param->conv_params[n].memref.buffer = (void*)buff;
 						//DMSG("BOOMERANG:AFTER SETTING:0x%x\n", param->types);
 					} else {
 						return TEE_ERROR_SECURITY;
 					}
 				}
 				break;
		default:
			break;
		}
	}
	return TEE_SUCCESS;
}
